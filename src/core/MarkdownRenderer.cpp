#include "MarkdownRenderer.h"

#include <QRegularExpression>
#include <QStringList>

namespace MyChatty {

static QString escapeHtml(QString text)
{
    text.replace('&', "&amp;");
    text.replace('<', "&lt;");
    text.replace('>', "&gt;");
    return text;
}

static QString renderInline(QString text)
{
    text = escapeHtml(std::move(text));
    text.replace(QRegularExpression("&lt;br\\s*/?&gt;", QRegularExpression::CaseInsensitiveOption), "<br/>");

    // Protect code spans while applying the other inline rules. In particular,
    // underscores inside code must not be interpreted as emphasis markers.
    QStringList codeSpans;
    const QRegularExpression inlineCode("`([^`\\n]+)`");
    qsizetype codeOffset = 0;
    while (true) {
        const QRegularExpressionMatch match = inlineCode.match(text, codeOffset);
        if (!match.hasMatch()) {
            break;
        }
        const QString placeholder = QString("\u0001CODE%1\u0002").arg(codeSpans.size());
        codeSpans.append(match.captured(1));
        text.replace(match.capturedStart(), match.capturedLength(), placeholder);
        codeOffset = match.capturedStart() + placeholder.size();
    }
    text.replace(QRegularExpression("!\\[([^\\]]*)\\]\\((https?://[^\\s)]+)\\)"),
                 "<a href=\"\\2\">\\1</a>");
    text.replace(QRegularExpression("\\[([^\\]]+)\\]\\((https?://[^\\s)]+)\\)"),
                 "<a href=\"\\2\">\\1</a>");

    QRegularExpression bareUrl("(?<![\"'=])(https?://[^\\s<]+)");
    qsizetype offset = 0;
    while (true) {
        const QRegularExpressionMatch match = bareUrl.match(text, offset);
        if (!match.hasMatch()) {
            break;
        }

        QString url = match.captured(1);
        QString trailing;
        while (!url.isEmpty() && QStringView(u".,;:!?)]").contains(url.back())) {
            trailing.prepend(url.back());
            url.chop(1);
        }
        if (url.isEmpty()) {
            offset = match.capturedEnd(1);
            continue;
        }

        const QString replacement = QString("<a href=\"%1\">%1</a>%2").arg(url, trailing);
        text.replace(match.capturedStart(1), match.capturedLength(1), replacement);
        offset = match.capturedStart(1) + replacement.size();
    }

    text.replace(QRegularExpression("\\*\\*([^*]+)\\*\\*"), "<strong>\\1</strong>");
    text.replace(QRegularExpression("__([^_]+)__"), "<strong>\\1</strong>");
    text.replace(QRegularExpression("(?<!\\*)\\*([^*]+)\\*(?!\\*)"), "<em>\\1</em>");
    text.replace(QRegularExpression("(?<!_)_([^_]+)_(?!_)"), "<em>\\1</em>");
    text.replace(QRegularExpression("~~([^~]+)~~"), "<s>\\1</s>");

    for (qsizetype i = 0; i < codeSpans.size(); ++i) {
        text.replace(QString("\u0001CODE%1\u0002").arg(i),
                     "<span style=\"font-family:Menlo, Consolas, monospace; "
                     "background-color:#f1f1f1;\">" + codeSpans.at(i) + "</span>");
    }
    return text;
}

static QString renderParagraph(const QStringList &lines)
{
    if (lines.isEmpty()) {
        return {};
    }
    return "<p style=\"margin:0 0 14px 0;\">" + renderInline(lines.join(' ')) + "</p>";
}

static QString renderList(const QStringList &items, bool ordered)
{
    if (items.isEmpty()) {
        return {};
    }

    QString html = ordered ? "<ol style=\"margin:0 0 14px 24px;\">" : "<ul style=\"margin:0 0 14px 24px;\">";
    for (const QString &item : items) {
        html += "<li>" + renderInline(item) + "</li>";
    }
    html += ordered ? "</ol>" : "</ul>";
    return html;
}

static QVariantMap block(const QString &type)
{
    QVariantMap row;
    row["type"] = type;
    return row;
}

static int leadingSpaces(const QString &line)
{
    int count = 0;
    while (count < line.size() && line.at(count).isSpace() && line.at(count) != '\n') {
        ++count;
    }
    return count;
}

static bool looksLikeTableSeparator(const QString &line)
{
    const QStringList cells = line.trimmed().split('|', Qt::SkipEmptyParts);
    if (cells.isEmpty()) {
        return false;
    }
    for (const QString &cell : cells) {
        if (!QRegularExpression("^\\s*:?-{3,}:?\\s*$").match(cell).hasMatch()) {
            return false;
        }
    }
    return true;
}

static bool looksLikeTableRow(const QString &line)
{
    return line.contains('|') && line.trimmed().count('|') >= 2;
}

static QStringList splitTableCells(const QString &line)
{
    QString row = line.trimmed();
    if (row.startsWith('|')) {
        row.remove(0, 1);
    }
    if (row.endsWith('|')) {
        row.chop(1);
    }
    return row.split('|');
}

static QVariantList tableCells(const QStringList &cells)
{
    QVariantList values;
    for (const QString &cell : cells) {
        QVariantMap value;
        value["html"] = renderInline(cell.trimmed());
        values.append(value);
    }
    return values;
}

static bool shouldContinueTableRow(const QString &line)
{
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    if (looksLikeTableRow(line)) {
        return true;
    }
    if (trimmed.startsWith("```")
        || trimmed == "---"
        || trimmed == "***"
        || trimmed.startsWith('>')
        || QRegularExpression("^#{1,6}\\s+").match(trimmed).hasMatch()) {
        return false;
    }
    return true;
}

static QVariantMap parseTableRow(const QStringList &lines, int columnCount)
{
    QStringList cells = splitTableCells(lines.value(0));
    while (cells.size() < columnCount) {
        cells.append(QString());
    }

    for (int i = 1; i < lines.size(); ++i) {
        const QString continuation = lines.at(i).trimmed();
        if (continuation.isEmpty()) {
            continue;
        }
        const int target = qMax(0, qMin(cells.size() - 1, columnCount - 1));
        if (!cells[target].trimmed().isEmpty()) {
            cells[target] += QStringLiteral("<br>");
        }
        cells[target] += continuation;
    }

    if (cells.size() > columnCount) {
        QString overflow;
        while (cells.size() > columnCount) {
            if (!overflow.isEmpty()) {
                overflow.prepend(QStringLiteral(" | "));
            }
            overflow.prepend(cells.takeLast());
        }
        cells[columnCount - 1] += QStringLiteral(" | ") + overflow;
    }

    QVariantMap row;
    row["cells"] = tableCells(cells);
    return row;
}

static QVariantList parseBlocks(const QString &markdown)
{
    QVariantList blocks;
    QStringList paragraph;
    QVariantList listItems;
    bool listOrdered = false;
    bool inCodeFence = false;
    QString code;

    auto flushParagraph = [&]() {
        if (paragraph.isEmpty()) {
            return;
        }
        QVariantMap row = block("paragraph");
        row["html"] = renderInline(paragraph.join(' '));
        blocks.append(row);
        paragraph.clear();
    };
    auto flushList = [&]() {
        if (listItems.isEmpty()) {
            return;
        }
        QVariantMap row = block("list");
        row["ordered"] = listOrdered;
        row["items"] = listItems;
        blocks.append(row);
        listItems.clear();
    };
    auto flushBlocks = [&]() {
        flushParagraph();
        flushList();
    };

    const QStringList lines = markdown.split('\n');
    for (int lineIndex = 0; lineIndex < lines.size(); ++lineIndex) {
        QString line = lines.at(lineIndex);
        const QString fenceCandidate = line.trimmed();
        const bool tripleFence = fenceCandidate.startsWith("```");
        const bool doubleFence = !tripleFence
            && QRegularExpression("^``[A-Za-z0-9_+.-]*$").match(fenceCandidate).hasMatch();
        if (tripleFence || doubleFence || (inCodeFence && fenceCandidate == "``")) {
            if (inCodeFence) {
                QVariantMap row = block("code");
                row["text"] = code.trimmed();
                blocks.append(row);
                code.clear();
                inCodeFence = false;
            } else {
                flushBlocks();
                inCodeFence = true;
            }
            continue;
        }

        if (inCodeFence) {
            code += line + '\n';
            continue;
        }

        const QString trimmed = line.trimmed();
        if (trimmed.compare("<pre>", Qt::CaseInsensitive) == 0
            || trimmed.compare("</pre>", Qt::CaseInsensitive) == 0) {
            flushBlocks();
            continue;
        }
        if (trimmed.isEmpty()) {
            flushBlocks();
            continue;
        }

        if (trimmed == "---" || trimmed == "***") {
            flushBlocks();
            blocks.append(block("rule"));
            continue;
        }

        int headingLevel = 0;
        while (headingLevel < trimmed.size() && headingLevel < 6 && trimmed.at(headingLevel) == '#') {
            ++headingLevel;
        }
        if (headingLevel > 0 && headingLevel < trimmed.size() && trimmed.at(headingLevel).isSpace()) {
            flushBlocks();
            QVariantMap row = block("heading");
            row["level"] = headingLevel;
            row["html"] = renderInline(trimmed.mid(headingLevel).trimmed());
            blocks.append(row);
            continue;
        }

        if (looksLikeTableRow(line) && lineIndex + 1 < lines.size()
            && looksLikeTableSeparator(lines.at(lineIndex + 1))) {
            flushBlocks();
            QVariantMap row = block("table");
            const QStringList headers = splitTableCells(line);
            row["headers"] = tableCells(headers);
            const int columnCount = qMax(1, headers.size());
            QVariantList bodyRows;
            lineIndex += 2;
            while (lineIndex < lines.size() && shouldContinueTableRow(lines.at(lineIndex))) {
                if (!looksLikeTableRow(lines.at(lineIndex))) {
                    break;
                }

                QStringList rowLines{lines.at(lineIndex)};
                ++lineIndex;
                while (lineIndex < lines.size()
                       && shouldContinueTableRow(lines.at(lineIndex))
                       && !looksLikeTableRow(lines.at(lineIndex))) {
                    rowLines.append(lines.at(lineIndex));
                    ++lineIndex;
                }
                bodyRows.append(parseTableRow(rowLines, columnCount));
            }
            --lineIndex;
            row["rows"] = bodyRows;
            blocks.append(row);
            continue;
        }

        const QRegularExpressionMatch unordered = QRegularExpression("^[-*+]\\s+(.*)$").match(trimmed);
        if (unordered.hasMatch()) {
            flushParagraph();
            listOrdered = false;
            QVariantMap item;
            item["html"] = renderInline(unordered.captured(1));
            item["level"] = leadingSpaces(line) / 2;
            item["ordered"] = false;
            listItems.append(item);
            continue;
        }

        const QRegularExpressionMatch ordered = QRegularExpression("^\\d+[.)]\\s+(.*)$").match(trimmed);
        if (ordered.hasMatch()) {
            flushParagraph();
            listOrdered = true;
            QVariantMap item;
            item["html"] = renderInline(ordered.captured(1));
            item["level"] = leadingSpaces(line) / 2;
            item["ordered"] = true;
            listItems.append(item);
            continue;
        }

        if (trimmed.startsWith('>')) {
            flushBlocks();
            QVariantMap row = block("quote");
            row["html"] = renderInline(trimmed.mid(1).trimmed());
            blocks.append(row);
            continue;
        }

        flushList();
        paragraph.append(trimmed);
    }

    if (inCodeFence) {
        QVariantMap row = block("code");
        row["text"] = code.trimmed();
        blocks.append(row);
    }
    flushBlocks();
    return blocks;
}

MarkdownRenderer::MarkdownRenderer(QObject *parent)
    : QObject(parent)
{
}

QString MarkdownRenderer::toHtml(const QString &markdown) const
{
    return render(markdown);
}

QString MarkdownRenderer::render(const QString &markdown)
{
    QString html;
    const QVariantList blocks = parseBlocks(markdown);
    for (const QVariant &value : blocks) {
        const QVariantMap row = value.toMap();
        const QString type = row.value("type").toString();
        if (type == "paragraph") {
            html += "<p style=\"margin:0 0 14px 0;\">" + row.value("html").toString() + "</p>";
        } else if (type == "heading") {
            const int level = row.value("level").toInt();
            const int size = qMax(18, 27 - level * 2);
            html += QString("<h%1 style=\"margin:18px 0 8px 0; font-size:%2px; font-weight:700;\">%3</h%1>")
                        .arg(level)
                        .arg(size)
                        .arg(row.value("html").toString());
        } else if (type == "code") {
            html += "<pre style=\"margin:0 0 14px 0; padding:12px; background-color:#f4f4f4; "
                    "border:1px solid #e5e5e5; border-radius:12px; white-space:pre-wrap;\">"
                    "<code style=\"font-family:Menlo, Consolas, monospace;\">"
                    + escapeHtml(row.value("text").toString()) + "</code></pre>";
        } else if (type == "rule") {
            html += "<hr style=\"border:0; border-top:1px solid #e4e4e4; margin:16px 0;\"/>";
        } else if (type == "quote") {
            html += "<blockquote style=\"margin:0 0 14px 0; padding-left:12px; color:#555555; "
                    "border-left:3px solid #dddddd;\">"
                    + row.value("html").toString() + "</blockquote>";
        } else if (type == "list") {
            const bool ordered = row.value("ordered").toBool();
            html += ordered ? "<ol style=\"margin:0 0 14px 24px;\">" : "<ul style=\"margin:0 0 14px 24px;\">";
            for (const QVariant &item : row.value("items").toList()) {
                html += "<li>" + item.toMap().value("html").toString() + "</li>";
            }
            html += ordered ? "</ol>" : "</ul>";
        } else if (type == "table") {
            html += "<table>";
            html += "<tr>";
            for (const QVariant &header : row.value("headers").toList()) {
                html += "<th>" + header.toMap().value("html").toString() + "</th>";
            }
            html += "</tr>";
            for (const QVariant &bodyRow : row.value("rows").toList()) {
                html += "<tr>";
                for (const QVariant &cell : bodyRow.toMap().value("cells").toList()) {
                    html += "<td>" + cell.toMap().value("html").toString() + "</td>";
                }
                html += "</tr>";
            }
            html += "</table>";
        }
    }
    return html;
}

QVariantList MarkdownRenderer::renderBlocks(const QString &markdown)
{
    return parseBlocks(markdown);
}

} // namespace MyChatty
