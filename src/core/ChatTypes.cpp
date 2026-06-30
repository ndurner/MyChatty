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
            body["tools"] = QJsonArray{QJsonObject{
                {"type", "mcp"},
                {"server_label", "exa"},
                {"server_description", "Exa web search and webpage fetch tools."},
                {"server_url", exaMcpServerUrl(request.exaApiKey)},
                {"require_approval", "never"},
            }};
        } else {
            body["tools"] = QJsonArray{QJsonObject{
                {"type", "web_search"},
                {"search_context_size", "medium"},
            }};
            body["include"] = QJsonArray{"reasoning.encrypted_content", "web_search_call.action.sources"};
        }
    }
    return body;
}

QJsonObject buildOpenaiChatPayload(const ChatRequest &request)
{
    QJsonArray messages;
    if (!request.customInstructions.trimmed().isEmpty()) {
        messages.append(QJsonObject{
            {"role", "system"},
            {"content", request.customInstructions.trimmed()},
        });
    }

    for (const ChatMessage &message : request.history) {
        if (message.isUser()) {
            messages.append(QJsonObject{
                {"role", "user"},
                {"content", contentForChatUser(message)},
            });
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
        {"reasoning_effort", ModelCatalog::openRouterReasoningEffort(request.effort)},
    };
    if (request.maxOutputTokens > 0) {
        body["max_tokens"] = request.maxOutputTokens;
    }
    if (request.enableWebSearch) {
        QJsonObject webPlugin{{"id", "web"}};
        if (request.useExaSearch) {
            webPlugin["engine"] = "exa";
        }
        body["plugins"] = QJsonArray{webPlugin};
        body["web_search_options"] = QJsonObject{
            {"search_context_size", "medium"},
        };
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
