#include "ChatController.h"

#include "ChatSerializer.h"
#include "OpenaiChatAPI.h"
#include "OpenaiResponsesAPI.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QStandardPaths>

namespace MyChatty {

static QString trimmedTitle(QString title)
{
    title = title.simplified();
    if (title.size() > 48) {
        title = title.left(45) + "...";
    }
    return title;
}

static QString titleFromReasoning(const QString &reasoning)
{
    for (QString line : reasoning.split('\n')) {
        line = line.trimmed();
        if (line.isEmpty() || line == "Thought" || line.startsWith("Thought for ")) {
            continue;
        }
        while (line.startsWith('#')) {
            line = line.mid(1).trimmed();
        }
        while ((line.startsWith("**") && line.endsWith("**") && line.size() > 4)
               || (line.startsWith("__") && line.endsWith("__") && line.size() > 4)) {
            line = line.mid(2, line.size() - 4).trimmed();
        }
        if (!line.isEmpty()) {
            return trimmedTitle(line);
        }
    }
    return {};
}

ChatController::ChatController(SettingsStore *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    refreshRecents();
    connect(&m_speech, &SpeechService::ready, this, [this](const QString &) {
        setToast(QStringLiteral("Audio opened."));
    });
    connect(&m_speech, &SpeechService::failed, this, [this](const QString &message) {
        setToast(message);
    });
}

QAbstractItemModel *ChatController::messages()
{
    return &m_messages;
}

QVariantList ChatController::pendingAttachments() const
{
    QVariantList rows;
    for (const Attachment &attachment : m_pendingAttachments) {
        rows.append(attachment.toVariant());
    }
    return rows;
}

QVariantList ChatController::recents() const
{
    return ConversationStore::recentsAsVariant(m_conversations);
}

QVariantList ChatController::modelOptions() const
{
    return ModelCatalog::modelOptions();
}

QVariantList ChatController::effortOptions() const
{
    return ModelCatalog::effortOptions();
}

QString ChatController::selectedModel() const
{
    return m_selectedModel;
}

QString ChatController::selectedEffort() const
{
    return m_selectedEffort;
}

QString ChatController::selectorLabel() const
{
    return QStringLiteral("%1 %2").arg(m_selectedModel, m_selectedEffort);
}

QString ChatController::selectedMenuTitle() const
{
    return ModelCatalog::modelForDisplayName(m_selectedModel).menuTitle;
}

bool ChatController::busy() const
{
    return m_busy;
}

QString ChatController::toast() const
{
    return m_toast;
}

void ChatController::setSelectedModel(const QString &value)
{
    if (m_selectedModel == value) {
        return;
    }
    m_selectedModel = value;
    emit selectedModelChanged();
}

void ChatController::setSelectedEffort(const QString &value)
{
    if (m_selectedEffort == value) {
        return;
    }
    m_selectedEffort = value;
    emit selectedEffortChanged();
}

void ChatController::sendMessage(const QString &text)
{
    if (m_busy) {
        return;
    }
    if (text.trimmed().isEmpty() && m_pendingAttachments.isEmpty()) {
        return;
    }

    ChatMessage user;
    user.id = newId("msg");
    user.role = "user";
    user.text = text.trimmed();
    user.attachments = m_pendingAttachments;
    user.createdAt = QDateTime::currentDateTimeUtc();
    m_pendingAttachments.clear();
    emit pendingAttachmentsChanged();
    m_messages.append(user);

    ChatRequest request;
    request.history = m_messages.messages();
    request.model = ModelCatalog::modelForDisplayName(m_selectedModel);
    request.effort = m_selectedEffort;
    request.customInstructions = m_settings ? m_settings->customInstructions() : QString();
    request.apiKey = apiKeyForModel(request.model);
    request.stream = true;

    ChatMessage assistant;
    assistant.id = newId("msg");
    assistant.role = "assistant";
    assistant.createdAt = QDateTime::currentDateTimeUtc();
    assistant.streaming = true;
    m_messages.append(assistant);
    const int assistantRow = m_messages.rowCount() - 1;

    if (request.apiKey.trimmed().isEmpty()) {
        assistant.text = QStringLiteral("Missing %1 API key. Add it in Settings.")
                             .arg(ModelCatalog::providerName(request.model.provider));
        assistant.streaming = false;
        m_messages.update(assistantRow, assistant);
        persistCurrentConversation();
        return;
    }

    setBusy(true);
    if (request.model.provider == ApiProvider::OpenRouterChat) {
        m_client = std::make_unique<OpenaiChatAPI>();
    } else {
        m_client = std::make_unique<OpenaiResponsesAPI>();
    }

    connect(m_client.get(), &ApiClient::textDelta, this, [this, assistantRow](const QString &delta) {
        ChatMessage *message = assistantMessageAt(assistantRow);
        if (!message) {
            return;
        }
        message->text += delta;
        m_messages.update(assistantRow, *message);
    });
    connect(m_client.get(), &ApiClient::reasoningDelta, this, [this, assistantRow](const QString &delta) {
        ChatMessage *message = assistantMessageAt(assistantRow);
        if (!message) {
            return;
        }
        message->reasoning += delta;
        m_messages.update(assistantRow, *message);
    });
    connect(m_client.get(), &ApiClient::toolEvent, this, [this](const QJsonObject &event) {
        const QString type = event.value("type").toString("tool event");
        setToast(QStringLiteral("Tool event: %1").arg(type));
    });
    connect(m_client.get(), &ApiClient::completed, this, [this, assistantRow](const ChatResult &result) {
        ChatMessage *message = assistantMessageAt(assistantRow);
        if (message) {
            if (message->text.isEmpty()) {
                message->text = result.text;
            }
            if (message->reasoning.isEmpty()) {
                message->reasoning = result.reasoning;
            }
            message->rawOutputItems = result.rawOutputItems;
            message->rawResponse = result.rawResponse;
            message->streaming = false;
            m_messages.update(assistantRow, *message);
        }
        setBusy(false);
        persistCurrentConversation();
        QObject *finishedClient = m_client.release();
        if (finishedClient) {
            finishedClient->deleteLater();
        }
    });
    connect(m_client.get(), &ApiClient::failed, this, [this, assistantRow](const QString &messageText) {
        ChatMessage *message = assistantMessageAt(assistantRow);
        if (message) {
            message->text = QStringLiteral("Error: %1").arg(messageText);
            message->streaming = false;
            m_messages.update(assistantRow, *message);
        }
        setBusy(false);
        persistCurrentConversation();
        QObject *finishedClient = m_client.release();
        if (finishedClient) {
            finishedClient->deleteLater();
        }
    });

    m_client->send(request);
}

void ChatController::attachFiles(const QVariant &urls, const QString &origin)
{
    const QVariantList list = urls.toList();
    for (const QVariant &value : list) {
        const QUrl url = value.toUrl();
        if (url.isEmpty()) {
            continue;
        }
        m_pendingAttachments.append(Attachment::fromUrl(url, origin));
    }
    emit pendingAttachmentsChanged();
}

void ChatController::removePendingAttachment(const QString &id)
{
    for (int i = 0; i < m_pendingAttachments.size(); ++i) {
        if (m_pendingAttachments.at(i).id == id) {
            m_pendingAttachments.removeAt(i);
            emit pendingAttachmentsChanged();
            return;
        }
    }
}

void ChatController::copyMessage(int row)
{
    if (row < 0 || row >= m_messages.messages().size()) {
        return;
    }
    QGuiApplication::clipboard()->setText(m_messages.messages().at(row).text);
    setToast(QStringLiteral("Copied."));
}

void ChatController::shareMessage(int)
{
    const QJsonObject exported = ChatSerializer::exportConversation(
        m_messages.messages(),
        m_settings ? m_settings->customInstructions() : QString());
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString fileName = QStringLiteral("mychatty-chat-%1.json")
                                 .arg(QDateTime::currentDateTimeUtc().toString("yyyyMMdd-hhmmss"));
    const QString path = QDir(dir).filePath(fileName);
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setToast(QStringLiteral("Could not write chat JSON."));
        return;
    }
    file.write(QJsonDocument(exported).toJson(QJsonDocument::Indented));
    file.close();
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    setToast(QStringLiteral("Shared JSON: %1").arg(fileName));
}

