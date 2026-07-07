#include "ToolExecutor.h"

#include "ConversationFileStore.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJSEngine>
#include <QRegularExpression>
#include <QScreen>
#include <QTimer>
#include <QUrl>
#include <QtWebView/QtWebView>
#include <QWebView>
#include <QWebViewSettings>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>

namespace MyChatty {
namespace {

static constexpr int PreviewLines = 40;
static constexpr int DefaultTextLimit = 80;
static constexpr int MaxTextLimit = 200;
static constexpr int ScreenshotWidth = 1024;
static constexpr int ScreenshotHeight = 1200;
static constexpr int ScreenshotOverlap = 160;
static constexpr int MaxScreenshots = 12;

QJsonObject toolOk(const QString &prints)
{
    return QJsonObject{{"success", true}, {"prints", prints}};
}

QJsonObject toolOkObject(QJsonObject object)
{
    object["success"] = true;
    return object;
}

QJsonObject toolError(const QString &message)
{
    return QJsonObject{{"success", false}, {"error", message}};
}

QString jsonStringLiteral(const QString &value)
{
    return QString::fromUtf8(QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact)).mid(1).chopped(1);
}

QString normalizedPageId(const QString &url)
{
    const QByteArray hash = QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Sha256).toHex().left(16);
    return QStringLiteral("page_%1").arg(QString::fromLatin1(hash));
}

QString pageCacheRoot(const QString &conversationId)
{
    ConversationFileStore store(conversationId.isEmpty() ? QStringLiteral("adhoc") : conversationId);
    const QString root = QDir(store.rootPath()).filePath(QStringLiteral("web-pages"));
    QDir().mkpath(root);
    return root;
}

QString pageDir(const QString &conversationId, const QString &pageId)
{
    QString safe = pageId;
    safe.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]")), QStringLiteral("_"));
    const QString dir = QDir(pageCacheRoot(conversationId)).filePath(safe);
    QDir().mkpath(dir);
    return dir;
}

bool writeTextFile(const QString &path, const QString &text)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        return false;
    }
    file.write(text.toUtf8());
    return true;
}

QString readTextFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll());
}

bool writeJsonFile(const QString &path, const QJsonObject &object)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    return true;
}

QJsonObject readJsonFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    return doc.isObject() ? doc.object() : QJsonObject{};
}

QStringList textLines(const QString &text)
{
    QString normalized = text;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace('\r', '\n');
    QStringList lines;
    for (const QString &line : normalized.split('\n')) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            lines.append(trimmed);
        }
    }
    return lines;
}

QString lineSlice(const QStringList &lines, int offset, int limit)
{
    const int lineCount = static_cast<int>(lines.size());
    const int start = std::clamp(offset, 0, lineCount);
    const int count = std::clamp(limit, 1, MaxTextLimit);
    return lines.mid(start, count).join('\n');
}

QJsonArray firstJsonArrayItems(const QJsonArray &items, int limit)
{
    QJsonArray result;
    const int count = std::min(limit, static_cast<int>(items.size()));
    for (int i = 0; i < count; ++i) {
        result.append(items.at(i));
    }
    return result;
}

QVariant runJavaScriptVariantSync(QWebView &view, const QString &script, int timeoutMs = 10000)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QVariant result;
    bool done = false;
    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        loop.quit();
    });
    view.runJavaScript(script, [&](const QVariant &value) {
        result = value;
        done = true;
        loop.quit();
    });
    timer.start(timeoutMs);
    loop.exec();
    return done ? result : QVariant();
}

bool loadPageSync(QWebView &view, const QUrl &url, int timeoutMs = 45000)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    bool loaded = false;
    bool timedOut = false;
    QObject::connect(&view, &QWebView::loadProgressChanged, &loop, [&](int progress) {
        if (progress == 100) {
            loaded = true;
            loop.quit();
        }
    });
    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        timedOut = true;
        loop.quit();
    });
    timer.start(timeoutMs);
    view.setUrl(url);
    loop.exec();
    if (timedOut || !loaded) {
        return false;
    }
    QEventLoop settleLoop;
    QTimer::singleShot(3000, &settleLoop, &QEventLoop::quit);
    settleLoop.exec();
    return true;
}

QString readabilityText(QWebView &view)
{
    const QString readability = readTextFile(QStringLiteral(":/third_party/readability/Readability.js"));
    if (readability.isEmpty()) {
        return {};
    }
    const QString script = QStringLiteral(R"JS(
(() => {
  const source = %1;
  try {
    (0, eval)(source);
    const clone = document.cloneNode(true);
    const article = new Readability(clone).parse();
    if (!article) return "";
    return [
      article.title ? "# " + article.title : "",
      article.excerpt || "",
      article.textContent || ""
    ].filter(Boolean).join("\n\n");
  } catch (error) {
    return "";
  }
})()
)JS").arg(jsonStringLiteral(readability));
    return runJavaScriptVariantSync(view, script, 10000).toString();
}

