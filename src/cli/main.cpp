#include "ApiClient.h"
#include "ChatTypes.h"
#include "ModelCatalog.h"
#include "OpenaiChatAPI.h"
#include "OpenaiResponsesAPI.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

#include <memory>

using namespace MyChatty;

static QString readWholeFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromUtf8(file.readAll()).trimmed();
}

static QString openAIKey()
{
    const QByteArray env = qgetenv("OPENAI_API_KEY");
    return QString::fromUtf8(env).trimmed();
}

static QString openAIKey(bool settingsKeys)
{
    const QString key = openAIKey();
    if (!key.isEmpty() || !settingsKeys) {
        return key;
    }
    QSettings settings("NilsDurner", "MyChatty");
    return settings.value("openAIKey").toString().trimmed();
}

static QString openRouterKey()
{
    const QByteArray env = qgetenv("OPENROUTER_API_KEY");
    return QString::fromUtf8(env).trimmed();
}

static QString openRouterKey(bool settingsKeys)
{
    const QString key = openRouterKey();
    if (!key.isEmpty() || !settingsKeys) {
        return key;
    }
    QSettings settings("NilsDurner", "MyChatty");
    return settings.value("openRouterKey").toString().trimmed();
}

static QString exaKey()
{
    const QByteArray env = qgetenv("EXA_API_KEY");
    return QString::fromUtf8(env).trimmed();
}

static QString cachePath(const QString &key)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/cli-cache";
    QDir().mkpath(dir);
    const QByteArray digest = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256).toHex();
    return dir + "/" + QString::fromLatin1(digest) + ".json";
}

