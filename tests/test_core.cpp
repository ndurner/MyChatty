#include "ApiClient.h"
#include "ChatSerializer.h"
#include "ChatController.h"
#include "ChatTypes.h"
#include "MarkdownRenderer.h"
#include "ModelCatalog.h"
#include "OpenaiResponsesAPI.h"
#include "SseParser.h"
#include "ToolExecutor.h"

#include <QJsonDocument>
#include <QDir>
#include <QSignalSpy>
#include <QTest>
#include <QTemporaryFile>

using namespace MyChatty;

class ApiClientProbe : public ApiClient {
public:
    using ApiClient::networkErrorText;

    void send(const ChatRequest &) override {}
};

class CoreTests : public QObject {
    Q_OBJECT

private slots:
    void modelCatalogContainsRequestedModels()
    {
        QCOMPARE(ModelCatalog::modelForProviderAndDisplayName("OpenAI", "5.6 Sol").apiModel,
                 QString("gpt-5.6-sol"));
        QCOMPARE(ModelCatalog::modelForProviderAndDisplayName("OpenAI", "5.6 Terra").apiModel,
                 QString("gpt-5.6-terra"));
        QCOMPARE(ModelCatalog::modelForProviderAndDisplayName("OpenAI", "5.6 Luna").apiModel,
                 QString("gpt-5.6-luna"));
        QCOMPARE(ModelCatalog::modelForDisplayName("5.5").apiModel, QString("gpt-5.5"));
        QCOMPARE(ModelCatalog::modelForDisplayName("5.4-mini").apiModel, QString("gpt-5.4-mini"));
        QCOMPARE(ModelCatalog::modelForDisplayName("5.5 Pro").apiModel, QString("gpt-5.5-pro"));
        QCOMPARE(ModelCatalog::modelForProviderAndDisplayName("OpenRouter", "5.6 Sol").apiModel,
                 QString("openai/gpt-5.6-sol"));
        QCOMPARE(ModelCatalog::modelForProviderAndDisplayName("OpenRouter", "5.6 Terra").apiModel,
                 QString("openai/gpt-5.6-terra"));
        QCOMPARE(ModelCatalog::modelForProviderAndDisplayName("OpenRouter", "5.6 Luna").apiModel,
                 QString("openai/gpt-5.6-luna"));
        const ModelInfo glm = ModelCatalog::modelForDisplayName("GLM-5.2");
        QCOMPARE(glm.apiModel, QString("z-ai/glm-5.2"));
        QCOMPARE(glm.provider, ApiProvider::OpenRouterChat);
        QCOMPARE(glm.providerOnly, QStringList{"Parasail"});
        QVERIFY(!glm.supportsImages);
        const ModelInfo kimi = ModelCatalog::modelForDisplayName("Kimi K2.6");
        QCOMPARE(kimi.apiModel, QString("moonshotai/kimi-k2.6"));
        QCOMPARE(kimi.provider, ApiProvider::OpenRouterChat);
        QCOMPARE(kimi.providerOnly, QStringList{"Moonshot AI"});
        QVERIFY(kimi.supportsImages);
        const ModelInfo gemma = ModelCatalog::modelForDisplayName("Gemma 4 Free");
        QCOMPARE(gemma.apiModel, QString("google/gemma-4-26b-a4b-it:free"));
        QCOMPARE(gemma.provider, ApiProvider::OpenRouterChat);
        QVERIFY(gemma.supportsImages);
        QVERIFY(!gemma.supportsFiles);

        const ModelInfo flash = ModelCatalog::modelForDisplayName("Gemini 3.5 Flash");
        QCOMPARE(flash.apiModel, QString("google/gemini-3.5-flash"));
        QCOMPARE(flash.provider, ApiProvider::OpenRouterChat);
        QVERIFY(flash.supportsFiles);

        const ModelInfo flashLite = ModelCatalog::modelForDisplayName("Gemini Flash Lite");
        QCOMPARE(flashLite.apiModel, QString("google/gemini-3.1-flash-lite"));
        QVERIFY(flashLite.supportsFiles);

        const ModelInfo geminiPro = ModelCatalog::modelForDisplayName("Gemini Pro Latest");
        QCOMPARE(geminiPro.apiModel, QString("~google/gemini-pro-latest"));
        QVERIFY(geminiPro.supportsFiles);

        const ModelInfo gptOss = ModelCatalog::modelForDisplayName("GPT OSS 20B");
        QCOMPARE(gptOss.apiModel, QString("openai/gpt-oss-20b"));
        QCOMPARE(gptOss.provider, ApiProvider::OpenRouterChat);
        QVERIFY(gptOss.providerOnly.isEmpty());

        const ModelInfo nvidiaGlm = ModelCatalog::modelForProviderAndDisplayName("NVIDIA", "GLM-5.2");
        QCOMPARE(nvidiaGlm.apiModel, QString("z-ai/glm-5.2"));
        QCOMPARE(nvidiaGlm.provider, ApiProvider::NvidiaChat);
        QCOMPARE(nvidiaGlm.chatParameters.value("temperature").toInt(), 1);
        QCOMPARE(nvidiaGlm.chatParameters.value("top_p").toInt(), 1);
        QCOMPARE(nvidiaGlm.chatParameters.value("max_tokens").toInt(), 16384);
        QVERIFY(!nvidiaGlm.chatParameters.contains("seed"));

        const ModelInfo nvidiaGptOss = ModelCatalog::modelForProviderAndDisplayName("NVIDIA", "GPT OSS 120B");
        QCOMPARE(nvidiaGptOss.apiModel, QString("openai/gpt-oss-120b"));
        QCOMPARE(nvidiaGptOss.provider, ApiProvider::NvidiaChat);
        QCOMPARE(nvidiaGptOss.chatParameters.value("max_tokens").toInt(), 4096);
        QVERIFY(!ModelCatalog::effortOptions().contains(QStringLiteral("Pro")));
        QCOMPARE(ModelCatalog::openAIReasoningEffort(QStringLiteral("Extra High")), QStringLiteral("xhigh"));
    }

