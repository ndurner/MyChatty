#include "ApiProtocolAdapter.h"

#include "OpenaiChatAPI.h"
#include "OpenaiResponsesAPI.h"

#include <QFile>
#include <QJsonDocument>

#include <utility>

namespace MyChatty {
namespace {

QJsonObject parseArguments(const QString &text)
{
    const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
    return doc.isObject() ? doc.object() : QJsonObject{};
}

QString imageDataUrl(const QString &path, const QString &mimeType)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QByteArray bytes = file.readAll();
    if (bytes.isEmpty()) {
        return {};
    }
    const QString type = mimeType.trimmed().isEmpty() ? QStringLiteral("image/png") : mimeType.trimmed();
    return QStringLiteral("data:%1;base64,%2")
        .arg(type, QString::fromLatin1(bytes.toBase64()));
}

QJsonValue responsesFunctionOutput(const QJsonObject &output)
{
    const QString imagePath = output.value(QStringLiteral("image_path")).toString();
    if (imagePath.isEmpty()) {
        return QString::fromUtf8(QJsonDocument(output).toJson(QJsonDocument::Compact));
    }

    QJsonObject textObject = output;
    textObject.remove(QStringLiteral("image_path"));
    textObject.remove(QStringLiteral("image_mime_type"));
    textObject.remove(QStringLiteral("image_detail"));

    const QString dataUrl = imageDataUrl(imagePath, output.value(QStringLiteral("image_mime_type")).toString());
    if (dataUrl.isEmpty()) {
        return QString::fromUtf8(QJsonDocument(output).toJson(QJsonDocument::Compact));
    }

    return QJsonArray{
        QJsonObject{
            {QStringLiteral("type"), QStringLiteral("input_text")},
            {QStringLiteral("text"), QString::fromUtf8(QJsonDocument(textObject).toJson(QJsonDocument::Compact))},
        },
        QJsonObject{
            {QStringLiteral("type"), QStringLiteral("input_image")},
            {QStringLiteral("image_url"), dataUrl},
            {QStringLiteral("detail"), output.value(QStringLiteral("image_detail")).toString(QStringLiteral("high"))},
        },
    };
}

QJsonValue chatToolOutput(const QJsonObject &output)
{
    const QString imagePath = output.value(QStringLiteral("image_path")).toString();
    if (imagePath.isEmpty()) {
        return QString::fromUtf8(QJsonDocument(output).toJson(QJsonDocument::Compact));
    }

    QJsonObject textObject = output;
    textObject.remove(QStringLiteral("image_path"));
    textObject.remove(QStringLiteral("image_mime_type"));
    textObject.remove(QStringLiteral("image_detail"));

    const QString dataUrl = imageDataUrl(imagePath, output.value(QStringLiteral("image_mime_type")).toString());
    if (dataUrl.isEmpty()) {
        return QString::fromUtf8(QJsonDocument(output).toJson(QJsonDocument::Compact));
    }

    return QJsonArray{
        QJsonObject{
            {QStringLiteral("type"), QStringLiteral("text")},
            {QStringLiteral("text"), QString::fromUtf8(QJsonDocument(textObject).toJson(QJsonDocument::Compact))},
        },
        QJsonObject{
            {QStringLiteral("type"), QStringLiteral("image_url")},
            {QStringLiteral("image_url"), QJsonObject{{QStringLiteral("url"), dataUrl}}},
        },
    };
}

} // namespace

QList<ToolCall> OpenAIResponsesProtocolAdapter::extractToolCalls(const ChatResult &result) const
{
    QList<ToolCall> calls;
    for (const QJsonValue &itemValue : result.rawOutputItems) {
        const QJsonObject item = itemValue.toObject();
        if (item.value(QStringLiteral("type")).toString() != QStringLiteral("function_call")) {
            continue;
        }
        ToolCall call{
            item.value(QStringLiteral("call_id")).toString(),
            item.value(QStringLiteral("name")).toString(),
            parseArguments(item.value(QStringLiteral("arguments")).toString()),
        };
        if (!call.id.isEmpty() && !call.name.isEmpty()) {
            calls.append(std::move(call));
        }
    }
    return calls;
}

QJsonObject OpenAIResponsesProtocolAdapter::serializeToolOutput(const QString &callId, const QJsonObject &output) const
{
    return QJsonObject{
        {QStringLiteral("type"), QStringLiteral("function_call_output")},
        {QStringLiteral("call_id"), callId},
        {QStringLiteral("output"), responsesFunctionOutput(output)},
    };
}

std::unique_ptr<ApiClient> OpenAIResponsesProtocolAdapter::createClient(QNetworkAccessManager *network) const
{
    return std::make_unique<OpenaiResponsesAPI>(network);
}

QList<ToolCall> ChatCompletionsProtocolAdapter::extractToolCalls(const ChatResult &result) const
{
    QList<ToolCall> calls;
    for (const QJsonValue &itemValue : result.rawOutputItems) {
        const QJsonArray toolCalls = itemValue.toObject().value(QStringLiteral("tool_calls")).toArray();
        for (const QJsonValue &callValue : toolCalls) {
            const QJsonObject callObject = callValue.toObject();
            const QJsonObject function = callObject.value(QStringLiteral("function")).toObject();
            ToolCall call{
                callObject.value(QStringLiteral("id")).toString(),
                function.value(QStringLiteral("name")).toString(),
                parseArguments(function.value(QStringLiteral("arguments")).toString()),
            };
            if (!call.id.isEmpty() && !call.name.isEmpty()) {
                calls.append(std::move(call));
            }
        }
    }
    return calls;
}

QJsonObject ChatCompletionsProtocolAdapter::serializeToolOutput(const QString &callId, const QJsonObject &output) const
{
    return QJsonObject{
        {QStringLiteral("role"), QStringLiteral("tool")},
        {QStringLiteral("tool_call_id"), callId},
        {QStringLiteral("content"), chatToolOutput(output)},
    };
}

std::unique_ptr<ApiClient> ChatCompletionsProtocolAdapter::createClient(QNetworkAccessManager *network) const
{
    return std::make_unique<OpenaiChatAPI>(network);
}

const ProviderProfile &providerProfile(ApiProvider provider)
{
    static const OpenAIResponsesProtocolAdapter responses;
    static const ChatCompletionsProtocolAdapter chatCompletions;
    static const ProviderProfile openAI{
        ApiProvider::OpenAIResponses,
        QStringLiteral("https://api.openai.com/v1/responses"),
        &responses,
        false,
    };
    static const ProviderProfile openRouter{
        ApiProvider::OpenRouterChat,
        QStringLiteral("https://openrouter.ai/api/v1/chat/completions"),
        &chatCompletions,
        true,
    };
    static const ProviderProfile nvidia{
        ApiProvider::NvidiaChat,
        QStringLiteral("https://integrate.api.nvidia.com/v1/chat/completions"),
        &chatCompletions,
        false,
    };

    switch (provider) {
    case ApiProvider::OpenRouterChat:
        return openRouter;
    case ApiProvider::NvidiaChat:
        return nvidia;
    case ApiProvider::OpenAIResponses:
        return openAI;
    }
    return openAI;
}

const ApiProtocolAdapter &protocolAdapter(ApiProvider provider)
{
    return *providerProfile(provider).protocol;
}

} // namespace MyChatty
