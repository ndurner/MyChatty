#pragma once

#include "ApiClient.h"
#include "SseParser.h"

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
    bool m_done = false;
};

} // namespace MyChatty
