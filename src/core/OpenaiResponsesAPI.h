#pragma once

#include "ApiClient.h"
#include "SseParser.h"

namespace MyChatty {

class OpenaiResponsesAPI : public ApiClient {
    Q_OBJECT
public:
    explicit OpenaiResponsesAPI(QObject *parent = nullptr);

    void send(const ChatRequest &request) override;

private:
    void handleEvent(const QString &eventName, const QByteArray &data);
    void finishFromReply();
    void completeIfNeeded();
    void absorbCompletedResponse(const QJsonObject &response);
    void appendReasoningSnapshot(const QString &text);

    SseParser m_parser;
    ChatResult m_result;
    bool m_done = false;
};

} // namespace MyChatty
