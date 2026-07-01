#include "ModelCatalog.h"

namespace MyChatty {

QList<ModelInfo> ModelCatalog::models()
{
    return {
        {"5.5", "GPT-5.5", "gpt-5.5", ApiProvider::OpenAIResponses, "OpenAI", {}},
        {"5.4-mini", "GPT-5.4 mini", "gpt-5.4-mini", ApiProvider::OpenAIResponses, "OpenAI", {}},
        {"5.5 Pro", "GPT-5.5 Pro", "gpt-5.5-pro", ApiProvider::OpenAIResponses, "OpenAI", {}},
        {"GLM-5.2", "GLM-5.2", "z-ai/glm-5.2", ApiProvider::OpenRouterChat, "OpenRouter", {"Parasail"}, false},
        {"Kimi K2.6", "Kimi K2.6", "moonshotai/kimi-k2.6", ApiProvider::OpenRouterChat, "OpenRouter", {"Moonshot AI"}},
        {"Gemma 4 Free", "Gemma 4 Free", "google/gemma-4-26b-a4b-it:free", ApiProvider::OpenRouterChat, "OpenRouter", {}},
    };
}

ModelInfo ModelCatalog::modelForDisplayName(const QString &displayName)
{
    for (const auto &model : models()) {
        if (model.displayName.compare(displayName, Qt::CaseInsensitive) == 0) {
            return model;
        }
    }
    return models().front();
}

ModelInfo ModelCatalog::modelForApiName(const QString &apiName)
{
    for (const auto &model : models()) {
        if (model.apiModel.compare(apiName, Qt::CaseInsensitive) == 0
            || model.displayName.compare(apiName, Qt::CaseInsensitive) == 0) {
            return model;
        }
    }

    ModelInfo custom;
    custom.displayName = apiName;
    custom.menuTitle = apiName;
    custom.apiModel = apiName;
    custom.provider = apiName.contains('/') ? ApiProvider::OpenRouterChat : ApiProvider::OpenAIResponses;
    custom.providerLabel = providerName(custom.provider);
    return custom;
}

QVariantList ModelCatalog::modelOptions()
{
    QVariantList result;
    for (const auto &model : models()) {
        QVariantMap row;
        row["displayName"] = model.displayName;
        row["menuTitle"] = model.menuTitle;
        row["apiModel"] = model.apiModel;
        row["provider"] = model.providerLabel;
        row["supportsImages"] = model.supportsImages;
        result.append(row);
    }
    return result;
}

QVariantList ModelCatalog::effortOptions()
{
    return {"Instant", "Medium", "High", "Extra High", "Pro"};
}

QString ModelCatalog::providerName(ApiProvider provider)
{
    switch (provider) {
    case ApiProvider::OpenAIResponses:
        return "OpenAI";
    case ApiProvider::OpenRouterChat:
        return "OpenRouter";
    }
    return "OpenAI";
}

QString ModelCatalog::openAIReasoningEffort(const QString &effort)
{
    const QString normalized = effort.trimmed().toLower();
    if (normalized == "instant") {
        return "low";
    }
    if (normalized == "high" || normalized == "extra high" || normalized == "pro") {
        return "high";
    }
    return "medium";
}

QString ModelCatalog::openRouterReasoningEffort(const QString &effort)
{
    const QString normalized = effort.trimmed().toLower();
    if (normalized == "instant") {
        return "low";
    }
    if (normalized == "extra high" || normalized == "pro") {
        return "xhigh";
    }
    if (normalized == "high") {
        return "high";
    }
    return "medium";
}

QString ModelCatalog::textVerbosity(const QString &effort)
{
    const QString normalized = effort.trimmed().toLower();
    if (normalized == "instant") {
        return "low";
    }
    if (normalized == "high" || normalized == "extra high" || normalized == "pro") {
        return "high";
    }
    return "medium";
}

} // namespace MyChatty