    void gpt56ProModeUsesProviderSpecificRequestShape()
    {
        ChatMessage user;
        user.id = "u1";
        user.role = "user";
        user.text = "hello";

        ChatRequest request;
        request.history = {user};
        request.effort = "Medium";
        request.reasoningMode = "Pro";

        request.model = ModelCatalog::modelForProviderAndDisplayName("OpenAI", "5.6 Sol");
        QJsonObject payload = buildOpenaiResponsesPayload(request);
        QCOMPARE(payload.value("model").toString(), QString("gpt-5.6-sol"));
        QCOMPARE(payload.value("reasoning").toObject().value("mode").toString(), QString("pro"));
        QCOMPARE(payload.value("reasoning").toObject().value("effort").toString(), QString("medium"));

        request.model = ModelCatalog::modelForProviderAndDisplayName("OpenRouter", "5.6 Sol");
        payload = buildOpenaiChatPayload(request);
        QCOMPARE(payload.value("model").toString(), QString("openai/gpt-5.6-sol-pro"));
        QVERIFY(!payload.value("reasoning").toObject().contains("mode"));
        QCOMPARE(payload.value("reasoning").toObject().value("effort").toString(), QString("medium"));

        request.reasoningMode = "Standard";
        payload = buildOpenaiChatPayload(request);
        QCOMPARE(payload.value("model").toString(), QString("openai/gpt-5.6-sol"));
    }

