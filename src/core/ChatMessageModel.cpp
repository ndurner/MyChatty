#include "ChatMessageModel.h"

#include "MarkdownRenderer.h"

namespace MyChatty {

ChatMessageModel::ChatMessageModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ChatMessageModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_messages.size();
}

QVariant ChatMessageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.size()) {
        return {};
    }
    const ChatMessage &message = m_messages.at(index.row());
    switch (role) {
    case IdRole:
        return message.id;
    case RoleNameRole:
        return message.role;
    case TextRole:
        return message.text;
    case HtmlRole:
        return MarkdownRenderer::render(message.text);
    case BlocksRole:
        return MarkdownRenderer::renderBlocks(message.text);
    case ReasoningRole:
        return message.reasoning;
    case IsUserRole:
        return message.isUser();
    case IsAssistantRole:
        return message.isAssistant();
    case AttachmentsRole: {
        QVariantList rows;
        for (const Attachment &attachment : message.attachments) {
            rows.append(attachment.toVariant());
        }
        return rows;
    }
    case TimestampRole:
        return message.createdAt.toLocalTime().toString("dddd h:mm");
    case StreamingRole:
        return message.streaming;
    default:
        return {};
    }
}

QHash<int, QByteArray> ChatMessageModel::roleNames() const
{
    return {
        {IdRole, "messageId"},
        {RoleNameRole, "roleName"},
        {TextRole, "text"},
        {HtmlRole, "html"},
        {ReasoningRole, "reasoning"},
        {IsUserRole, "isUser"},
        {IsAssistantRole, "isAssistant"},
        {AttachmentsRole, "attachments"},
        {TimestampRole, "timestamp"},
        {StreamingRole, "streaming"},
        {BlocksRole, "blocks"},
    };
}

const QList<ChatMessage> &ChatMessageModel::messages() const
{
    return m_messages;
}

QList<ChatMessage> &ChatMessageModel::messages()
{
    return m_messages;
}

void ChatMessageModel::setMessages(const QList<ChatMessage> &messages)
{
    beginResetModel();
    m_messages = messages;
    endResetModel();
}

void ChatMessageModel::append(const ChatMessage &message)
{
    const int row = m_messages.size();
    beginInsertRows({}, row, row);
    m_messages.append(message);
    endInsertRows();
}

void ChatMessageModel::update(int row, const ChatMessage &message)
{
    if (row < 0 || row >= m_messages.size()) {
        return;
    }
    m_messages[row] = message;
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx);
}

void ChatMessageModel::clear()
{
    beginResetModel();
    m_messages.clear();
    endResetModel();
}

} // namespace MyChatty
