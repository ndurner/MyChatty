#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVariantList>

namespace MyChatty {

enum class ApiProvider {
    OpenAIResponses,
    OpenRouterChat,
    NvidiaChat
};

struct ModelInfo {
    QString displayName;
    QString menuTitle;
    QString apiModel;
    ApiProvider provider = ApiProvider::OpenAIResponses;
    QString providerLabel;
    QStringList providerOnly;
    bool supportsImages = true;
    bool supportsFiles = false;
    QJsonObject chatParameters;
};

class ModelCatalog {
public:
    static QList<ModelInfo> models();
    static ModelInfo modelForDisplayName(const QString &displayName);
    static ModelInfo modelForProviderAndDisplayName(const QString &provider, const QString &displayName);
    static ModelInfo modelForApiName(const QString &apiName);
    static QVariantList providerOptions();
    static QVariantList modelOptions();
    static QVariantList modelOptionsForProvider(const QString &provider);
    static QVariantList effortOptions();
    static QString providerName(ApiProvider provider);
    static QString openAIReasoningEffort(const QString &effort);
    static QString openRouterReasoningEffort(const QString &effort);
    static QString nvidiaReasoningEffort(const QString &effort);
    static QString textVerbosity(const QString &effort);
};

} // namespace MyChatty