    void reasoningEffortMappingsMatchVendorCapabilities()
    {
        using EffortMapping = QPair<QString, QString>;
        const auto verify = [](const QString &provider,
                               const QString &displayName,
                               const QList<EffortMapping> &mappings) {
            const ModelInfo model = ModelCatalog::modelForProviderAndDisplayName(provider, displayName);
            QVariantList expectedOptions;
            for (const auto &[uiEffort, apiEffort] : mappings) {
                expectedOptions.append(uiEffort);

                ChatRequest request;
                request.model = model;
                request.effort = uiEffort;
                const QJsonObject payload = model.provider == ApiProvider::OpenAIResponses
                    ? buildOpenaiResponsesPayload(request)
                    : buildOpenaiChatPayload(request);

                if (model.provider == ApiProvider::OpenAIResponses) {
                    QCOMPARE(payload.value("reasoning").toObject().value("effort").toString(), apiEffort);
                } else if (model.provider == ApiProvider::OpenRouterChat) {
                    QCOMPARE(payload.contains("reasoning"), !apiEffort.isEmpty());
                    if (!apiEffort.isEmpty()) {
                        QCOMPARE(payload.value("reasoning").toObject().value("effort").toString(), apiEffort);
                    }
                } else {
                    QCOMPARE(payload.contains("reasoning_effort"), !apiEffort.isEmpty());
                    if (!apiEffort.isEmpty()) {
                        QCOMPARE(payload.value("reasoning_effort").toString(), apiEffort);
                    }
                }
            }
            QCOMPARE(ModelCatalog::effortOptionsForModel(model), expectedOptions);
        };

        const QList<EffortMapping> fullOpenAI{
            {"Instant", "none"},
            {"Medium", "medium"},
            {"High", "high"},
            {"Extra High", "xhigh"},
        };
        for (const QString &model : {QStringLiteral("5.6 Sol"),
                                     QStringLiteral("5.6 Terra"),
                                     QStringLiteral("5.6 Luna"),
                                     QStringLiteral("5.5"),
                                     QStringLiteral("5.4-mini")}) {
            verify("OpenAI", model, fullOpenAI);
        }
        verify("OpenAI", "5.5 Pro", {{"Medium", "medium"}, {"High", "high"}, {"Extra High", "xhigh"}});

        for (const QString &model : {QStringLiteral("5.6 Sol"),
                                     QStringLiteral("5.6 Terra"),
                                     QStringLiteral("5.6 Luna")}) {
            verify("OpenRouter", model, fullOpenAI);
        }
        verify("OpenRouter", "GLM-5.2", {{"High", "high"}, {"Extra High", "xhigh"}});
        verify("OpenRouter", "Gemini 3.5 Flash", {{"Instant", "minimal"}, {"Medium", "medium"}, {"High", "high"}});
        verify("OpenRouter", "Gemini Flash Lite", {{"Instant", "minimal"}, {"Medium", "medium"}, {"High", "high"}});
        verify("OpenRouter", "Gemini Pro Latest", {{"Instant", "low"}, {"Medium", "medium"}, {"High", "high"}});
        verify("OpenRouter", "GPT OSS 20B", {{"Instant", "low"}, {"Medium", "medium"}, {"High", "high"}});
        verify("OpenRouter", "Kimi K2.6", {{"Default", ""}});
        verify("OpenRouter", "Gemma 4 Free", {{"Default", ""}});

        verify("NVIDIA", "Nemotron 3 Ultra 550B A55B", {{"Instant", "none"}, {"Medium", "medium"}, {"High", "high"}});
        verify("NVIDIA", "GPT OSS 20B", {{"Instant", "low"}, {"Medium", "medium"}, {"High", "high"}});
        verify("NVIDIA", "GPT OSS 120B", {{"Instant", "low"}, {"Medium", "medium"}, {"High", "high"}});
        verify("NVIDIA", "GLM-5.2", {{"Default", ""}});
    }

    void legacyProModelDoesNotReceiveGpt56Mode()
    {
        ChatMessage user;
        user.id = "u1";
        user.role = "user";
        user.text = "hello";

        ChatRequest request;
        request.history = {user};
        request.model = ModelCatalog::modelForProviderAndDisplayName("OpenAI", "5.5 Pro");
        request.effort = "High";
        request.reasoningMode = "Pro";

        const QJsonObject payload = buildOpenaiResponsesPayload(request);
        QCOMPARE(payload.value("model").toString(), QString("gpt-5.5-pro"));
        QVERIFY(!payload.value("reasoning").toObject().contains("mode"));
    }

    void openRouterPayloadPinsParasailForGlm()
    {
        ChatMessage user;
        user.id = "u1";
        user.role = "user";
        user.text = "hello";
        ChatRequest request;
        request.history = {user};
        request.model = ModelCatalog::modelForDisplayName("GLM-5.2");
        request.effort = "Medium";
        const QJsonObject payload = buildOpenaiChatPayload(request);
        QCOMPARE(payload.value("model").toString(), QString("z-ai/glm-5.2"));
        const QJsonObject provider = payload.value("provider").toObject();
        QCOMPARE(provider.value("allow_fallbacks").toBool(), false);
        QCOMPARE(provider.value("only").toArray().first().toString(), QString("Parasail"));
        QCOMPARE(provider.value("order").toArray().first().toString(), QString("Parasail"));
    }

    void chatControllerDefaultsToFreeOpenRouterModel()
    {
        ChatController controller(nullptr);
        QCOMPARE(controller.selectedModel(), QString("Gemma 4 Free"));
        QCOMPARE(controller.selectedProvider(), QString("OpenRouter"));
        QCOMPARE(controller.selectedEffort(), QString("Default"));
        QCOMPARE(controller.effortOptions(), QVariantList{"Default"});
        QCOMPARE(ModelCatalog::modelForDisplayName(controller.selectedModel()).provider,
                 ApiProvider::OpenRouterChat);
    }

