#include "ChatMessageModel.h"

#include "MarkdownRenderer.h"

#include <QJsonDocument>

namespace MyChatty {
namespace {

QString compactJson(const QJsonValue &value)
{
    if (value.isUndefined() || value.isNull()) {
        return {};
    }
    if (value.isString()) {
        const QJsonDocument doc = QJsonDocument::fromJson(value.toString().toUtf8());
        if (doc.isObject() || doc.isArray()) {
            return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
        }
        return value.toString();
    }
    if (value.isObject()) {
        return QString::fromUtf8(QJsonDocument(value.toObject()).toJson(QJsonDocument::Compact));
    }
    if (value.isArray()) {
        return QString::fromUtf8(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
    }
    return value.toVariant().toString();
}

QString toolLabel(const QString &name)
{
    if (name == QStringLiteral("eval_javascript")) {
        return QStringLiteral("Code Interpreter");
    }
    if (name == QStringLiteral("open_web_page")) {
        return QStringLiteral("Open Web Page");
    }
    if (name == QStringLiteral("read_web_page_text")) {
        return QStringLiteral("Read Web Page");
    }
    if (name == QStringLiteral("get_next_web_page_screenshot")) {
        return QStringLiteral("Web Page Screenshot");
    }
    if (name == QStringLiteral("openrouter:web_search") || name == QStringLiteral("web_search")) {
        return QStringLiteral("Web Search");
    }
    return name.isEmpty() ? QStringLiteral("Tool") : name;
}

bool isToolOutputItem(const QJsonObject &item)
{
    return item.value("type").toString() == QStringLiteral("function_call_output")
        || item.value("role").toString() == QStringLiteral("tool");
}

bool isToolOnlyMessage(const ChatMessage &message)
{
    if (!message.isAssistant()
        || !message.text.trimmed().isEmpty()
        || !message.reasoning.trimmed().isEmpty()
        || message.rawOutputItems.isEmpty()
        || !message.attachments.isEmpty()) {
        return false;
    }
    for (const QJsonValue &itemValue : message.rawOutputItems) {
        if (!isToolOutputItem(itemValue.toObject())) {
            return false;
        }
    }
    return true;
}

QVariantList toolCallsForMessage(const QList<ChatMessage> &messages, int row)
{
    QVariantList rows;
    QHash<QString, int> rowForCallId;
    QJsonArray displayItems = messages.at(row).toolIndicators;
    for (const QJsonValue &item : messages.at(row).rawOutputItems) {
        displayItems.append(item);
    }
    for (int i = row + 1; i < messages.size() && isToolOnlyMessage(messages.at(i)); ++i) {
        for (const QJsonValue &item : messages.at(i).rawOutputItems) {
            displayItems.append(item);
        }
    }

    for (const QJsonValue &itemValue : displayItems) {
        const QJsonObject item = itemValue.toObject();
        if (item.value("type").toString() == QStringLiteral("server_tool_call")
            || item.value("type").toString() == QStringLiteral("web_search_call")) {
            const QString name = item.value("name").toString(QStringLiteral("web_search"));
            QVariantMap row;
            row["name"] = name;
            row["label"] = toolLabel(name);
            row["arguments"] = compactJson(item.value("arguments"));
            row["output"] = compactJson(item.value("output"));
            row["isOutput"] = false;
            row["hasOutput"] = !row.value("output").toString().isEmpty();
            row["callId"] = item.value("id").toString();
            rows.append(row);
            continue;
        }
        if (item.value("type").toString() == QStringLiteral("function_call")) {
            const QString name = item.value("name").toString();
            const QString callId = item.value("call_id").toString();
            QVariantMap row;
            row["name"] = name;
            row["label"] = toolLabel(name);
            row["arguments"] = compactJson(item.value("arguments"));
            row["output"] = {};
            row["isOutput"] = false;
            row["hasOutput"] = false;
            row["callId"] = callId;
            if (!callId.isEmpty()) {
                rowForCallId.insert(callId, rows.size());
            }
            rows.append(row);
        }

        const QJsonArray calls = item.value("tool_calls").toArray();
        for (const QJsonValue &callValue : calls) {
            const QJsonObject call = callValue.toObject();
            const QJsonObject function = call.value("function").toObject();
            const QString name = function.value("name").toString();
            const QString callId = call.value("id").toString();
            QVariantMap row;
            row["name"] = name;
            row["label"] = toolLabel(name);
            row["arguments"] = compactJson(function.value("arguments"));
            row["output"] = {};
            row["isOutput"] = false;
            row["hasOutput"] = false;
            row["callId"] = callId;
            if (!callId.isEmpty()) {
                rowForCallId.insert(callId, rows.size());
            }
            rows.append(row);
        }

        if (isToolOutputItem(item)) {
            const QString callId = item.value("call_id").toString(item.value("tool_call_id").toString());
            const QString output = compactJson(item.value("output").isUndefined() ? item.value("content") : item.value("output"));
            if (!callId.isEmpty() && rowForCallId.contains(callId)) {
                QVariantMap row = rows.at(rowForCallId.value(callId)).toMap();
                row["output"] = output;
                row["hasOutput"] = true;
                rows[rowForCallId.value(callId)] = row;
            } else {
                QVariantMap row;
                row["name"] = QStringLiteral("tool_output");
                row["label"] = QStringLiteral("Tool result");
                row["arguments"] = {};
                row["output"] = output;
                row["isOutput"] = true;
                row["hasOutput"] = true;
                row["callId"] = callId;
                rows.append(row);
            }
        }
    }
    return rows;
}

} // namespace

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
    case ToolCallsRole:
        return toolCallsForMessage(m_messages, index.row());
    case ToolOnlyRole:
        return isToolOnlyMessage(message);
    case ApprovalRole:
        return message.approval.toVariantMap();
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
        {ToolCallsRole, "toolCalls"},
        {ToolOnlyRole, "toolOnly"},
        {ApprovalRole, "approval"},
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
    if (isToolOnlyMessage(message)) {
        for (int previous = row - 1; previous >= 0; --previous) {
            if (!isToolOnlyMessage(m_messages.at(previous))) {
                const QModelIndex idx = index(previous);
                emit dataChanged(idx, idx, {ToolCallsRole});
                break;
            }
        }
    }
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

void ChatMessageModel::removeAfter(int row)
{
    if (row < -1 || row >= m_messages.size() - 1) {
        return;
    }
    const int first = row + 1;
    const int last = m_messages.size() - 1;
    beginRemoveRows({}, first, last);
    m_messages.erase(m_messages.begin() + first, m_messages.end());
    endRemoveRows();
}

void ChatMessageModel::clear()
{
    beginResetModel();
    m_messages.clear();
    endResetModel();
}

} // namespace MyChatty
