#pragma once

#include "ChatTypes.h"

#include <QObject>
#include <QVariantList>

namespace MyChatty {

struct Conversation {
    QString id;
    QString title;
    QString provider;
    QString model;
    QString effort;
    QString reasoningMode = QStringLiteral("Standard");
    QDateTime createdAt;
    QDateTime updatedAt;
    QList<ChatMessage> messages;
};

class ConversationStore {
public:
    static QList<Conversation> load();
    static void saveAll(const QList<Conversation> &conversations);
    static void upsert(const Conversation &conversation);
    static QString storagePath();
    static QVariantList recentsAsVariant(const QList<Conversation> &conversations);
};

} // namespace MyChatty