QJsonArray visibleLinks(QWebView &view)
{
    const QString script = QStringLiteral(R"JS(
JSON.stringify(Array.from(document.links).filter(a => {
  const rect = a.getBoundingClientRect();
  const style = window.getComputedStyle(a);
  return a.href && a.innerText && rect.width > 0 && rect.height > 0 && style.visibility !== "hidden" && style.display !== "none";
}).slice(0, 200).map(a => ({ text: a.innerText.trim().replace(/\s+/g, " ").slice(0, 160), href: a.href })))
)JS");
    const QJsonDocument doc = QJsonDocument::fromJson(runJavaScriptVariantSync(view, script, 10000).toString().toUtf8());
    return doc.isArray() ? doc.array() : QJsonArray{};
}

void captureScreenshotTiles(QWebView &view, const QString &dir, QJsonObject &metadata)
{
    const int contentHeight = std::max(
        runJavaScriptVariantSync(view, QStringLiteral("Math.max(document.body.scrollHeight, document.documentElement.scrollHeight)")).toInt(),
        ScreenshotHeight);
    QJsonArray screenshots;
    int y = 0;
    int index = 0;
    while (index < MaxScreenshots && y < contentHeight) {
        runJavaScriptVariantSync(view, QStringLiteral("window.scrollTo(0, %1); true").arg(y), 5000);
        QEventLoop settleLoop;
        QTimer::singleShot(250, &settleLoop, &QEventLoop::quit);
        settleLoop.exec();

        QScreen *screen = view.screen() ? view.screen() : QGuiApplication::primaryScreen();
        if (!screen) {
            break;
        }
        const QPixmap pixmap = screen->grabWindow(view.winId());
        if (pixmap.isNull()) {
            break;
        }
        const QString fileName = QStringLiteral("screenshot-%1.png").arg(index + 1, 2, 10, QLatin1Char('0'));
        const QString path = QDir(dir).filePath(fileName);
        if (!pixmap.toImage().save(path)) {
            break;
        }
        screenshots.append(QJsonObject{
            {QStringLiteral("path"), path},
            {QStringLiteral("y"), y},
            {QStringLiteral("width"), pixmap.width()},
            {QStringLiteral("height"), pixmap.height()},
        });
        ++index;
        if (y + ScreenshotHeight >= contentHeight) {
            break;
        }
        y += ScreenshotHeight - ScreenshotOverlap;
    }
    metadata[QStringLiteral("screenshots")] = screenshots;
    metadata[QStringLiteral("nextScreenshotIndex")] = 0;
    metadata[QStringLiteral("contentHeight")] = contentHeight;
}

class ScriptHost : public QObject {
    Q_OBJECT
public:
    Q_INVOKABLE void print(const QString &value)
    {
        m_output.append(value);
    }

    QString output() const
    {
        return m_output.join('\n') + (m_output.isEmpty() ? QString() : QStringLiteral("\n"));
    }

private:
    QStringList m_output;
};

} // namespace

ToolExecutor::ToolExecutor(SettingsStore *settings, QString conversationId)
    : m_settings(settings)
    , m_conversationId(std::move(conversationId))
{
}

QJsonObject ToolExecutor::execute(const QString &name, const QJsonObject &arguments) const
{
    if (name == QStringLiteral("eval_javascript")) {
        return evalJavaScript(arguments);
    }
    if (name == QStringLiteral("open_web_page")) {
        return openPage(arguments);
    }
    if (name == QStringLiteral("read_web_page_text")) {
        return readPageText(arguments);
    }
    if (name == QStringLiteral("get_next_web_page_screenshot")) {
        return getNextScreenshot(arguments);
    }
    return toolError(QStringLiteral("Unknown tool '%1'").arg(name));
}

