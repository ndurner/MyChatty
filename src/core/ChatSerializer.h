#pragma once

#include "ChatTypes.h"

#include <QJsonDocument>

namespace MyChatty {

class ChatSerializer {
public:
    static QJsonObject exportConversation(const QList<ChatMessage> &messages, const QString &systemPrompt);
    static QList<ChatMessage> importConversation(const QJsonObject &object, QString *systemPrompt = nullptr);
    static QJsonObject messageToStoreJson(const ChatMessage &message);
    static ChatMessage messageFromStoreJson(const QJsonObject &object);
};

} // namespace MyChatty
