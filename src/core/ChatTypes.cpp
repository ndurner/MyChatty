#include "ChatTypes.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QMimeDatabase>
#include <QRandomGenerator>
#include <QUrl>
#include <QUrlQuery>

namespace MyChatty {

static constexpr auto ExaMcpServerUrl = "https://mcp.exa.ai/mcp";

static QString exaMcpServerUrl(const QString &configuredApiKey)
{
    QString apiKey = configuredApiKey.trimmed();
    if (apiKey.isEmpty()) {
        apiKey = QString::fromUtf8(qgetenv("EXA_API_KEY")).trimmed();
    }
    if (apiKey.isEmpty()) {
        return QString::fromLatin1(ExaMcpServerUrl);
    }

    QUrl url(QString::fromLatin1(ExaMcpServerUrl));
    QUrlQuery query(url);
    query.addQueryItem("exaApiKey", apiKey);
    url.setQuery(query);
    return url.toString();
}

QString newId(const QString &prefix)
{
    const auto now = QString::number(QDateTime::currentMSecsSinceEpoch(), 16);
    const auto rnd = QString::number(QRandomGenerator::global()->generate64(), 16);
    return prefix + "_" + now + rnd.left(8);
}

QString guessMimeType(const QString &path)
{
    QMimeDatabase db;
    const QMimeType type = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
    if (type.isValid() && !type.name().isEmpty()) {
        return type.name();
    }
    return "application/octet-stream";
}

QString fileNameFromUrl(const QUrl &url)
{
    const QString path = url.isLocalFile() ? url.toLocalFile() : url.path();
    const QString fileName = QFileInfo(path).fileName();
    return fileName.isEmpty() ? QStringLiteral("attachment") : fileName;
}

Attachment Attachment::fromUrl(const QUrl &url, const QString &origin)
{
    Attachment attachment;
    attachment.id = newId("att");
    attachment.url = url;
    attachment.localPath = url.isLocalFile() ? url.toLocalFile() : url.toString();
    attachment.fileName = fileNameFromUrl(url);
    attachment.mimeType = url.isLocalFile() ? guessMimeType(url.toLocalFile()) : "application/octet-stream";
    attachment.origin = origin;
    return attachment;
}

bool Attachment::isImage() const
{
    return mimeType.startsWith("image/");
}

bool Attachment::isTextLike() const
{
    return mimeType.startsWith("text/")
        || mimeType == "application/json"
        || mimeType == "application/xml"
        || mimeType == "application/javascript"
        || fileName.endsWith(".md", Qt::CaseInsensitive)
        || fileName.endsWith(".csv", Qt::CaseInsensitive);
}

static bool isPdfAttachment(const Attachment &attachment)
{
    return attachment.mimeType == QStringLiteral("application/pdf")
        || attachment.fileName.endsWith(".pdf", Qt::CaseInsensitive);
}

QByteArray Attachment::readBytes(qint64 limit) const
{
    if (localPath.isEmpty()) {
        return {};
    }
    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.read(limit + 1).left(limit);
}

QString Attachment::readText(qint64 limit) const
{
    return QString::fromUtf8(readBytes(limit));
}

QString Attachment::dataUrl(qint64 limit) const
{
    const QByteArray bytes = readBytes(limit);
    if (bytes.isEmpty()) {
        return {};
    }
    return QStringLiteral("data:%1;base64,%2")
        .arg(mimeType.isEmpty() ? QStringLiteral("application/octet-stream") : mimeType,
             QString::fromLatin1(bytes.toBase64()));
}

QVariantMap Attachment::toVariant() const
{
    QVariantMap map;
    map["id"] = id;
    map["url"] = url;
    map["path"] = localPath;
    map["fileName"] = fileName;
    map["mimeType"] = mimeType;
    map["origin"] = origin;
    map["isImage"] = isImage();
    return map;
}

QJsonObject Attachment::toJsonMetadata() const
{
    return {
        {"id", id},
        {"url", url.toString()},
        {"path", localPath},
        {"fileName", fileName},
        {"mimeType", mimeType},
        {"origin", origin},
    };
}

static QJsonArray attachmentsForResponses(const QList<Attachment> &attachments)
{
    QJsonArray parts;
    for (const Attachment &attachment : attachments) {
        if (isPdfAttachment(attachment)) {
            const QString data = attachment.dataUrl();
            if (!data.isEmpty()) {
                parts.append(QJsonObject{
                    {"type", "input_file"},
                    {"filename", attachment.fileName},
                    {"file_data", data},
                });
            }
            continue;
        }

        if (attachment.isImage()) {
            const QString data = attachment.dataUrl();
            if (!data.isEmpty()) {
                parts.append(QJsonObject{
                    {"type", "input_image"},
                    {"image_url", data},
                    {"detail", "high"},
                });
            }
            continue;
        }

        if (attachment.isTextLike()) {
            const QString text = attachment.readText();
            if (!text.isEmpty()) {
                parts.append(QJsonObject{
                    {"type", "input_text"},
                    {"text", QStringLiteral("Attached file: %1\n\n%2").arg(attachment.fileName, text)},
                });
            }
        } else {
            parts.append(QJsonObject{
                {"type", "input_text"},
                {"text", QStringLiteral("Attached file: %1 (%2). Binary content is available in the client export but was not inlined for this API call.")
                             .arg(attachment.fileName, attachment.mimeType)},
            });
        }
    }
    return parts;
}

static QJsonArray contentForResponsesUser(const ChatMessage &message)
{
    QJsonArray content;
    if (!message.text.trimmed().isEmpty()) {
        content.append(QJsonObject{{"type", "input_text"}, {"text", message.text}});
    }
    const QJsonArray attachmentParts = attachmentsForResponses(message.attachments);
    for (const auto &part : attachmentParts) {
        content.append(part);
    }
    if (content.isEmpty()) {
        content.append(QJsonObject{{"type", "input_text"}, {"text", ""}});
    }
    return content;
}

static QJsonArray attachmentsForChat(const QList<Attachment> &attachments)
{
    QJsonArray parts;
    for (const Attachment &attachment : attachments) {
        if (isPdfAttachment(attachment)) {
            const QString data = attachment.dataUrl();
            if (!data.isEmpty()) {
                parts.append(QJsonObject{
                    {"type", "file"},
                    {"file", QJsonObject{
                        {"filename", attachment.fileName},
                        {"file_data", data},
                    }},
                });
            }
            continue;
        }

        if (attachment.isImage()) {
            const QString data = attachment.dataUrl();
            if (!data.isEmpty()) {
                parts.append(QJsonObject{
                    {"type", "image_url"},
                    {"image_url", QJsonObject{{"url", data}}},
                });
            }
            continue;
        }

        if (attachment.isTextLike()) {
            const QString text = attachment.readText();
            if (!text.isEmpty()) {
                parts.append(QJsonObject{
                    {"type", "text"},
                    {"text", QStringLiteral("Attached file: %1\n\n%2").arg(attachment.fileName, text)},
                });
            }
        } else {
            parts.append(QJsonObject{
                {"type", "text"},
                {"text", QStringLiteral("Attached file: %1 (%2). Binary content is available in the client export but was not inlined for this API call.")
                             .arg(attachment.fileName, attachment.mimeType)},
            });
        }
    }
    return parts;
}

static QJsonValue contentForChatUser(const ChatMessage &message)
{
    const bool hasAttachments = !message.attachments.isEmpty();
    if (!hasAttachments) {
        return message.text;
    }

    QJsonArray content;
    if (!message.text.trimmed().isEmpty()) {
        content.append(QJsonObject{{"type", "text"}, {"text", message.text}});
    }
    const QJsonArray attachmentParts = attachmentsForChat(message.attachments);
    for (const auto &part : attachmentParts) {
        content.append(part);
    }
    return content;
}

static bool hasPdfAttachments(const QList<ChatMessage> &messages)
{
    for (const ChatMessage &message : messages) {
        for (const Attachment &attachment : message.attachments) {
            if (isPdfAttachment(attachment)) {
                return true;
            }
        }
    }
    return false;
}

static void appendTool(QJsonObject &body, const QJsonObject &tool)
{
    QJsonArray tools = body.value("tools").toArray();
    tools.append(tool);
    body["tools"] = tools;
}

static QJsonObject evalJavaScriptTool()
{
    return QJsonObject{
        {"type", "function"},
        {"name", "eval_javascript"},
        {"description", "Evaluate short JavaScript code in the configured Qt QJSEngine sandbox. Use this for arithmetic, dates, JSON, small tables, and deterministic calculations. Use print(...) for the final result. A scoped fs object exposes only this conversation's virtual files."},
        {"parameters", QJsonObject{
            {"type", "object"},
            {"properties", QJsonObject{
                {"javascript_source_code", QJsonObject{
                    {"type", "string"},
                    {"description", "JavaScript source code to execute. Keep it short and print the answer."},
                }},
            }},
            {"required", QJsonArray{"javascript_source_code"}},
            {"additionalProperties", false},
        }},
    };
}

static QJsonObject openRouterWebSearchTool(bool preferExa)
{
    QJsonObject tool{{"type", "openrouter:web_search"}};
    if (preferExa) {
        tool["parameters"] = QJsonObject{
            {"engine", "exa"},
            {"search_context_size", "medium"},
        };
    }
    return tool;
}

static QJsonObject chatFunctionTool(const QJsonObject &responsesTool)
{
    QJsonObject function = responsesTool;
    function.remove("type");
    QJsonObject parameters = function.value("parameters").toObject();
    parameters.remove("additionalProperties");
    function["parameters"] = parameters;
    return QJsonObject{
        {"type", "function"},
        {"function", function},
    };
}

QJsonObject buildOpenaiResponsesPayload(const ChatRequest &request)
{
    QJsonArray input;
    for (const ChatMessage &message : request.history) {
        if (message.isUser()) {
            input.append(QJsonObject{
                {"role", "user"},
                {"content", contentForResponsesUser(message)},
            });
        } else if (message.isAssistant()) {
            if (!message.rawOutputItems.isEmpty()) {
                for (const auto &item : message.rawOutputItems) {
                    input.append(item);
                }
            } else if (!message.text.isEmpty()) {
                input.append(QJsonObject{
                    {"role", "assistant"},
                    {"content", QJsonArray{QJsonObject{{"type", "output_text"}, {"text", message.text}}}},
                });
            }
        }
    }

    QJsonObject body{
        {"model", request.model.apiModel},
        {"input", input},
        {"store", false},
        {"stream", request.stream},
        {"reasoning", QJsonObject{
            {"effort", ModelCatalog::openAIReasoningEffort(request.effort)},
            {"summary", "auto"},
        }},
        {"text", QJsonObject{
            {"format", QJsonObject{{"type", "text"}}},
            {"verbosity", ModelCatalog::textVerbosity(request.effort)},
        }},
        {"include", QJsonArray{"reasoning.encrypted_content"}},
    };

    if (!request.customInstructions.trimmed().isEmpty()) {
        body["instructions"] = QStringLiteral("Formatting re-enabled\n%1").arg(request.customInstructions.trimmed());
    }
    if (request.maxOutputTokens > 0) {
        body["max_output_tokens"] = request.maxOutputTokens;
    }
    if (request.enableWebSearch) {
        if (request.useExaSearch) {
            appendTool(body, QJsonObject{
                {"type", "mcp"},
                {"server_label", "exa"},
                {"server_description", "Exa web search and webpage fetch tools."},
                {"server_url", exaMcpServerUrl(request.exaApiKey)},
                {"require_approval", "never"},
            });
        } else {
            appendTool(body, QJsonObject{
                {"type", "web_search"},
                {"search_context_size", "medium"},
            });
            body["include"] = QJsonArray{"reasoning.encrypted_content", "web_search_call.action.sources"};
        }
    }
    if (request.enableJavaScriptUse) {
        appendTool(body, evalJavaScriptTool());
    }
    return body;
}

QJsonObject buildOpenaiChatPayload(const ChatRequest &request)
{
    QJsonArray messages;
    QString systemInstructions = request.customInstructions.trimmed();
    if (request.enableWebSearch) {
        const QString webInstruction = QStringLiteral("Web search is available through the openrouter:web_search server tool. Use it for current facts, public people, organizations, news, prices, schedules, and other information that may be outside the model's training data.");
        systemInstructions = systemInstructions.isEmpty()
            ? webInstruction
            : systemInstructions + QStringLiteral("\n") + webInstruction;
    }
    if (!systemInstructions.isEmpty()) {
        messages.append(QJsonObject{
            {"role", "system"},
            {"content", systemInstructions},
        });
    }

    for (const ChatMessage &message : request.history) {
        if (message.isUser()) {
            messages.append(QJsonObject{
                {"role", "user"},
                {"content", contentForChatUser(message)},
            });
        } else if (!message.rawOutputItems.isEmpty()) {
            for (const auto &item : message.rawOutputItems) {
                messages.append(item);
            }
        } else if (message.isAssistant() && !message.text.isEmpty()) {
            messages.append(QJsonObject{
                {"role", "assistant"},
                {"content", message.text},
            });
        }
    }

    QJsonObject body{
        {"model", request.model.apiModel},
        {"messages", messages},
        {"stream", request.stream},
        {"reasoning", QJsonObject{
            {"effort", ModelCatalog::openRouterReasoningEffort(request.effort)},
            {"exclude", false},
        }},
    };
    if (request.maxOutputTokens > 0) {
        body["max_tokens"] = request.maxOutputTokens;
    }
    if (request.enableWebSearch) {
        QJsonArray tools = body.value("tools").toArray();
        tools.append(openRouterWebSearchTool(request.useExaSearch));
        body["tools"] = tools;
    }
    if (hasPdfAttachments(request.history)) {
        QJsonArray plugins = body.value("plugins").toArray();
        const QString engine = request.openRouterPdfEngine == QStringLiteral("native")
            ? QStringLiteral("native")
            : QStringLiteral("cloudflare-ai");
        plugins.append(QJsonObject{
            {"id", "file-parser"},
            {"pdf", QJsonObject{{"engine", engine}}},
        });
        body["plugins"] = plugins;
    }
    if (request.enableJavaScriptUse) {
        QJsonArray tools = body.value("tools").toArray();
        tools.append(chatFunctionTool(evalJavaScriptTool()));
        body["tools"] = tools;
    }
    if (!request.model.providerOnly.isEmpty()) {
        QJsonArray providers;
        for (const QString &provider : request.model.providerOnly) {
            providers.append(provider);
        }
        body["provider"] = QJsonObject{
            {"only", providers},
            {"order", providers},
            {"allow_fallbacks", false},
        };
    }
    return body;
}

} // namespace MyChatty