void ChatController::readAloud(int row)
{
    if (row < 0 || row >= m_messages.messages().size()) {
        return;
    }
    const ChatMessage &message = m_messages.messages().at(row);
    m_speech.speak(message.text, m_settings ? m_settings->openAIKey() : QString());
}

void ChatController::newChat()
{
    if (m_busy && m_client) {
        m_client->cancel();
    }
    setBusy(false);
    m_currentConversationId.clear();
    m_messages.clear();
    m_pendingAttachments.clear();
    emit pendingAttachmentsChanged();
}

void ChatController::loadConversation(const QString &id)
{
    for (const Conversation &conversation : m_conversations) {
        if (conversation.id == id) {
            if (m_busy && m_client) {
                m_client->cancel();
                setBusy(false);
            }
            m_currentConversationId = conversation.id;
            m_selectedModel = conversation.model;
            m_selectedEffort = conversation.effort;
            m_messages.setMessages(conversation.messages);
            emit selectedModelChanged();
            emit selectedEffortChanged();
            return;
        }
    }
}

void ChatController::notifyNotImplemented(const QString &feature)
{
    Q_UNUSED(feature);
    setToast(QStringLiteral("Not implemented"));
}

void ChatController::clearToast()
{
    setToast(QString());
}

void ChatController::loadDemoConversation()
{
    if (m_busy && m_client) {
        m_client->cancel();
    }
    setBusy(false);
    m_currentConversationId.clear();

    ChatMessage user;
    user.id = newId("msg");
    user.role = "user";
    user.text = "Show me a compact Markdown answer with bold text and a code fence.";
    user.createdAt = QDateTime::currentDateTimeUtc().addSecs(-90);

    ChatMessage assistant;
    assistant.id = newId("msg");
    assistant.role = "assistant";
    assistant.createdAt = QDateTime::currentDateTimeUtc().addSecs(-70);
    assistant.reasoning = "Thought for 4s";
    assistant.text = QStringLiteral(
        "Here is a **compact Markdown response** with a fenced code block:\n\n"
        "```cpp\n"
        "QString greeting = \"Hello from MyChatty\";\n"
        "qDebug() << greeting;\n"
        "```\n\n"
        "The action row below supports copy, read-aloud, and JSON sharing.");

    m_messages.setMessages({user, assistant});
}

