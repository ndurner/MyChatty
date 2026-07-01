#pragma once

#include <QJsonArray>
#include <QString>
#include <QVariantList>

namespace MyChatty {

enum class ApiProvider {
    OpenAIResponses,
    OpenRouterChat
};

struct ModelInfo {
    QString displayName;
    QString menuTitle;
    QString apiModel;
    ApiProvider provider = ApiProvider::OpenAIResponses;
    QString providerLabel;
    QStringList providerOnly;
    bool supportsImages = true;
};

class ModelCatalog {
public:
    static QList<ModelInfo> models();
    static ModelInfo modelForDisplayName(const QString &displayName);
    static ModelInfo modelForApiName(const QString &apiName);
    static QVariantList modelOptions();
    static QVariantList effortOptions();
    static QString providerName(ApiProvider provider);
    static QString openAIReasoningEffort(const QString &effort);
    static QString openRouterReasoningEffort(const QString &effort);
    static QString textVerbosity(const QString &effort);
};

} // namespace MyChatty
