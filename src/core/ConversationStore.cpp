#include "ConversationStore.h"

#include "ChatSerializer.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QStandardPaths>

namespace MyChatty {

static QJsonObject conversationToJson(const Conversation &conversation)
{
    QJsonArray messages;
    for (const ChatMessage &message : conversation.messages) {
        messages.append(ChatSerializer::messageToStoreJson(message));
    }
    return {
        {"id", conversation.id},
        {"title", conversation.title},
        {"model", conversation.model},
        {"effort", conversation.effort},
        {"createdAt", conversation.createdAt.toUTC().toString(Qt::ISODateWithMs)},
        {"updatedAt", conversation.updatedAt.toUTC().toString(Qt::ISODateWithMs)},
        {"messages", messages},
    };
}

static Conversation conversationFromJson(const QJsonObject &object)
{
    Conversation conversation;
    conversation.id = object.value("id").toString(newId("conv"));
    conversation.title = object.value("title").toString("New chat");
    conversation.model = object.value("model").toString("Gemma 4 Free");
    conversation.effort = object.value("effort").toString("Medium");
    conversation.createdAt = QDateTime::fromString(object.value("createdAt").toString(), Qt::ISODateWithMs);
    conversation.updatedAt = QDateTime::fromString(object.value("updatedAt").toString(), Qt::ISODateWithMs);
    if (!conversation.createdAt.isValid()) {
        conversation.createdAt = QDateTime::currentDateTimeUtc();
    }
    if (!conversation.updatedAt.isValid()) {
        conversation.updatedAt = conversation.createdAt;
    }
    for (const auto &messageValue : object.value("messages").toArray()) {
        conversation.messages.append(ChatSerializer::messageFromStoreJson(messageValue.toObject()));
    }
    return conversation;
}

QString ConversationStore::storagePath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/conversations.json";
}

QList<Conversation> ConversationStore::load()
{
    QFile file(storagePath());
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QList<Conversation> conversations;
    for (const auto &value : doc.object().value("conversations").toArray()) {
        conversations.append(conversationFromJson(value.toObject()));
    }
    std::sort(conversations.begin(), conversations.end(), [](const Conversation &a, const Conversation &b) {
        return a.updatedAt > b.updatedAt;
    });
    return conversations;
}

void ConversationStore::saveAll(const QList<Conversation> &conversations)
{
    QJsonArray array;
    for (const Conversation &conversation : conversations) {
        array.append(conversationToJson(conversation));
    }
    QFile file(storagePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(QJsonObject{{"conversations", array}}).toJson(QJsonDocument::Indented));
    }
}

void ConversationStore::upsert(const Conversation &conversation)
{
    QList<Conversation> conversations = load();
    bool replaced = false;
    for (Conversation &existing : conversations) {
        if (existing.id == conversation.id) {
            existing = conversation;
            replaced = true;
            break;
        }
    }
    if (!replaced) {
        conversations.prepend(conversation);
    }
    std::sort(conversations.begin(), conversations.end(), [](const Conversation &a, const Conversation &b) {
        return a.updatedAt > b.updatedAt;
    });
    while (conversations.size() > 100) {
        conversations.removeLast();
    }
    saveAll(conversations);
}

QVariantList ConversationStore::recentsAsVariant(const QList<Conversation> &conversations)
{
    QVariantList rows;
    for (const Conversation &conversation : conversations) {
        QVariantMap row;
        row["id"] = conversation.id;
        row["title"] = conversation.title;
        row["updatedAt"] = conversation.updatedAt.toLocalTime().toString("MMM d, h:mm");
        rows.append(row);
    }
    return rows;
}

} // namespace MyChatty