    void chatControllerSwitchesProviderModelList()
    {
        ChatController controller(nullptr);
        controller.setSelectedProvider("NVIDIA");
        QCOMPARE(controller.selectedProvider(), QString("NVIDIA"));
        QCOMPARE(controller.selectedModel(), QString("GLM-5.2"));
        QCOMPARE(controller.selectedEffort(), QString("Default"));
        QCOMPARE(controller.effortOptions(), QVariantList{"Default"});
        QCOMPARE(controller.modelOptions().first().toMap().value("provider").toString(), QString("NVIDIA"));

        controller.setSelectedModel("Nemotron 3 Ultra 550B A55B");
        QCOMPARE(controller.selectedEffort(), QString("Medium"));
        QCOMPARE(controller.effortOptions(), QVariantList({"Instant", "Medium", "High"}));
    }

    void responsesPayloadUsesResponsesShape()
    {
        ChatMessage user;
        user.id = "u1";
        user.role = "user";
        user.text = "hello";
        ChatRequest request;
        request.history = {user};
        request.model = ModelCatalog::modelForDisplayName("5.4-mini");
        request.effort = "High";
        request.customInstructions = "Be terse.";
        const QJsonObject payload = buildOpenaiResponsesPayload(request);
        QCOMPARE(payload.value("model").toString(), QString("gpt-5.4-mini"));
        QVERIFY(payload.contains("input"));
        QVERIFY(payload.contains("reasoning"));
        QCOMPARE(payload.value("reasoning").toObject().value("effort").toString(), QString("high"));
        QVERIFY(payload.value("instructions").toString().contains("Be terse."));
    }

    void responsesApiReportsNestedFailureMessage()
    {
        OpenaiResponsesAPI client;
        QSignalSpy failed(&client, &ApiClient::failed);
        const QJsonObject event{
            {"type", "response.failed"},
            {"response", QJsonObject{
                {"error", QJsonObject{{"message", "The requested model does not exist."}}}
            }}
        };

        QVERIFY(QMetaObject::invokeMethod(
            &client,
            "handleEvent",
            Qt::DirectConnection,
            Q_ARG(QString, QStringLiteral("response.failed")),
            Q_ARG(QByteArray, QJsonDocument(event).toJson(QJsonDocument::Compact))));
        QCOMPARE(failed.count(), 1);
        QCOMPARE(failed.first().first().toString(), QString("The requested model does not exist."));
    }

    void responsesPayloadSupportsNativeWebSearch()
    {
        ChatMessage user;
        user.id = "u1";
        user.role = "user";
        user.text = "what changed today?";
        ChatRequest request;
        request.history = {user};
        request.model = ModelCatalog::modelForDisplayName("5.4-mini");
        request.effort = "Medium";
        request.enableWebSearch = true;

        const QJsonObject payload = buildOpenaiResponsesPayload(request);
        const QJsonArray tools = payload.value("tools").toArray();
        QCOMPARE(tools.size(), 1);
        QCOMPARE(tools.first().toObject().value("type").toString(), QString("web_search"));
        QVERIFY(payload.value("include").toArray().contains("web_search_call.action.sources"));
    }

    void responsesPayloadSupportsExaMcpSearch()
    {
        ChatMessage user;
        user.id = "u1";
        user.role = "user";
        user.text = "what changed today?";
        ChatRequest request;
        request.history = {user};
        request.model = ModelCatalog::modelForDisplayName("5.4-mini");
        request.effort = "Medium";
        request.exaApiKey = "exa-test-key";
        request.enableWebSearch = true;
        request.useExaSearch = true;

        const QJsonObject payload = buildOpenaiResponsesPayload(request);
        const QJsonObject tool = payload.value("tools").toArray().first().toObject();
        QCOMPARE(tool.value("type").toString(), QString("mcp"));
        QCOMPARE(tool.value("server_label").toString(), QString("exa"));
        QCOMPARE(tool.value("server_url").toString(), QString("https://mcp.exa.ai/mcp?exaApiKey=exa-test-key"));
        QCOMPARE(tool.value("require_approval").toString(), QString("never"));
        QVERIFY(!tool.contains("allowed_tools"));
    }

