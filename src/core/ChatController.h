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
    Q_PROPERTY(QVariantList providerOptions READ providerOptions CONSTANT)
    Q_PROPERTY(QVariantList modelOptions READ modelOptions NOTIFY selectedProviderChanged)
    Q_PROPERTY(QVariantList effortOptions READ effortOptions NOTIFY effortOptionsChanged)
    Q_PROPERTY(QString selectedProvider READ selectedProvider WRITE setSelectedProvider NOTIFY selectedProviderChanged)
    Q_PROPERTY(QString selectedModel READ selectedModel WRITE setSelectedModel NOTIFY selectedModelChanged)
    Q_PROPERTY(QString selectedEffort READ selectedEffort WRITE setSelectedEffort NOTIFY selectedEffortChanged)
    Q_PROPERTY(QString selectedReasoningMode READ selectedReasoningMode WRITE setSelectedReasoningMode NOTIFY selectedReasoningModeChanged)
    Q_PROPERTY(bool supportsProReasoning READ supportsProReasoning NOTIFY selectedModelChanged)
    Q_PROPERTY(QString selectorLabel READ selectorLabel NOTIFY selectorLabelChanged)
    Q_PROPERTY(QString selectedMenuTitle READ selectedMenuTitle NOTIFY selectedModelChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString toast READ toast NOTIFY toastChanged)
public:
    explicit ChatController(SettingsStore *settings, QObject *parent = nullptr);

    QAbstractItemModel *messages();
    QVariantList pendingAttachments() const;
    QVariantList recents() const;
    QVariantList providerOptions() const;
    QVariantList modelOptions() const;
    QVariantList effortOptions() const;
    QString selectedProvider() const;
    QString selectedModel() const;
    QString selectedEffort() const;
    QString selectedReasoningMode() const;
    bool supportsProReasoning() const;
    QString selectorLabel() const;
    QString selectedMenuTitle() const;
    bool busy() const;
    QString toast() const;

    void setSelectedProvider(const QString &value);
    void setSelectedModel(const QString &value);
    void setSelectedEffort(const QString &value);
    void setSelectedReasoningMode(const QString &value);

    Q_INVOKABLE void sendMessage(const QString &text);
    Q_INVOKABLE void attachFiles(const QVariant &urls, const QString &origin);
    Q_INVOKABLE void removePendingAttachment(const QString &id);
    Q_INVOKABLE void copyMessage(int row);
    Q_INVOKABLE void amendUserMessage(int row, const QString &text);
    Q_INVOKABLE void forkConversation(int row);
    Q_INVOKABLE void shareMessage(int row);
    Q_INVOKABLE void readAloud(int row);
    Q_INVOKABLE void resolveToolApproval(int row, const QString &decision);
    Q_INVOKABLE void newChat();
    Q_INVOKABLE void loadConversation(const QString &id);
    Q_INVOKABLE void notifyNotImplemented(const QString &feature);
    Q_INVOKABLE void clearToast();
    Q_INVOKABLE void loadDemoConversation();

signals:
    void pendingAttachmentsChanged();
    void recentsChanged();
    void selectedProviderChanged();
    void selectedModelChanged();
    void selectedEffortChanged();
    void effortOptionsChanged();
    void selectedReasoningModeChanged();
    void selectorLabelChanged();
    void busyChanged();
    void toastChanged();

private:
    void setBusy(bool value);
    void normalizeSelectedEffort();
    void setToast(const QString &message);
    ChatRequest makeRequest() const;
    void appendAssistantAndBeginRequest();
    void beginRequest(const ChatRequest &request, int assistantRow, int toolDepth);
    bool continueAfterToolCalls(const ChatResult &result, ApiProvider provider, int toolDepth);
    bool requestWebApprovalIfNeeded(ApiProvider provider, int toolDepth, const QString &callId, const QString &name, const QJsonObject &arguments);
    void appendToolOutputAndContinue(ApiProvider provider, int toolDepth, const QString &callId, const QJsonObject &output);
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

    struct PendingToolApproval {
        bool active = false;
        QString approvalId;
        ApiProvider provider = ApiProvider::OpenRouterChat;
        int toolDepth = 0;
        QString callId;
        QString name;
        QJsonObject arguments;
    };

    SettingsStore *m_settings = nullptr;
    ChatMessageModel m_messages;
    QList<Attachment> m_pendingAttachments;
    QList<Conversation> m_conversations;
    QString m_currentConversationId;
    QString m_selectedProvider = "OpenRouter";
    QString m_selectedModel = "Gemma 4 Free";
    QString m_selectedEffort = "Medium";
    QString m_selectedReasoningMode = "Standard";
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
    int m_pendingCompletionToolDepth = 0;
    PendingToolApproval m_pendingToolApproval;
    QStringList m_alwaysApprovedWebHosts;
};

} // namespace MyChatty
