#pragma once

#include "ChatTypes.h"

#include <memory>

class QNetworkAccessManager;

namespace MyChatty {

class ApiClient;

class ApiProtocolAdapter {
public:
    virtual ~ApiProtocolAdapter() = default;

    virtual QJsonObject buildPayload(const ChatRequest &request) const = 0;
    virtual QList<ToolCall> extractToolCalls(const ChatResult &result) const = 0;
    virtual QJsonObject serializeToolOutput(const QString &callId, const QJsonObject &output) const = 0;
    virtual std::unique_ptr<ApiClient> createClient(QNetworkAccessManager *network) const = 0;
};

class OpenAIResponsesProtocolAdapter final : public ApiProtocolAdapter {
public:
    QJsonObject buildPayload(const ChatRequest &request) const override;
    QList<ToolCall> extractToolCalls(const ChatResult &result) const override;
    QJsonObject serializeToolOutput(const QString &callId, const QJsonObject &output) const override;
    std::unique_ptr<ApiClient> createClient(QNetworkAccessManager *network) const override;
};

class ChatCompletionsProtocolAdapter final : public ApiProtocolAdapter {
public:
    QJsonObject buildPayload(const ChatRequest &request) const override;
    QList<ToolCall> extractToolCalls(const ChatResult &result) const override;
    QJsonObject serializeToolOutput(const QString &callId, const QJsonObject &output) const override;
    std::unique_ptr<ApiClient> createClient(QNetworkAccessManager *network) const override;
};

struct ProviderProfile {
    ApiProvider provider = ApiProvider::OpenAIResponses;
    QString endpoint;
    const ApiProtocolAdapter *protocol = nullptr;
    bool sendsOpenRouterAttribution = false;
};

const ApiProtocolAdapter &protocolAdapter(ApiProvider provider);
const ProviderProfile &providerProfile(ApiProvider provider);

} // namespace MyChatty