    void openRouterPayloadSupportsWebSearchEngines()
    {
        ChatMessage user;
        user.id = "u1";
        user.role = "user";
        user.text = "what changed today?";
        ChatRequest request;
        request.history = {user};
        request.model = ModelCatalog::modelForDisplayName("Gemma 4 Free");
        request.effort = "Medium";
        request.enableWebSearch = true;

        QJsonObject payload = buildOpenaiChatPayload(request);
        QJsonObject tool = payload.value("tools").toArray().first().toObject();
        QCOMPARE(tool.value("type").toString(), QString("openrouter:web_search"));
        QVERIFY(!tool.contains("parameters"));
        QVERIFY(!payload.contains("plugins"));
        QVERIFY(!payload.contains("web_search_options"));
        QCOMPARE(payload.value("messages").toArray().first().toObject().value("role").toString(), QString("system"));
        QVERIFY(payload.value("messages").toArray().first().toObject().value("content").toString().contains("openrouter:web_search"));

        request.useExaSearch = true;
        payload = buildOpenaiChatPayload(request);
        tool = payload.value("tools").toArray().first().toObject();
        QCOMPARE(tool.value("parameters").toObject().value("engine").toString(), QString("exa"));
        QCOMPARE(tool.value("parameters").toObject().value("search_context_size").toString(), QString("medium"));
    }

    void nvidiaPayloadUsesChatCompletionsExtensions()
    {
        ChatMessage user;
        user.id = "u1";
        user.role = "user";
        user.text = "hello";
        ChatRequest request;
        request.history = {user};
        request.model = ModelCatalog::modelForProviderAndDisplayName("NVIDIA", "Nemotron 3 Ultra 550B A55B");
        request.effort = "Medium";
        request.maxOutputTokens = 128;

        QJsonObject payload = buildOpenaiChatPayload(request);
        QCOMPARE(payload.value("model").toString(), QString("nvidia/nemotron-3-ultra-550b-a55b"));
        QVERIFY(!payload.contains("reasoning"));
        QCOMPARE(payload.value("temperature").toInt(), 1);
        QCOMPARE(payload.value("top_p").toDouble(), 0.95);
        QCOMPARE(payload.value("max_tokens").toInt(), 128);
        QCOMPARE(payload.value("reasoning_budget").toInt(), 16384);
        const QJsonObject kwargs = payload.value("chat_template_kwargs").toObject();
        QVERIFY(!kwargs.contains("enable_thinking"));
        QCOMPARE(kwargs.value("force_nonempty_content").toBool(), true);
        QVERIFY(!kwargs.contains("medium_effort"));
        QCOMPARE(payload.value("reasoning_effort").toString(), QString("medium"));

        request.model = ModelCatalog::modelForProviderAndDisplayName("NVIDIA", "GPT OSS 20B");
        request.maxOutputTokens = 0;
        payload = buildOpenaiChatPayload(request);
        QCOMPARE(payload.value("temperature").toInt(), 1);
        QCOMPARE(payload.value("top_p").toInt(), 1);
        QCOMPARE(payload.value("max_tokens").toInt(), 4096);
        QCOMPARE(payload.value("reasoning_effort").toString(), QString("medium"));
    }

    void openRouterPayloadAddsPdfFileParser()
    {
        QTemporaryFile pdf(QDir::tempPath() + "/mychatty-pdf-XXXXXX.pdf");
        QVERIFY(pdf.open());
        pdf.write("%PDF-1.4\n1 0 obj\n<<>>\nendobj\n%%EOF\n");
        pdf.close();

        ChatMessage user;
        user.id = "u1";
        user.role = "user";
        user.text = "summarize";
        user.attachments = {Attachment::fromUrl(QUrl::fromLocalFile(pdf.fileName()), "files")};
        ChatRequest request;
        request.history = {user};
        request.model = ModelCatalog::modelForDisplayName("Gemma 4 Free");
        request.effort = "Medium";

        QJsonObject payload = buildOpenaiChatPayload(request);
        QJsonObject plugin = payload.value("plugins").toArray().first().toObject();
        QCOMPARE(plugin.value("id").toString(), QString("file-parser"));
        QCOMPARE(plugin.value("pdf").toObject().value("engine").toString(), QString("cloudflare-ai"));
        QJsonArray content = payload.value("messages").toArray().first().toObject().value("content").toArray();
        QCOMPARE(content.at(1).toObject().value("type").toString(), QString("file"));
        QVERIFY(content.at(1).toObject().value("file").toObject().value("file_data").toString().startsWith("data:application/pdf;base64,"));

        request.openRouterPdfEngine = "native";
        payload = buildOpenaiChatPayload(request);
        plugin = payload.value("plugins").toArray().first().toObject();
        QCOMPARE(plugin.value("pdf").toObject().value("engine").toString(), QString("native"));
    }

