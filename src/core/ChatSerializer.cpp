#include "ChatSerializer.h"

#include <QJsonDocument>

namespace MyChatty {

static QJsonObject exportAttachmentPart(const Attachment &attachment)
{
    const QString data = attachment.dataUrl();
    if (attachment.isImage()) {
        return {
            {"type", "image_url"},
            {"image_url", QJsonObject{{"url", data}}},
            {"origin", attachment.origin},
        };
    }
    return {
        {"type", "file"},
        {"file", QJsonObject{
            {"url", data},
            {"name", attachment.fileName},
            {"mime_type", attachment.mimeType},
            {"origin", attachment.origin},
        }},
    };
}

QJsonObject ChatSerializer::exportConversation(const QList<ChatMessage> &messages, const QString &systemPrompt)
{
    QJsonArray exportedMessages;
    if (!systemPrompt.trimmed().isEmpty()) {
        exportedMessages.append(QJsonObject{
            {"role", "system"},
            {"content", systemPrompt.trimmed()},
        });
    }

    for (const ChatMessage &message : messages) {
        if (message.text.isEmpty() && message.attachments.isEmpty()) {
            continue;
        }

        if (message.isUser() && !message.attachments.isEmpty()) {
            QJsonArray content;
            if (!message.text.trimmed().isEmpty()) {
                content.append(QJsonObject{{"type", "text"}, {"text", message.text}});
            }
            for (const Attachment &attachment : message.attachments) {
                content.append(exportAttachmentPart(attachment));
            }
            exportedMessages.append(QJsonObject{{"role", message.role}, {"content", content}});
        } else {
            exportedMessages.append(QJsonObject{{"role", message.role}, {"content", message.text}});
        }
    }

    return QJsonObject{{"messages", exportedMessages}};
}

QList<ChatMessage> ChatSerializer::importConversation(const QJsonObject &object, QString *systemPrompt)
{
    QList<ChatMessage> messages;
    const QJsonArray exportedMessages = object.value("messages").toArray();
    for (const auto &value : exportedMessages) {
        const QJsonObject source = value.toObject();
        const QString role = source.value("role").toString();
        if (role == "system") {
            if (systemPrompt) {
                *systemPrompt = source.value("content").toString();
            }
            continue;
        }

        ChatMessage message;
        message.id = newId("msg");
        message.role = role;
        message.createdAt = QDateTime::currentDateTimeUtc();

        const QJsonValue content = source.value("content");
        if (content.isString()) {
            message.text = content.toString();
        } else if (content.isArray()) {
            QStringList textParts;
            for (const auto &partValue : content.toArray()) {
                const QJsonObject part = partValue.toObject();
                if (part.value("type").toString() == "text" || part.value("type").toString() == "input_text") {
                    textParts.append(part.value("text").toString());
                }
            }
            message.text = textParts.join("\n");
        }
        messages.append(message);
    }
    return messages;
}

QJsonObject ChatSerializer::messageToStoreJson(const ChatMessage &message)
{
    QJsonArray attachments;
    for (const Attachment &attachment : message.attachments) {
        attachments.append(attachment.toJsonMetadata());
    }
    return {
        {"id", message.id},
        {"role", message.role},
        {"text", message.text},
        {"createdAt", message.createdAt.toUTC().toString(Qt::ISODateWithMs)},
        {"reasoning", message.reasoning},
        {"approval", message.approval},
        {"attachments", attachments},
        {"rawOutputItems", message.rawOutputItems},
        {"toolIndicators", message.toolIndicators},
        {"rawResponse", message.rawResponse},
    };
}

ChatMessage ChatSerializer::messageFromStoreJson(const QJsonObject &object)
{
    ChatMessage message;
    message.id = object.value("id").toString(newId("msg"));
    message.role = object.value("role").toString();
    message.text = object.value("text").toString();
    message.createdAt = QDateTime::fromString(object.value("createdAt").toString(), Qt::ISODateWithMs);
    if (!message.createdAt.isValid()) {
        message.createdAt = QDateTime::currentDateTimeUtc();
    }
    message.reasoning = object.value("reasoning").toString();
    message.approval = object.value("approval").toObject();
    message.rawOutputItems = object.value("rawOutputItems").toArray();
    message.toolIndicators = object.value("toolIndicators").toArray();
    message.rawResponse = object.value("rawResponse").toObject();

    for (const auto &attachmentValue : object.value("attachments").toArray()) {
        const QJsonObject source = attachmentValue.toObject();
        Attachment attachment;
        attachment.id = source.value("id").toString(newId("att"));
        attachment.url = QUrl(source.value("url").toString());
        attachment.localPath = source.value("path").toString();
        attachment.fileName = source.value("fileName").toString();
        attachment.mimeType = source.value("mimeType").toString();
        attachment.origin = source.value("origin").toString();
        message.attachments.append(attachment);
    }
    return message;
}

} // namespace MyChatty
