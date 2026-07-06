#pragma once

#include "ApiClient.h"
#include "SseParser.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QMap>

namespace MyChatty {

class OpenaiChatAPI : public ApiClient {
    Q_OBJECT
public:
    explicit OpenaiChatAPI(QObject *parent = nullptr);

    void send(const ChatRequest &request) override;

private:
    void handleEvent(const QString &eventName, const QByteArray &data);
    void finishFromReply();
    void completeIfNeeded();

    SseParser m_parser;
    ChatResult m_result;
    QByteArray m_responseBody;
    QJsonArray m_reasoningDetails;
    QMap<int, QJsonObject> m_toolCalls;
    bool m_done = false;
};

} // namespace MyChatty