    void payloadsAdvertiseJavaScriptTool()
    {
        ChatMessage user;
        user.id = "u1";
        user.role = "user";
        user.text = "2+2";
        ChatRequest request;
        request.history = {user};
        request.model = ModelCatalog::modelForDisplayName("5.4-mini");
        request.effort = "Medium";
        request.enableJavaScriptUse = true;

        QJsonObject payload = buildOpenaiResponsesPayload(request);
        QJsonArray tools = payload.value("tools").toArray();
        QCOMPARE(tools.size(), 1);
        QCOMPARE(tools.at(0).toObject().value("name").toString(), QString("eval_javascript"));

        request.enableWebBrowser = true;
        request.enablePageScreenshots = true;
        payload = buildOpenaiResponsesPayload(request);
        tools = payload.value("tools").toArray();
        QCOMPARE(tools.size(), 4);
        QCOMPARE(tools.at(1).toObject().value("name").toString(), QString("open_web_page"));
        QVERIFY(tools.at(1).toObject().value("parameters").toObject().value("required").toArray().contains("url_provenance"));
        QCOMPARE(tools.at(2).toObject().value("name").toString(), QString("read_web_page_text"));
        QCOMPARE(tools.at(3).toObject().value("name").toString(), QString("get_next_web_page_screenshot"));
        request.enablePageScreenshots = false;

        request.model = ModelCatalog::modelForDisplayName("Gemini 3.5 Flash");
        payload = buildOpenaiChatPayload(request);
        tools = payload.value("tools").toArray();
        QCOMPARE(tools.size(), 3);
        const QJsonObject function = tools.at(0).toObject().value("function").toObject();
        QCOMPARE(function.value("name").toString(), QString("eval_javascript"));
        QVERIFY(!function.value("parameters").toObject().contains("additionalProperties"));
        QCOMPARE(tools.at(1).toObject().value("function").toObject().value("name").toString(), QString("open_web_page"));
        QVERIFY(tools.at(1).toObject().value("function").toObject().value("parameters").toObject().value("required").toArray().contains("url_provenance"));
        QCOMPARE(tools.at(2).toObject().value("function").toObject().value("name").toString(), QString("read_web_page_text"));
        QCOMPARE(payload.value("reasoning").toObject().value("effort").toString(), QString("medium"));
    }

    void toolExecutorEvaluatesJavaScript()
    {
        ToolExecutor executor(nullptr, "test-js");
        const QJsonObject result = executor.execute("eval_javascript", QJsonObject{
            {"javascript_source_code", "const x = 2 ** 10; print(x + Math.sqrt(81));"},
        });
        QVERIFY(result.value("success").toBool());
        QCOMPARE(result.value("prints").toString(), QString("1033\n"));
    }

    void toolExecutorProvidesScopedJavaScriptFs()
    {
        ToolExecutor executor(nullptr, "test-js-fs");
        QJsonObject result = executor.execute("eval_javascript", QJsonObject{
            {"javascript_source_code", "fs.writeText('/tmp/result.txt', 'hello fs'); print(fs.readText('/tmp/result.txt'));"},
        });
        QVERIFY(result.value("success").toBool());
        QCOMPARE(result.value("prints").toString(), QString("hello fs\n"));

        result = executor.execute("eval_javascript", QJsonObject{
            {"javascript_source_code", "print(fs.readText('/../conversations.json') === '');"},
        });
        QVERIFY(result.value("success").toBool());
        QCOMPARE(result.value("prints").toString(), QString("true\n"));
    }

    void openRouterPayloadReplaysToolMessages()
    {
        ChatMessage user;
        user.id = "u1";
        user.role = "user";
        user.text = "calculate";

        ChatMessage assistant;
        assistant.id = "a1";
        assistant.role = "assistant";
        assistant.rawOutputItems = QJsonArray{QJsonObject{
            {"role", "assistant"},
            {"content", ""},
            {"reasoning_details", QJsonArray{QJsonObject{
                {"type", "reasoning.encrypted"},
                {"data", "opaque"},
                {"format", "google-gemini-v1"},
                {"index", 0},
            }}},
            {"tool_calls", QJsonArray{QJsonObject{
                {"id", "call_1"},
                {"type", "function"},
                {"function", QJsonObject{{"name", "eval_javascript"}, {"arguments", "{\"javascript_source_code\":\"print(2+2)\"}"}}},
            }}},
        }};

        ChatMessage tool;
        tool.id = "t1";
        tool.role = "assistant";
        tool.rawOutputItems = QJsonArray{QJsonObject{
            {"role", "tool"},
            {"tool_call_id", "call_1"},
            {"content", "{\"success\":true,\"prints\":\"4\\n\"}"},
        }};

        ChatRequest request;
        request.history = {user, assistant, tool};
        request.model = ModelCatalog::modelForDisplayName("Gemini 3.5 Flash");
        request.effort = "Medium";

        const QJsonArray messages = buildOpenaiChatPayload(request).value("messages").toArray();
        QCOMPARE(messages.size(), 3);
        QCOMPARE(messages.at(1).toObject().value("tool_calls").toArray().first().toObject().value("id").toString(), QString("call_1"));
        QCOMPARE(messages.at(1).toObject().value("reasoning_details").toArray().first().toObject().value("data").toString(), QString("opaque"));
        QCOMPARE(messages.at(2).toObject().value("role").toString(), QString("tool"));
    }

