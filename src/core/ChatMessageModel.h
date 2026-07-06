#pragma once

#include "ChatTypes.h"

#include <QAbstractListModel>

namespace MyChatty {

class ChatMessageModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        RoleNameRole,
        TextRole,
        HtmlRole,
        ReasoningRole,
        IsUserRole,
        IsAssistantRole,
        AttachmentsRole,
        TimestampRole,
        StreamingRole,
        BlocksRole,
        ToolCallsRole,
        ToolOnlyRole,
    };

    explicit ChatMessageModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    const QList<ChatMessage> &messages() const;
    QList<ChatMessage> &messages();
    void setMessages(const QList<ChatMessage> &messages);
    void append(const ChatMessage &message);
    void update(int row, const ChatMessage &message);
    void clear();

private:
    QList<ChatMessage> m_messages;
};

} // namespace MyChatty
