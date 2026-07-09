#pragma once

#include "ApiClient.h"
#include "SseParser.h"

namespace MyChatty {

class OpenaiResponsesAPI : public ApiClient {
    Q_OBJECT
public:
    explicit OpenaiResponsesAPI(QObject *parent = nullptr);

    void send(const ChatRequest &request) override;

private slots:
    void handleEvent(const QString &eventName, const QByteArray &data);
    void finishFromReply();

private:
    void completeIfNeeded();
    void absorbCompletedResponse(const QJsonObject &response);
    void appendReasoningSnapshot(const QString &text);

    SseParser m_parser;
    ChatResult m_result;
    QByteArray m_responseBody;
    bool m_done = false;
};

} // namespace MyChatty
