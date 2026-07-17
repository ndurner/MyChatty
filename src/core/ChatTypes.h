#pragma once

#include "ModelCatalog.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMetaType>
#include <QMimeType>
#include <QUrl>

namespace MyChatty {

struct Attachment {
    QString id;
    QUrl url;
    QString localPath;
    QString fileName;
    QString mimeType;
    QString origin;

    bool isImage() const;
    bool isTextLike() const;
    QByteArray readBytes(qint64 limit = 20 * 1024 * 1024) const;
    QString readText(qint64 limit = 1024 * 1024) const;
    QString dataUrl(qint64 limit = 20 * 1024 * 1024) const;
    QVariantMap toVariant() const;
    QJsonObject toJsonMetadata() const;

    static Attachment fromUrl(const QUrl &url, const QString &origin);
};

struct ChatMessage {
    QString id;
    QString role;
    QString text;
    QList<Attachment> attachments;
    QDateTime createdAt;
    QString reasoning;
    QJsonObject approval;
    QJsonArray rawOutputItems;
    QJsonArray toolIndicators;
    QJsonObject rawResponse;
    bool streaming = false;

    bool isUser() const { return role == "user"; }
    bool isAssistant() const { return role == "assistant"; }
};

struct ChatRequest {
    QList<ChatMessage> history;
    ModelInfo model;
    QString effort;
    QString reasoningMode = QStringLiteral("Standard");
    QString customInstructions;
    QString apiKey;
    QString exaApiKey;
    QString openRouterPdfEngine = QStringLiteral("cloudflare-ai");
    bool enableWebSearch = false;
    bool useExaSearch = false;
    bool enableJavaScriptUse = false;
    bool enableWebBrowser = false;
    bool enablePageScreenshots = false;
    int maxOutputTokens = 0;
    bool stream = true;
};

struct ChatResult {
    QString text;
    QString reasoning;
    QJsonArray rawOutputItems;
    QJsonObject rawResponse;
    QJsonArray rawEvents;
    QJsonObject usage;
};

struct ToolCall {
    QString id;
    QString name;
    QJsonObject arguments;
};

} // namespace MyChatty

Q_DECLARE_METATYPE(MyChatty::ChatResult)

namespace MyChatty {

QString newId(const QString &prefix);
QString guessMimeType(const QString &path);
QString fileNameFromUrl(const QUrl &url);

QJsonObject buildOpenaiResponsesPayload(const ChatRequest &request);
QJsonObject buildOpenaiChatPayload(const ChatRequest &request);

} // namespace MyChatty
