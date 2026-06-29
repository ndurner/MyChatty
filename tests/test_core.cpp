#include "ChatSerializer.h"
#include "ChatController.h"
#include "ChatTypes.h"
#include "MarkdownRenderer.h"
#include "ModelCatalog.h"
#include "SseParser.h"

#include <QJsonDocument>
#include <QSignalSpy>
#include <QTest>

using namespace MyChatty;

class CoreTests : public QObject {
    Q_OBJECT

private slots:
    void modelCatalogContainsRequestedModels()
    {
        QCOMPARE(ModelCatalog::modelForDisplayName("5.5").apiModel, QString("gpt-5.5"));
        QCOMPARE(ModelCatalog::modelForDisplayName("5.4-mini").apiModel, QString("gpt-5.4-mini"));
        QCOMPARE(ModelCatalog::modelForDisplayName("5.5 Pro").apiModel, QString("gpt-5.5-pro"));
        const ModelInfo glm = ModelCatalog::modelForDisplayName("GLM-5.2");
        QCOMPARE(glm.apiModel, QString("z-ai/glm-5.2"));
        QCOMPARE(glm.provider, ApiProvider::OpenRouterChat);
        QCOMPARE(glm.providerOnly, QStringList{"Parasail"});
        const ModelInfo gemma = ModelCatalog::modelForDisplayName("Gemma 4 Free");
        QCOMPARE(gemma.apiModel, QString("google/gemma-4-26b-a4b-it:free"));
        QCOMPARE(gemma.provider, ApiProvider::OpenRouterChat);
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
        QCOMPARE(ModelCatalog::modelForDisplayName(controller.selectedModel()).provider,
                 ApiProvider::OpenRouterChat);
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
