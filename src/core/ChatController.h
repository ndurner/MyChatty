#pragma once

#include "ApiClient.h"
#include "ChatMessageModel.h"
#include "ConversationStore.h"
#include "SettingsStore.h"
#include "SpeechService.h"

#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <memory>

namespace MyChatty {

class ChatController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *messages READ messages CONSTANT)
    Q_PROPERTY(QVariantList pendingAttachments READ pendingAttachments NOTIFY pendingAttachmentsChanged)
    Q_PROPERTY(QVariantList recents READ recents NOTIFY recentsChanged)
    Q_PROPERTY(QVariantList modelOptions READ modelOptions CONSTANT)
    Q_PROPERTY(QVariantList effortOptions READ effortOptions CONSTANT)
    Q_PROPERTY(QString selectedModel READ selectedModel WRITE setSelectedModel NOTIFY selectedModelChanged)
    Q_PROPERTY(QString selectedEffort READ selectedEffort WRITE setSelectedEffort NOTIFY selectedEffortChanged)
    Q_PROPERTY(QString selectorLabel READ selectorLabel NOTIFY selectedModelChanged)
    Q_PROPERTY(QString selectedMenuTitle READ selectedMenuTitle NOTIFY selectedModelChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString toast READ toast NOTIFY toastChanged)
public:
    explicit ChatController(SettingsStore *settings, QObject *parent = nullptr);

    QAbstractItemModel *messages();
    QVariantList pendingAttachments() const;
    QVariantList recents() const;
    QVariantList modelOptions() const;
    QVariantList effortOptions() const;
    QString selectedModel() const;
    QString selectedEffort() const;
    QString selectorLabel() const;
    QString selectedMenuTitle() const;
    bool busy() const;
    QString toast() const;

    void setSelectedModel(const QString &value);
    void setSelectedEffort(const QString &value);

    Q_INVOKABLE void sendMessage(const QString &text);
    Q_INVOKABLE void attachFiles(const QVariant &urls, const QString &origin);
    Q_INVOKABLE void removePendingAttachment(const QString &id);
    Q_INVOKABLE void copyMessage(int row);
    Q_INVOKABLE void shareMessage(int row);
    Q_INVOKABLE void readAloud(int row);
    Q_INVOKABLE void newChat();
    Q_INVOKABLE void loadConversation(const QString &id);
    Q_INVOKABLE void notifyNotImplemented(const QString &feature);
    Q_INVOKABLE void clearToast();
    Q_INVOKABLE void loadDemoConversation();

signals:
    void pendingAttachmentsChanged();
    void recentsChanged();
    void selectedModelChanged();
    void selectedEffortChanged();
    void busyChanged();
    void toastChanged();

private:
    void setBusy(bool value);
    void setToast(const QString &message);
    ChatRequest makeRequest() const;
    void beginRequest(const ChatRequest &request, int assistantRow, int toolDepth);
    bool continueAfterToolCalls(const ChatResult &result, ApiProvider provider, int toolDepth);
    void queueTextDelta(int assistantRow, const QString &delta);
    void queueReasoningDelta(int assistantRow, const QString &delta);
    void flushStreamDeltas();
    void completePendingRequestIfReady();
    void resetStreamBuffer();
    void ensureCurrentConversationId();
    void importAttachmentsToWorkspace(const QList<Attachment> &attachments);
    void persistCurrentConversation();
    void refreshRecents();
    QString apiKeyForModel(const ModelInfo &model) const;
    QString titleForConversation() const;
    ChatMessage *assistantMessageAt(int row);

    SettingsStore *m_settings = nullptr;
    ChatMessageModel m_messages;
    QList<Attachment> m_pendingAttachments;
    QList<Conversation> m_conversations;
    QString m_currentConversationId;
    QString m_selectedModel = "Gemma 4 Free";
    QString m_selectedEffort = "Medium";
    bool m_busy = false;
    QString m_toast;
    std::unique_ptr<ApiClient> m_client;
    SpeechService m_speech;

    QTimer m_streamFlushTimer;
    int m_streamAssistantRow = -1;
    QString m_pendingTextDelta;
    QString m_pendingReasoningDelta;

    bool m_hasPendingCompletion = false;
    ChatResult m_pendingCompletionResult;
    ApiProvider m_pendingCompletionProvider = ApiProvider::OpenRouterChat;
    bool m_pendingCompletionWebSearchEnabled = false;
    int m_pendingCompletionToolDepth = 0;
};

} // namespace MyChatty