    void messageModelExposesToolCallIndicators()
    {
        ChatMessage assistant;
        assistant.id = "a1";
        assistant.role = "assistant";
        assistant.rawOutputItems = QJsonArray{QJsonObject{
            {"role", "assistant"},
            {"content", ""},
            {"tool_calls", QJsonArray{QJsonObject{
                {"id", "call_1"},
                {"type", "function"},
                {"function", QJsonObject{{"name", "eval_javascript"}, {"arguments", "{\"javascript_source_code\":\"print(8)\"}"}}},
            }}},
        }};

        ChatMessage tool;
        tool.id = "t1";
        tool.role = "assistant";
        tool.rawOutputItems = QJsonArray{QJsonObject{
            {"role", "tool"},
            {"tool_call_id", "call_1"},
            {"content", "{\"success\":true,\"prints\":\"8\\n\"}"},
        }};

        ChatMessageModel model;
        model.append(assistant);
        QSignalSpy changed(&model, &ChatMessageModel::dataChanged);
        model.append(tool);
        QCOMPARE(changed.count(), 1);
        const QVariantList calls = model.data(model.index(0), ChatMessageModel::ToolCallsRole).toList();
        QCOMPARE(calls.size(), 1);
        QCOMPARE(calls.first().toMap().value("label").toString(), QString("Code Interpreter"));
        QVERIFY(calls.first().toMap().value("arguments").toString().contains("print(8)"));
        QCOMPARE(calls.first().toMap().value("hasOutput").toBool(), true);
        QVERIFY(calls.first().toMap().value("output").toString().contains("prints"));
        QCOMPARE(model.data(model.index(1), ChatMessageModel::ToolOnlyRole).toBool(), true);
    }

    void messageModelExposesWebSearchIndicators()
    {
        ChatMessage assistant;
        assistant.id = "a1";
        assistant.role = "assistant";
        assistant.toolIndicators = QJsonArray{QJsonObject{
            {"type", "server_tool_call"},
            {"name", "openrouter:web_search"},
        }};

        ChatMessageModel model;
        model.append(assistant);
        const QVariantList calls = model.data(model.index(0), ChatMessageModel::ToolCallsRole).toList();
        QCOMPARE(calls.size(), 1);
        QCOMPARE(calls.first().toMap().value("label").toString(), QString("Web Search"));
    }

    void messageModelExposesToolApproval()
    {
        ChatMessage assistant;
        assistant.id = "a1";
        assistant.role = "assistant";
        assistant.approval = QJsonObject{
            {"id", "approval_1"},
            {"type", "open_web_page"},
            {"status", "pending"},
            {"url", "https://example.com"},
        };

        ChatMessageModel model;
        model.append(assistant);
        const QVariantMap approval = model.data(model.index(0), ChatMessageModel::ApprovalRole).toMap();
        QCOMPARE(approval.value("type").toString(), QString("open_web_page"));
        QCOMPARE(approval.value("status").toString(), QString("pending"));
        QCOMPARE(approval.value("url").toString(), QString("https://example.com"));
    }

    void markdownRendersBoldAndCodeFence()
    {
        const QString html = MarkdownRenderer::render("### Title\n\n**bold** and `inline`\n\n<pre>\n```cpp\nint x = 1 < 2;\n```\n</pre>");
        QVERIFY(!html.contains("<html"));
        QVERIFY(!html.contains("<body"));
        QVERIFY(!html.contains("&lt;pre&gt;"));
        QVERIFY(html.contains("<h3"));
        QVERIFY(html.contains("<strong>bold</strong>"));
        QVERIFY(html.contains("<code"));
        QVERIFY(html.contains("int x = 1"));
        QVERIFY(html.contains("&lt;"));
    }