void ChatController::setBusy(bool value)
{
    if (m_busy == value) {
        return;
    }
    m_busy = value;
    emit busyChanged();
}

void ChatController::setToast(const QString &message)
{
    if (m_toast == message) {
        return;
    }
    m_toast = message;
    emit toastChanged();
}

void ChatController::persistCurrentConversation()
{
    if (m_messages.messages().isEmpty()) {
        return;
    }
    Conversation conversation;
    conversation.id = m_currentConversationId.isEmpty() ? newId("conv") : m_currentConversationId;
    conversation.title = titleForConversation();
    conversation.model = m_selectedModel;
    conversation.effort = m_selectedEffort;
    conversation.createdAt = QDateTime::currentDateTimeUtc();
    conversation.updatedAt = QDateTime::currentDateTimeUtc();
    conversation.messages = m_messages.messages();
    m_currentConversationId = conversation.id;
    ConversationStore::upsert(conversation);
    refreshRecents();
}

void ChatController::refreshRecents()
{
    m_conversations = ConversationStore::load();
    emit recentsChanged();
}

QString ChatController::apiKeyForModel(const ModelInfo &model) const
{
    if (!m_settings) {
        return {};
    }
    if (model.provider == ApiProvider::OpenRouterChat) {
        return m_settings->openRouterKey();
    }
    return m_settings->openAIKey();
}

QString ChatController::titleForConversation() const
{
    for (const ChatMessage &message : m_messages.messages()) {
        if (message.isAssistant()) {
            const QString title = titleFromReasoning(message.reasoning);
            if (!title.isEmpty()) {
                return title;
            }
        }
    }

    for (const ChatMessage &message : m_messages.messages()) {
        if (message.isUser() && !message.text.trimmed().isEmpty()) {
            return trimmedTitle(message.text);
        }
    }
    return QStringLiteral("New chat");
}

ChatMessage *ChatController::assistantMessageAt(int row)
{
    QList<ChatMessage> &messages = m_messages.messages();
    if (row < 0 || row >= messages.size() || !messages[row].isAssistant()) {
        return nullptr;
    }
    return &messages[row];
}

} // namespace MyChatty
