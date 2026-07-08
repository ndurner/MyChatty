#include "ModelCatalog.h"

namespace MyChatty {

QList<ModelInfo> ModelCatalog::models()
{
    return {
        {"5.5", "GPT-5.5", "gpt-5.5", ApiProvider::OpenAIResponses, "OpenAI", {}, true, true},
        {"5.4-mini", "GPT-5.4 mini", "gpt-5.4-mini", ApiProvider::OpenAIResponses, "OpenAI", {}, true, true},
        {"5.5 Pro", "GPT-5.5 Pro", "gpt-5.5-pro", ApiProvider::OpenAIResponses, "OpenAI", {}, true, true},
        {"5.5", "GPT-5.5", "openai/gpt-5.5", ApiProvider::OpenRouterChat, "OpenRouter", {}, true, false},
        {"5.4-mini", "GPT-5.4 mini", "openai/gpt-5.4-mini", ApiProvider::OpenRouterChat, "OpenRouter", {}, true, false},
        {"5.5 Pro", "GPT-5.5 Pro", "openai/gpt-5.5-pro", ApiProvider::OpenRouterChat, "OpenRouter", {}, true, false},
        {"GLM-5.2", "GLM-5.2", "z-ai/glm-5.2", ApiProvider::OpenRouterChat, "OpenRouter", {"Parasail"}, false, false},
        {"Kimi K2.6", "Kimi K2.6", "moonshotai/kimi-k2.6", ApiProvider::OpenRouterChat, "OpenRouter", {"Moonshot AI"}, true, false},
        {"Gemini 3.5 Flash", "Gemini 3.5 Flash", "google/gemini-3.5-flash", ApiProvider::OpenRouterChat, "OpenRouter", {}, true, true},
        {"Gemini Flash Lite", "Gemini Flash Lite", "google/gemini-3.1-flash-lite", ApiProvider::OpenRouterChat, "OpenRouter", {}, true, true},
        {"Gemini Pro Latest", "Gemini Pro Latest", "~google/gemini-pro-latest", ApiProvider::OpenRouterChat, "OpenRouter", {}, true, true},
        {"GPT OSS 20B", "GPT OSS 20B", "openai/gpt-oss-20b", ApiProvider::OpenRouterChat, "OpenRouter", {}, true, false},
        {"Gemma 4 Free", "Gemma 4 Free", "google/gemma-4-26b-a4b-it:free", ApiProvider::OpenRouterChat, "OpenRouter", {}, true, false},
        {"GLM-5.2", "GLM-5.2", "z-ai/glm-5.2", ApiProvider::NvidiaChat, "NVIDIA", {}, false, false,
         QJsonObject{{"temperature", 1}, {"top_p", 1}, {"max_tokens", 16384}}},
        {"Nemotron 3 Ultra 550B A55B", "Nemotron 3 Ultra", "nvidia/nemotron-3-ultra-550b-a55b", ApiProvider::NvidiaChat, "NVIDIA", {}, false, false,
         QJsonObject{{"temperature", 1}, {"top_p", 0.95}, {"max_tokens", 16384}, {"chat_template_kwargs", QJsonObject{{"enable_thinking", true}, {"force_nonempty_content", true}}}, {"reasoning_budget", 16384}}},
        {"GPT OSS 20B", "GPT OSS 20B", "openai/gpt-oss-20b", ApiProvider::NvidiaChat, "NVIDIA", {}, false, false,
         QJsonObject{{"temperature", 1}, {"top_p", 1}, {"max_tokens", 4096}}},
        {"GPT OSS 120B", "GPT OSS 120B", "openai/gpt-oss-120b", ApiProvider::NvidiaChat, "NVIDIA", {}, false, false,
         QJsonObject{{"temperature", 1}, {"top_p", 1}, {"max_tokens", 4096}}},
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

ModelInfo ModelCatalog::modelForProviderAndDisplayName(const QString &provider, const QString &displayName)
{
    for (const auto &model : models()) {
        if (model.providerLabel.compare(provider, Qt::CaseInsensitive) == 0
            && model.displayName.compare(displayName, Qt::CaseInsensitive) == 0) {
            return model;
        }
    }
    for (const auto &model : models()) {
        if (model.providerLabel.compare(provider, Qt::CaseInsensitive) == 0) {
            return model;
        }
    }
    return modelForDisplayName(displayName);
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
    custom.supportsFiles = custom.provider == ApiProvider::OpenAIResponses;
    return custom;
}

QVariantList ModelCatalog::providerOptions()
{
    QVariantList result;
    for (const QString &provider : {QStringLiteral("OpenAI"), QStringLiteral("OpenRouter"), QStringLiteral("NVIDIA")}) {
        QVariantMap row;
        row["name"] = provider;
        result.append(row);
    }
    return result;
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
        row["supportsFiles"] = model.supportsFiles;
        result.append(row);
    }
    return result;
}

QVariantList ModelCatalog::modelOptionsForProvider(const QString &provider)
{
    QVariantList result;
    for (const auto &model : models()) {
        if (model.providerLabel.compare(provider, Qt::CaseInsensitive) != 0) {
            continue;
        }
        QVariantMap row;
        row["displayName"] = model.displayName;
        row["menuTitle"] = model.menuTitle;
        row["apiModel"] = model.apiModel;
        row["provider"] = model.providerLabel;
        row["supportsImages"] = model.supportsImages;
        row["supportsFiles"] = model.supportsFiles;
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
    case ApiProvider::NvidiaChat:
        return "NVIDIA";
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

QString ModelCatalog::nvidiaReasoningEffort(const QString &effort)
{
    const QString normalized = effort.trimmed().toLower();
    if (normalized == "instant") {
        return "none";
    }
    if (normalized == "pro" || normalized == "extra high") {
        return "max";
    }
    return "high";
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
