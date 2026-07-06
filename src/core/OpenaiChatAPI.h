#pragma once

#include "ApiClient.h"
#include "SseParser.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>
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
    QElapsedTimer m_traceTimer;
    bool m_traceStream = false;
    int m_traceReadyReadCount = 0;
    int m_traceReasoningCount = 0;
    int m_traceTextCount = 0;
    int m_traceToolCount = 0;
    bool m_done = false;
};

} // namespace MyChatty