    void markdownParsesTablesAndNestedLists()
    {
        const QVariantList blocks = MarkdownRenderer::renderBlocks(
            "| A | B |\n"
            "| --- | ---: |\n"
            "| one | two |\n\n"
            "- Parent\n"
            "  - Child\n"
            "  1. Numbered child\n");
        QCOMPARE(blocks.at(0).toMap().value("type").toString(), QString("table"));
        QCOMPARE(blocks.at(0).toMap().value("headers").toList().size(), 2);
        QCOMPARE(blocks.at(0).toMap().value("rows").toList().size(), 1);
        QCOMPARE(blocks.at(1).toMap().value("type").toString(), QString("list"));
        const QVariantList items = blocks.at(1).toMap().value("items").toList();
        QCOMPARE(items.size(), 3);
        QCOMPARE(items.at(0).toMap().value("level").toInt(), 0);
        QCOMPARE(items.at(1).toMap().value("level").toInt(), 1);
        QCOMPARE(items.at(2).toMap().value("ordered").toBool(), true);
    }

    void markdownParsesMultilineTableRows()
    {
        const QVariantList blocks = MarkdownRenderer::renderBlocks(
            "| Field | Details |\n"
            "|-------|---------|\n"
            "| **Profession** | Software engineer. |\n"
            "| **Current focus** |  \n"
            "* Research and engineering in generative AI\n"
            "* Security and privacy |\n"
            "| **Output** | Papers <br> Patents <br> Talks |\n");

        QCOMPARE(blocks.size(), 1);
        const QVariantMap table = blocks.first().toMap();
        QCOMPARE(table.value("type").toString(), QString("table"));
        const QVariantList rows = table.value("rows").toList();
        QCOMPARE(rows.size(), 3);
        const QVariantList focusCells = rows.at(1).toMap().value("cells").toList();
        const QString focus = focusCells.at(1).toMap().value("html").toString();
        QVERIFY(focus.contains("Research and engineering"));
        QVERIFY(focus.contains("Security and privacy"));
        QVERIFY(!focus.contains("&lt;br"));

        const QString html = MarkdownRenderer::render("Line one <br> line two");
        QVERIFY(html.contains("<br/>"));
        QVERIFY(!html.contains("&lt;br"));
    }

    void markdownRendersLinks()
    {
        const QString html = MarkdownRenderer::render(
            "[OpenAI](https://openai.com) and https://example.com.\n\n"
            "| Site | Link |\n"
            "| --- | --- |\n"
            "| Docs | [Qt](https://doc.qt.io) |\n\n"
            "- [Item](https://example.org)\n");
        QVERIFY(html.contains("<a href=\"https://openai.com\">OpenAI</a>"));
        QVERIFY(html.contains("<a href=\"https://example.com\">https://example.com</a>"));
        QVERIFY(!html.contains("href=\"https://example.com.\""));
        QVERIFY(html.contains("<a href=\"https://doc.qt.io\">Qt</a>"));
        QVERIFY(html.contains("<a href=\"https://example.org\">Item</a>"));
    }

    void sseParserCombinesDataLines()
    {
        SseParser parser;
        QSignalSpy spy(&parser, &SseParser::eventReceived);
        parser.feed("event: test\ndata: {\"a\":1}\ndata: {\"b\":2}\n\n");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toString(), QString("test"));
    }

    void networkErrorsPreferProviderRawMetadata()
    {
        const QByteArray body = R"({
            "error": {
                "message": "Provider returned error",
                "metadata": {
                    "raw": "upstream rate-limited"
                }
            }
        })";
        QCOMPARE(ApiClientProbe::networkErrorText(nullptr, body), QString("upstream rate-limited"));
    }

    void exportUsesOaiChatMessagesShape()
    {
        ChatMessage user;
        user.id = "u1";
        user.role = "user";
        user.text = "hello";
        ChatMessage assistant;
        assistant.id = "a1";
        assistant.role = "assistant";
        assistant.text = "world";
        const QJsonObject exported = ChatSerializer::exportConversation({user, assistant}, "system");
        const QJsonArray messages = exported.value("messages").toArray();
        QCOMPARE(messages.size(), 3);
        QCOMPARE(messages.first().toObject().value("role").toString(), QString("system"));
        QCOMPARE(messages.last().toObject().value("content").toString(), QString("world"));
    }
};

QTEST_MAIN(CoreTests)
#include "test_core.moc"