static QString optionValue(QCommandLineParser &parser, const QString &name, const QString &fallback)
{
    const QString value = parser.value(name);
    return value.isEmpty() ? fallback : value;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("NilsDurner");
    QCoreApplication::setApplicationName("MyChattyCLI");
    qRegisterMetaType<MyChatty::ChatResult>("MyChatty::ChatResult");

    QCommandLineParser parser;
    parser.setApplicationDescription("Run MyChatty provider calls through the shared app code.");
    parser.addHelpOption();
    parser.addOptions({
        {{"p", "provider"}, "Provider: openai or openrouter.", "provider", "openai"},
        {{"m", "model"}, "API model name or display name.", "model"},
        {{"e", "effort"}, "Effort: Instant, Medium, High, Extra High, Pro.", "effort", "Medium"},
        {{"q", "prompt"}, "Prompt text.", "prompt"},
        {{"f", "file"}, "Read prompt text from a file.", "file"},
        {{"i", "instructions"}, "Custom instructions.", "instructions"},
        {{"t", "max-tokens"}, "Maximum output tokens.", "tokens", "256"},
        {"web-search", "Enable web search for provider calls."},
        {"exa-search", "Use Exa for web search instead of provider-native search."},
        {"json", "Print result JSON instead of streaming plain text."},
        {"dry-run", "Print the JSON request payload and exit."},
        {"no-cache", "Bypass and overwrite the CLI response cache."},
        {"code-interpreter", "Advertise the local eval_javascript Code Interpreter tool."},
        {"settings-keys", "Read missing provider API keys from the MyChatty app QSettings domain."},
        {"trace-stream", "Print timestamped text and reasoning deltas for streaming diagnostics."},
    });
    parser.process(app);

    QString prompt = parser.value("prompt");
    if (prompt.isEmpty() && parser.isSet("file")) {
        prompt = readWholeFile(parser.value("file"));
    }
    if (prompt.trimmed().isEmpty()) {
        QTextStream(stderr) << "Prompt is required. Use --prompt or --file.\n";
        return 2;
    }

    const QString provider = parser.value("provider").toLower();
    const QString modelArg = parser.value("model");
    ModelInfo model = modelArg.isEmpty()
        ? (provider == "openrouter"
               ? ModelCatalog::modelForApiName("google/gemma-4-26b-a4b-it:free")
               : ModelCatalog::modelForApiName("gpt-5.4-mini"))
        : ModelCatalog::modelForApiName(modelArg);
    if (provider == "openrouter" && model.provider != ApiProvider::OpenRouterChat) {
        model.provider = ApiProvider::OpenRouterChat;
        model.providerLabel = "OpenRouter";
    } else if (provider == "openai") {
        model.provider = ApiProvider::OpenAIResponses;
        model.providerLabel = "OpenAI";
    }

    ChatMessage user;
    user.id = newId("msg");
    user.role = "user";
    user.text = prompt.trimmed();
    user.createdAt = QDateTime::currentDateTimeUtc();

    ChatRequest request;
    request.history = {user};
    request.model = model;
    request.effort = parser.value("effort");
    request.customInstructions = parser.value("instructions");
    request.maxOutputTokens = parser.value("max-tokens").toInt();
    request.exaApiKey = exaKey();
    request.enableWebSearch = parser.isSet("web-search");
    request.useExaSearch = parser.isSet("exa-search");
    request.enableJavaScriptUse = parser.isSet("code-interpreter");
    request.stream = true;
    request.apiKey = model.provider == ApiProvider::OpenRouterChat
        ? openRouterKey(parser.isSet("settings-keys"))
        : openAIKey(parser.isSet("settings-keys"));

    const QJsonObject payload = model.provider == ApiProvider::OpenRouterChat
        ? buildOpenaiChatPayload(request)
        : buildOpenaiResponsesPayload(request);
    if (parser.isSet("dry-run")) {
        QTextStream(stdout) << QJsonDocument(payload).toJson(QJsonDocument::Indented);
        return 0;
    }

    if (request.apiKey.trimmed().isEmpty()) {
        QTextStream(stderr) << "Missing API key for " << ModelCatalog::providerName(model.provider) << ".\n";
        return 3;
    }

    const QString cacheKey = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    const QString path = cachePath(cacheKey);
    if (!parser.isSet("no-cache")) {
        QFile cached(path);
        if (cached.open(QIODevice::ReadOnly)) {
            const QJsonObject cachedObject = QJsonDocument::fromJson(cached.readAll()).object();
            if (parser.isSet("json")) {
                QTextStream(stdout) << QJsonDocument(cachedObject).toJson(QJsonDocument::Indented);
            } else {
                QTextStream(stdout) << cachedObject.value("text").toString() << "\n";
            }
            return 0;
        }
    }

    std::unique_ptr<ApiClient> client;
    if (model.provider == ApiProvider::OpenRouterChat) {
        client = std::make_unique<OpenaiChatAPI>();
    } else {
        client = std::make_unique<OpenaiResponsesAPI>();
    }

    int exitCode = 0;
    QElapsedTimer timer;
    timer.start();
    int textDeltaCount = 0;
    int reasoningDeltaCount = 0;
    QObject::connect(client.get(), &ApiClient::textDelta, &app, [&](const QString &delta) {
        ++textDeltaCount;
        if (parser.isSet("trace-stream")) {
            QTextStream(stdout) << QString::number(timer.elapsed() / 1000.0, 'f', 3)
                                << "s text " << textDeltaCount << ": "
                                << QJsonDocument(QJsonArray{delta}).toJson(QJsonDocument::Compact) << "\n" << Qt::flush;
        } else if (!parser.isSet("json")) {
            QTextStream(stdout) << delta << Qt::flush;
        }
    });
    QObject::connect(client.get(), &ApiClient::reasoningDelta, &app, [&](const QString &delta) {
        ++reasoningDeltaCount;
        if (parser.isSet("trace-stream")) {
            QTextStream(stdout) << QString::number(timer.elapsed() / 1000.0, 'f', 3)
                                << "s reasoning " << reasoningDeltaCount << ": "
                                << QJsonDocument(QJsonArray{delta}).toJson(QJsonDocument::Compact) << "\n" << Qt::flush;
        }
    });
    QObject::connect(client.get(), &ApiClient::completed, &app, [&](const ChatResult &result) {
        QJsonObject output{
            {"text", result.text},
            {"reasoning", result.reasoning},
            {"rawOutputItems", result.rawOutputItems},
            {"rawResponse", result.rawResponse},
            {"rawEvents", result.rawEvents},
            {"usage", result.usage},
        };
        QFile cacheFile(path);
        if (cacheFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            cacheFile.write(QJsonDocument(output).toJson(QJsonDocument::Indented));
        }
        if (parser.isSet("trace-stream")) {
            QTextStream(stdout) << QString::number(timer.elapsed() / 1000.0, 'f', 3)
                                << "s complete text_deltas=" << textDeltaCount
                                << " reasoning_deltas=" << reasoningDeltaCount << "\n";
        } else if (parser.isSet("json")) {
            QTextStream(stdout) << QJsonDocument(output).toJson(QJsonDocument::Indented);
        } else {
            QTextStream(stdout) << "\n";
        }
        app.quit();
    });
    QObject::connect(client.get(), &ApiClient::failed, &app, [&](const QString &message) {
        QTextStream(stderr) << message << "\n";
        exitCode = 4;
        app.quit();
    });

    client->send(request);
    app.exec();
    return exitCode;
}
