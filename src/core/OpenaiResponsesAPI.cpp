#include "OpenaiResponsesAPI.h"

#include <QJsonDocument>
#include <QNetworkReply>

namespace MyChatty {

OpenaiResponsesAPI::OpenaiResponsesAPI(QObject *parent)
    : ApiClient(parent)
{
    connect(&m_parser, &SseParser::eventReceived, this, &OpenaiResponsesAPI::handleEvent);
}

OpenaiResponsesAPI::OpenaiResponsesAPI(QNetworkAccessManager *network, QObject *parent)
    : ApiClient(network, parent)
{
    connect(&m_parser, &SseParser::eventReceived, this, &OpenaiResponsesAPI::handleEvent);
}

void OpenaiResponsesAPI::send(const ChatRequest &request)
{
    m_done = false;
    m_result = {};
    m_responseBody.clear();
    m_pendingError.clear();

    QNetworkRequest networkRequest = jsonRequest(QUrl("https://api.openai.com/v1/responses"), request.apiKey);
    const QByteArray body = QJsonDocument(buildOpenaiResponsesPayload(request)).toJson(QJsonDocument::Compact);
    m_reply = m_network->post(networkRequest, body);

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
    connect(m_reply, &QNetworkReply::finished, this, &OpenaiResponsesAPI::finishFromReply);
}

void OpenaiResponsesAPI::handleEvent(const QString &eventName, const QByteArray &data)
{
    if (data.trimmed() == "[DONE]") {
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        return;
    }
    const QJsonObject object = doc.object();
    m_result.rawEvents.append(object);

    const QString type = object.value("type").toString(eventName);
    if (type == "response.output_text.delta") {
        const QString delta = object.value("delta").toString();
        if (!delta.isEmpty()) {
            m_result.text += delta;
            emit textDelta(delta);
        }
    } else if (type == "response.reasoning_summary_text.delta"
               || type == "response.reasoning_text.delta") {
        const QString delta = object.value("delta").toString();
        if (!delta.isEmpty()) {
            m_result.reasoning += delta;
            emit reasoningDelta(delta);
        }
    } else if (type == "response.output_item.added" || type == "response.output_item.done") {
        const QJsonObject item = object.value("item").toObject();
        const QString itemType = item.value("type").toString();
        if (itemType == "reasoning") {
            const QJsonArray summary = item.value("summary").toArray();
            for (const auto &summaryItem : summary) {
                const QJsonObject summaryObject = summaryItem.toObject();
                if (summaryObject.value("type").toString() == "summary_text") {
                    appendReasoningSnapshot(summaryObject.value("text").toString());
                }
            }
        } else if (itemType.contains("call") || itemType.contains("tool")) {
            emit toolEvent(item);
        }
    } else if (type.contains("web_search_call") || type.contains("function_call") || type.contains("tool")) {
        emit toolEvent(object);
    } else if (type == "response.completed") {
        absorbCompletedResponse(object.value("response").toObject());
    } else if (type == "response.failed" || type == "error") {
        QJsonObject error = object.value("error").toObject();
        if (error.isEmpty()) {
            error = object.value("response").toObject().value("error").toObject();
        }
        const QString message = error.value("message").toString(object.value("message").toString());
        m_pendingError = message.isEmpty() ? QStringLiteral("OpenAI request failed") : message;
    }
}

void OpenaiResponsesAPI::finishFromReply()
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

    QNetworkReply *reply = m_reply;
    const QString networkError = reply->error() != QNetworkReply::NoError
        ? networkErrorText(reply, m_responseBody) : QString();
    reply->deleteLater();
    m_reply.clear();

    if (!m_pendingError.isEmpty() || !networkError.isEmpty()) {
        m_done = true;
        emit failed(!m_pendingError.isEmpty() ? m_pendingError : networkError);
        emit settled();
        return;
    }

    completeIfNeeded();
    emit settled();
}

void OpenaiResponsesAPI::completeIfNeeded()
{
    if (m_done) {
        return;
    }
    m_done = true;
    emit completed(m_result);
}

void OpenaiResponsesAPI::appendReasoningSnapshot(const QString &text)
{
    if (text.isEmpty() || m_result.reasoning.endsWith(text)) {
        return;
    }

    const QString delta = text.startsWith(m_result.reasoning)
        ? text.mid(m_result.reasoning.size())
        : text;
    if (delta.isEmpty()) {
        return;
    }
    m_result.reasoning += delta;
    emit reasoningDelta(delta);
}

void OpenaiResponsesAPI::absorbCompletedResponse(const QJsonObject &response)
{
    if (response.isEmpty()) {
        return;
    }
    m_result.rawResponse = response;
    m_result.rawOutputItems = response.value("output").toArray();
    m_result.usage = response.value("usage").toObject();

    if (m_result.text.isEmpty()) {
        for (const auto &itemValue : m_result.rawOutputItems) {
            const QJsonObject item = itemValue.toObject();
            if (item.value("type").toString() != "message") {
                continue;
            }
            const QJsonArray content = item.value("content").toArray();
            for (const auto &partValue : content) {
                const QJsonObject part = partValue.toObject();
                if (part.value("type").toString() == "output_text") {
                    const QString text = part.value("text").toString();
                    m_result.text += text;
                    emit textDelta(text);
                }
            }
        }
    }
}

} // namespace MyChatty
