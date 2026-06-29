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

    QNetworkRequest networkRequest = jsonRequest(QUrl("https://openrouter.ai/api/v1/chat/completions"), request.apiKey);
    networkRequest.setRawHeader("HTTP-Referer", "https://github.com/ndurner/MyChatty");
    networkRequest.setRawHeader("X-Title", "MyChatty");

    const QByteArray body = QJsonDocument(buildOpenaiChatPayload(request)).toJson(QJsonDocument::Compact);
    m_reply = m_network.post(networkRequest, body);

    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        if (!m_reply) {
            return;
        }
        m_parser.feed(m_reply->readAll());
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
        m_done = true;
        emit failed(error.value("message").toString(QStringLiteral("OpenRouter request failed")));
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
        if (delta.contains("tool_calls")) {
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
        m_parser.feed(rest);
        m_parser.finish();
    }

    if (m_reply->error() != QNetworkReply::NoError && !m_done) {
        emit failed(networkErrorText(m_reply, rest));
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
    m_done = true;
    emit completed(m_result);
}

} // namespace MyChatty
