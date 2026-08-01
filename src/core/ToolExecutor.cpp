#include "ToolExecutor.h"

#include "ConversationFileStore.h"
#include "WebPageText.h"

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

struct RenderedPageSnapshot {
    QString rawBodyHtml;
    QString bodyHtml;
    QString bodyMarkdown;
    QString conversionError;
    int removedNodeCount = 0;
};

RenderedPageSnapshot renderedPageSnapshot(QWebView &view)
{
    const QString converter = readTextFile(QStringLiteral(":/third_party/turndown/turndown.js"))
        + QLatin1Char('\n')
        + readTextFile(QStringLiteral(":/third_party/turndown/turndown-plugin-gfm.js"));
    if (converter.trimmed().isEmpty()) {
        return {{}, {}, {}, QStringLiteral("Markdown converter resources are unavailable"), 0};
    }
    const QString script = converter + QStringLiteral(R"JS(
(() => {
  if (!document.body) return JSON.stringify({ conversionError: "Page has no document body" });
  const clone = document.body.cloneNode(true);
  const rawBodyHtml = clone.innerHTML;
  let removedNodeCount = 0;
  const removeAll = selector => clone.querySelectorAll(selector).forEach(node => {
    node.remove();
    removedNodeCount += 1;
  });
  removeAll([
    "script", "style", "noscript", "template", "svg", "canvas", "link", "meta",
    "nav", "footer", "aside", "form", "fieldset", "object", "embed", "iframe",
    "input", "textarea", "select", "button",
    ".material-icons", ".material-icons-outlined", ".material-symbols-outlined",
    ".material-symbols-rounded", ".google-symbols",
    "[hidden]", "[aria-hidden='true']",
    "[style*='display: none']", "[style*='display:none']",
    "[style*='visibility: hidden']", "[style*='visibility:hidden']",
    "[role='navigation']", "[role='banner']", "[role='contentinfo']",
    "[role='dialog']", "[role='alertdialog']", "[role='search']",
    "[role='menu']", "[role='menuitem']", "[role='toolbar']", "[role='tooltip']",
    "[role='complementary']"
  ].join(","));
  clone.querySelectorAll("header").forEach(node => {
    if (!node.closest("main, article")) {
      node.remove();
      removedNodeCount += 1;
    }
  });
  const explicitUiName = /(?:^|[^a-z0-9])(dialog|jump[-_\s]?links?|menu|navigation|popup|sidebar|skip[-_\s]?links?)(?=$|[^a-z0-9])/i;
  const ancillaryContentName = /(?:^|[^a-z0-9])((?:content|journey)[-_\s]?recs?|feedback[a-z]*|highly[-_\s]?rated|recommend(?:ation|ed)?s?|related|similar)(?=$|[^a-z0-9])/i;
  const unlikelyName = /(?:^|[-_\s])(ad-break|agegate|banner|breadcrumbs|comment|community|disqus|extra|footer|gdpr|legends|menu|pager|pagination|popup|related|remark|replies|rss|share|shoutbox|sidebar|skyscraper|social|sponsor|supplemental)(?:$|[-_\s])/i;
  const likelyContentName = /(?:^|[-_\s])(article|body|content|entry|hentry|main|page|post|story|text)(?:$|[-_\s])/i;
  const linkDensity = node => {
    const textLength = (node.textContent || "").trim().length;
    if (!textLength) return 0;
    const linkLength = Array.from(node.querySelectorAll("a"))
      .reduce((length, link) => length + (link.textContent || "").trim().length, 0);
    return linkLength / textLength;
  };
  const candidateName = node => ["class", "id", "aria-label", "data-testid"]
    .map(attribute => node.getAttribute(attribute) || "").join(" ");
  clone.querySelectorAll("[class], [id], [aria-label], [data-testid]").forEach(node => {
    if (!node.parentNode || !ancillaryContentName.test(candidateName(node))) return;
    let listItem = node.parentElement;
    while (listItem && listItem.tagName !== "LI") listItem = listItem.parentElement;
    (listItem || node).remove();
    removedNodeCount += 1;
  });
  clone.querySelectorAll("[class], [id], [aria-label], [data-testid]").forEach(node => {
    if (!node.parentNode) return;
    const name = candidateName(node);
    if (ancillaryContentName.test(name)) return;
    const explicitUi = explicitUiName.test(name);
    const likelyContent = likelyContentName.test(name);
    const semanticContent = node.matches("main, article, h1, h2, h3, h4, h5, h6");
    const density = linkDensity(node);
    const conflictedUi = explicitUi && likelyContent && density >= 0.5;
    const conflictedUnlikely = unlikelyName.test(name) && likelyContent && density >= 0.2;
    if ((explicitUi && !semanticContent && (!likelyContent || conflictedUi))
        || (unlikelyName.test(name) && (!likelyContent || conflictedUnlikely))) {
      node.remove();
      removedNodeCount += 1;
    }
  });
  clone.querySelectorAll("li").forEach(node => {
    if (!node.textContent.trim() && !node.querySelector("img, video, audio")) {
      node.remove();
      removedNodeCount += 1;
    }
  });
  clone.querySelectorAll("ul, ol").forEach(node => {
    if (!node.textContent.trim() && !node.querySelector("img, video, audio")) {
      node.remove();
      removedNodeCount += 1;
    }
  });
  // turndown-plugin-gfm recognizes a header row only when its tbody is the
  // table's first child. Colgroups are presentation metadata, but otherwise
  // cause valid data tables to be emitted as raw HTML.
  removeAll("colgroup");
  const normalizedTableHeader = table => {
    const row = table.rows && table.rows[0];
    return row ? Array.from(row.cells)
      .map(cell => (cell.textContent || "").trim().replace(/\s+/g, " "))
      .join("\u001f") : "";
  };
  const tables = Array.from(clone.querySelectorAll("table"));
  tables.forEach((table, index) => {
    if (!table.parentNode || table.rows.length !== 1) return;
    const header = normalizedTableHeader(table);
    if (!header) return;
    const duplicate = tables.slice(index + 1).some(candidate =>
      candidate.rows.length > 1 && normalizedTableHeader(candidate) === header);
    if (duplicate) {
      table.remove();
      removedNodeCount += 1;
    }
  });
  clone.querySelectorAll("[href]").forEach(node => {
    try { node.setAttribute("href", new URL(node.getAttribute("href"), document.baseURI).href); } catch (_) {}
  });
  clone.querySelectorAll("[src]").forEach(node => {
    try { node.setAttribute("src", new URL(node.getAttribute("src"), document.baseURI).href); } catch (_) {}
  });
  const bodyHtml = clone.innerHTML;
  let bodyMarkdown = "";
  let conversionError = "";
  try {
    const service = new TurndownService({
      headingStyle: "atx",
      bulletListMarker: "-",
      codeBlockStyle: "fenced",
      fence: "```",
      emDelimiter: "*",
      strongDelimiter: "**"
    });
    service.use(turndownPluginGfm.gfm);
    const hasMarkdownHeader = table => {
      const firstRow = table && table.rows && table.rows[0];
      return !!firstRow && Array.from(firstRow.cells)
        .every(cell => cell.tagName === "TH");
    };
    const compactCellText = content => content
      .replace(/\|/g, "\\|")
      .replace(/\s+/g, " ")
      .trim();
    // Jina's Markify follows the same rule: table-cell contents are compacted
    // onto one line. Turndown GFM otherwise leaves block-child newlines in a
    // cell, producing invalid Markdown for common documentation tables.
    service.addRule("compactGfmTableCell", {
      filter: node => (node.tagName === "TH" || node.tagName === "TD")
        && hasMarkdownHeader(node.closest("table")),
      replacement: (content, node) => {
        const prefix = node.cellIndex === 0 ? "| " : " ";
        return prefix + compactCellText(content) + " |";
      }
    });
    // GFM has no syntax for a table without a heading row. Follow Markify's
    // non-lossy fallback and emit each row as compact plain text instead of
    // preserving a large raw-HTML block.
    service.addRule("plainTableCell", {
      filter: node => (node.tagName === "TH" || node.tagName === "TD")
        && !hasMarkdownHeader(node.closest("table")),
      replacement: (content, node) => compactCellText(content)
        + (node.cellIndex + 1 < node.parentElement.cells.length ? " | " : "")
    });
    service.addRule("plainTableRow", {
      filter: node => node.tagName === "TR"
        && !hasMarkdownHeader(node.closest("table")),
      replacement: content => "\n" + content.trim() + "\n"
    });
    service.addRule("plainTable", {
      filter: node => node.tagName === "TABLE" && !hasMarkdownHeader(node),
      replacement: content => "\n\n" + content.trim() + "\n\n"
    });
    bodyMarkdown = service.turndown(clone);
  } catch (error) {
    conversionError = String(error && error.message ? error.message : error);
  }
  return JSON.stringify({ rawBodyHtml, bodyHtml, bodyMarkdown, conversionError, removedNodeCount });
})()
)JS");
    const QJsonDocument result = QJsonDocument::fromJson(
        runJavaScriptVariantSync(view, script, 10000).toString().toUtf8());
    if (!result.isObject()) {
        return {};
    }
    const QJsonObject object = result.object();
    return {
        object.value(QStringLiteral("rawBodyHtml")).toString(),
        object.value(QStringLiteral("bodyHtml")).toString(),
        object.value(QStringLiteral("bodyMarkdown")).toString(),
        object.value(QStringLiteral("conversionError")).toString(),
        object.value(QStringLiteral("removedNodeCount")).toInt(),
    };
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

    const RenderedPageSnapshot snapshot = renderedPageSnapshot(view);
    const QString chosenText = finalizeRenderedMarkdown(view.title(), snapshot.bodyMarkdown);
    writeTextFile(QDir(dir).filePath(QStringLiteral("rendered-raw.html")), snapshot.rawBodyHtml);
    writeTextFile(QDir(dir).filePath(QStringLiteral("rendered.html")), snapshot.bodyHtml);
    if (chosenText.trimmed().isEmpty()) {
        const bool pageWasEmpty = snapshot.bodyHtml.trimmed().isEmpty();
        const QString errorCode = pageWasEmpty
            ? QStringLiteral("no_extractable_dom")
            : QStringLiteral("markdown_conversion_failed");
        QString error = pageWasEmpty
            ? QStringLiteral("The rendered page exposed no extractable DOM content")
            : QStringLiteral("Could not convert the rendered page DOM to Markdown");
        if (!snapshot.conversionError.isEmpty()) {
            error += QStringLiteral(": %1").arg(snapshot.conversionError);
        }
        return QJsonObject{
            {QStringLiteral("success"), false},
            {QStringLiteral("error"), error},
            {QStringLiteral("error_code"), errorCode},
            {QStringLiteral("text_source"), QStringLiteral("rendered_dom_turndown")},
            {QStringLiteral("text_cleanup"), QStringLiteral("readability_boilerplate_v1")},
            {QStringLiteral("markdown_converter"), QStringLiteral("turndown_7.2.0_gfm_1.0.2")},
            {QStringLiteral("removed_node_count"), snapshot.removedNodeCount},
        };
    }
    const QStringList lines = textLines(chosenText);
    const QJsonArray links = visibleLinks(view);

    writeTextFile(QDir(dir).filePath(QStringLiteral("rendered.md")), chosenText);
    writeTextFile(QDir(dir).filePath(QStringLiteral("text.txt")), chosenText);

    QJsonObject metadata{
        {QStringLiteral("page_id"), pageId},
        {QStringLiteral("requested_url"), urlText},
        {QStringLiteral("final_url"), view.url().toString()},
        {QStringLiteral("title"), view.title()},
        {QStringLiteral("text_source"), QStringLiteral("rendered_dom_turndown")},
        {QStringLiteral("text_cleanup"), QStringLiteral("readability_boilerplate_v1")},
        {QStringLiteral("markdown_converter"), QStringLiteral("turndown_7.2.0_gfm_1.0.2")},
        {QStringLiteral("removed_node_count"), snapshot.removedNodeCount},
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
        {QStringLiteral("text_cleanup"), metadata.value(QStringLiteral("text_cleanup"))},
        {QStringLiteral("markdown_converter"), metadata.value(QStringLiteral("markdown_converter"))},
        {QStringLiteral("removed_node_count"), metadata.value(QStringLiteral("removed_node_count"))},
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
