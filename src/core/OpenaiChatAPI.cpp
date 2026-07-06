#include "OpenaiChatAPI.h"

#include <QJsonDocument>
#include <QNetworkReply>

namespace MyChatty {

OpenaiChatAPI::OpenaiChatAPI(QObject *parent)
    : ApiClient(parent)
{
    connect(&m_parser, &SseParser::eventReceived, this, &OpenaiChatAPI::handleEvent);
}

void OpenaiChatAPI::send(const ChatRequest &request)
{
    m_done = false;
    m_result = {};
    m_responseBody.clear();
    m_reasoningDetails = {};
    m_toolCalls.clear();

    QNetworkRequest networkRequest = jsonRequest(QUrl("https://openrouter.ai/api/v1/chat/completions"), request.apiKey);
    networkRequest.setRawHeader("HTTP-Referer", "https://github.com/ndurner/MyChatty");
    networkRequest.setRawHeader("X-Title", "MyChatty");

    const QByteArray body = QJsonDocument(buildOpenaiChatPayload(request)).toJson(QJsonDocument::Compact);
    m_reply = m_network.post(networkRequest, body);

    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        if (!m_reply) {
            return;
        }
        const QByteArray chunk = m_reply->readAll();
        m_responseBody.append(chunk);
        if (m_responseBody.size() > 4096) {
            m_responseBody.truncate(4096);
        }
        m_parser.feed(chunk);
    });
    connect(m_reply, &QNetworkReply::finished, this, &OpenaiChatAPI::finishFromReply);
}

void OpenaiChatAPI::handleEvent(const QString &, const QByteArray &data)
{
    if (data.trimmed() == "[DONE]") {
        completeIfNeeded();
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return;
    }
    const QJsonObject object = doc.object();
    m_result.rawEvents.append(object);

    if (object.contains("error")) {
        const QJsonObject error = object.value("error").toObject();
        QString message = error.value("metadata").toObject().value("raw").toString();
        if (message.isEmpty()) {
            message = error.value("message").toString(QStringLiteral("OpenRouter request failed"));
        }
        m_done = true;
        emit failed(message);
        return;
    }

    if (object.contains("usage")) {
        m_result.usage = object.value("usage").toObject();
    }

    const QJsonArray choices = object.value("choices").toArray();
    for (const auto &choiceValue : choices) {
        const QJsonObject choice = choiceValue.toObject();
        const QJsonObject delta = choice.value("delta").toObject();
        const QString content = delta.value("content").toString();
        if (!content.isEmpty()) {
            m_result.text += content;
            emit textDelta(content);
        }
        const QString reasoning = delta.value("reasoning").toString();
        if (!reasoning.isEmpty()) {
            m_result.reasoning += reasoning;
            emit reasoningDelta(reasoning);
        }
        if (delta.contains("reasoning_details")) {
            const QJsonArray details = delta.value("reasoning_details").toArray();
            for (const QJsonValue &detail : details) {
                m_reasoningDetails.append(detail);
            }
        }
        if (delta.contains("tool_calls")) {
            const QJsonArray calls = delta.value("tool_calls").toArray();
            for (const QJsonValue &callValue : calls) {
                const QJsonObject deltaCall = callValue.toObject();
                const int index = deltaCall.value("index").toInt(m_toolCalls.size());
                QJsonObject call = m_toolCalls.value(index);
                if (deltaCall.contains("id")) {
                    call["id"] = deltaCall.value("id").toString();
                }
                call["type"] = deltaCall.value("type").toString(call.value("type").toString("function"));
                QJsonObject function = call.value("function").toObject();
                const QJsonObject deltaFunction = deltaCall.value("function").toObject();
                if (deltaFunction.contains("name")) {
                    function["name"] = deltaFunction.value("name").toString();
                }
                if (deltaFunction.contains("arguments")) {
                    function["arguments"] = function.value("arguments").toString()
                        + deltaFunction.value("arguments").toString();
                }
                call["function"] = function;
                m_toolCalls.insert(index, call);
            }
            emit toolEvent(delta);
        }
    }
}

void OpenaiChatAPI::finishFromReply()
{
    if (!m_reply) {
        return;
    }
    m_parser.finish();
    const QByteArray rest = m_reply->readAll();
    if (!rest.isEmpty()) {
        m_responseBody.append(rest);
        if (m_responseBody.size() > 4096) {
            m_responseBody.truncate(4096);
        }
        m_parser.feed(rest);
        m_parser.finish();
    }

    if (m_reply->error() != QNetworkReply::NoError && !m_done) {
        emit failed(networkErrorText(m_reply, m_responseBody));
        m_reply->deleteLater();
        return;
    }

    completeIfNeeded();
    m_reply->deleteLater();
}

void OpenaiChatAPI::completeIfNeeded()
{
    if (m_done) {
        return;
    }
    if (!m_toolCalls.isEmpty()) {
        QJsonArray calls;
        for (auto it = m_toolCalls.cbegin(); it != m_toolCalls.cend(); ++it) {
            calls.append(it.value());
        }
        QJsonObject assistant{
            {"role", "assistant"},
            {"content", m_result.text},
            {"tool_calls", calls},
        };
        if (!m_reasoningDetails.isEmpty()) {
            assistant["reasoning_details"] = m_reasoningDetails;
        }
        if (!m_result.reasoning.isEmpty()) {
            assistant["reasoning"] = m_result.reasoning;
        }
        m_result.rawOutputItems.append(assistant);
    }
    m_done = true;
    emit completed(m_result);
}

} // namespace MyChatty