QJsonObject ToolExecutor::openPage(const QJsonObject &arguments) const
{
    if (!qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        return toolError(QStringLiteral("open_web_page requires the MyChatty GUI app; Qt WebView is not available in this process"));
    }
    const QString urlText = arguments.value(QStringLiteral("url")).toString().trimmed();
    const QUrl url(urlText);
    if (!url.isValid() || (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))) {
        return toolError(QStringLiteral("url must be a valid http(s) URL"));
    }

    const QString conversationId = m_conversationId.isEmpty() ? QStringLiteral("adhoc") : m_conversationId;
    const QString pageId = normalizedPageId(url.toString(QUrl::FullyEncoded));
    const QString dir = pageDir(conversationId, pageId);

    QtWebView::initialize();
    QWebView view;
    view.resize(ScreenshotWidth, ScreenshotHeight);
    view.setFlags(Qt::Tool | Qt::FramelessWindowHint);
    view.setPosition(-20000, -20000);
    view.setHttpUserAgentString(QStringLiteral("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/18.0 Safari/605.1.15"));
    view.settings()->setAttribute(QWebViewSettings::WebAttribute::JavaScriptEnabled, true);
    view.deleteAllCookies();
    view.show();

    if (!loadPageSync(view, url)) {
        return toolError(QStringLiteral("Timed out loading page"));
    }

    const QString visibleText = runJavaScriptVariantSync(
        view,
        QStringLiteral("document.body ? document.body.innerText : ''"),
        10000).toString();
    const QString articleText = readabilityText(view);
    const QString chosenText = articleText.trimmed().isEmpty() ? visibleText : articleText;
    const QStringList lines = textLines(chosenText);
    const QJsonArray links = visibleLinks(view);

    writeTextFile(QDir(dir).filePath(QStringLiteral("readability.txt")), articleText);
    writeTextFile(QDir(dir).filePath(QStringLiteral("visible.txt")), visibleText);
    writeTextFile(QDir(dir).filePath(QStringLiteral("text.txt")), chosenText);

    QJsonObject metadata{
        {QStringLiteral("page_id"), pageId},
        {QStringLiteral("requested_url"), urlText},
        {QStringLiteral("final_url"), view.url().toString()},
        {QStringLiteral("title"), view.title()},
        {QStringLiteral("text_source"), articleText.trimmed().isEmpty() ? QStringLiteral("visible_text") : QStringLiteral("readability_text")},
        {QStringLiteral("line_count"), lines.size()},
        {QStringLiteral("link_count"), links.size()},
        {QStringLiteral("links"), links},
        {QStringLiteral("screenshots_enabled"), m_settings && m_settings->pageScreenshotsEnabled()},
        {QStringLiteral("provenance"), arguments.value(QStringLiteral("url_provenance")).toString(QStringLiteral("model_constructed"))},
    };
    if (m_settings && m_settings->pageScreenshotsEnabled()) {
        captureScreenshotTiles(view, dir, metadata);
    }
    writeJsonFile(QDir(dir).filePath(QStringLiteral("metadata.json")), metadata);

    return toolOkObject(QJsonObject{
        {QStringLiteral("page_id"), pageId},
        {QStringLiteral("title"), view.title()},
        {QStringLiteral("final_url"), view.url().toString()},
        {QStringLiteral("text_source"), metadata.value(QStringLiteral("text_source"))},
        {QStringLiteral("line_count"), lines.size()},
        {QStringLiteral("link_count"), links.size()},
        {QStringLiteral("links"), firstJsonArrayItems(links, 80)},
        {QStringLiteral("screenshots_enabled"), metadata.value(QStringLiteral("screenshots_enabled"))},
        {QStringLiteral("screenshot_count"), metadata.value(QStringLiteral("screenshots")).toArray().size()},
        {QStringLiteral("text"), lineSlice(lines, 0, PreviewLines)},
    });
}

QJsonObject ToolExecutor::readPageText(const QJsonObject &arguments) const
{
    const QString pageId = arguments.value(QStringLiteral("page_id")).toString();
    if (pageId.trimmed().isEmpty()) {
        return toolError(QStringLiteral("page_id is required"));
    }
    const QString dir = pageDir(m_conversationId.isEmpty() ? QStringLiteral("adhoc") : m_conversationId, pageId);
    const QJsonObject metadata = readJsonFile(QDir(dir).filePath(QStringLiteral("metadata.json")));
    if (metadata.isEmpty()) {
        return toolError(QStringLiteral("Unknown page_id"));
    }
    const QString text = readTextFile(QDir(dir).filePath(QStringLiteral("text.txt")));
    const QStringList lines = textLines(text);
    const int offset = arguments.value(QStringLiteral("offset")).toInt(0);
    const int limit = arguments.value(QStringLiteral("limit")).toInt(DefaultTextLimit);
    const int normalizedLimit = std::clamp(limit, 1, MaxTextLimit);

    return toolOkObject(QJsonObject{
        {QStringLiteral("page_id"), pageId},
        {QStringLiteral("offset"), std::clamp(offset, 0, static_cast<int>(lines.size()))},
        {QStringLiteral("limit"), normalizedLimit},
        {QStringLiteral("line_count"), lines.size()},
        {QStringLiteral("text"), lineSlice(lines, offset, normalizedLimit)},
    });
}

QJsonObject ToolExecutor::getNextScreenshot(const QJsonObject &arguments) const
{
    if (!m_settings || !m_settings->pageScreenshotsEnabled()) {
        return toolError(QStringLiteral("Page screenshots are disabled in Advanced Settings"));
    }
    const QString pageId = arguments.value(QStringLiteral("page_id")).toString();
    if (pageId.trimmed().isEmpty()) {
        return toolError(QStringLiteral("page_id is required"));
    }
    const QString dir = pageDir(m_conversationId.isEmpty() ? QStringLiteral("adhoc") : m_conversationId, pageId);
    QJsonObject metadata = readJsonFile(QDir(dir).filePath(QStringLiteral("metadata.json")));
    if (metadata.isEmpty()) {
        return toolError(QStringLiteral("Unknown page_id"));
    }
    const QJsonArray screenshots = metadata.value(QStringLiteral("screenshots")).toArray();
    int index = metadata.value(QStringLiteral("nextScreenshotIndex")).toInt(0);
    if (index >= screenshots.size()) {
        return toolOkObject(QJsonObject{
            {QStringLiteral("page_id"), pageId},
            {QStringLiteral("done"), true},
            {QStringLiteral("remaining"), 0},
        });
    }
    const QJsonObject screenshot = screenshots.at(index).toObject();
    const QString path = screenshot.value(QStringLiteral("path")).toString();
    metadata[QStringLiteral("nextScreenshotIndex")] = index + 1;
    writeJsonFile(QDir(dir).filePath(QStringLiteral("metadata.json")), metadata);
    return toolOkObject(QJsonObject{
        {QStringLiteral("page_id"), pageId},
        {QStringLiteral("done"), false},
        {QStringLiteral("index"), index},
        {QStringLiteral("remaining"), screenshots.size() - index - 1},
        {QStringLiteral("path"), path},
        {QStringLiteral("image_path"), path},
        {QStringLiteral("image_mime_type"), QStringLiteral("image/png")},
        {QStringLiteral("image_detail"), QStringLiteral("high")},
        {QStringLiteral("y"), screenshot.value(QStringLiteral("y"))},
        {QStringLiteral("width"), screenshot.value(QStringLiteral("width"))},
        {QStringLiteral("height"), screenshot.value(QStringLiteral("height"))},
    });
}

QJsonObject ToolExecutor::evalJavaScript(const QJsonObject &arguments) const
{
    Q_UNUSED(m_settings);
    const QString source = arguments.value("javascript_source_code").toString();
    if (source.trimmed().isEmpty()) {
        return toolError(QStringLiteral("javascript_source_code is required"));
    }

    QJSEngine engine;
    ScriptHost host;
    ConversationFileStore fs(m_conversationId.isEmpty() ? QStringLiteral("adhoc") : m_conversationId);
    engine.globalObject().setProperty(QStringLiteral("__host"), engine.newQObject(&host));
    engine.globalObject().setProperty(QStringLiteral("fs"), engine.newQObject(&fs));
    engine.evaluate(QStringLiteral(
        "this.print = function() {"
        "  __host.print(Array.prototype.map.call(arguments, function(value) {"
        "    return typeof value === 'string' ? value : JSON.stringify(value);"
        "  }).join(' '));"
        "};"
        "this.require = undefined;"
        "this.importScripts = undefined;"
    ));

    std::atomic_bool finished = false;
    std::mutex watchdogMutex;
    std::condition_variable watchdogCondition;
    std::thread watchdog([&engine, &finished, &watchdogMutex, &watchdogCondition]() {
        std::unique_lock<std::mutex> lock(watchdogMutex);
        const bool completed = watchdogCondition.wait_for(lock, std::chrono::seconds(2), [&finished]() {
            return finished.load();
        });
        if (!completed) {
            engine.setInterrupted(true);
        }
    });

    const QJSValue result = engine.evaluate(source, QStringLiteral("model-tool.js"));
    finished.store(true);
    watchdogCondition.notify_one();
    if (watchdog.joinable()) {
        watchdog.join();
    }
    if (result.isError()) {
        return toolError(QStringLiteral("%1:%2: %3")
                             .arg(result.property(QStringLiteral("fileName")).toString())
                             .arg(result.property(QStringLiteral("lineNumber")).toInt())
                             .arg(result.toString()));
    }
    QString prints = host.output();
    if (prints.isEmpty() && !result.isUndefined()) {
        prints = result.toString() + QStringLiteral("\n");
    }
    return toolOk(prints);
}

} // namespace MyChatty

#include "ToolExecutor.moc"
