#include "WebPageText.h"

#include <QRegularExpression>

namespace MyChatty {
namespace {

QString normalizedMarkdown(QString markdown)
{
    markdown.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    markdown.replace('\r', '\n');
    markdown.replace(QRegularExpression(QStringLiteral("[ \\t]+(?=\\n|$)")), QString());
    markdown.replace(QRegularExpression(QStringLiteral("\\n{3,}")), QStringLiteral("\n\n"));
    return markdown.trimmed();
}

} // namespace

QString finalizeRenderedMarkdown(const QString &title, const QString &bodyMarkdown)
{
    const QString body = normalizedMarkdown(bodyMarkdown);
    if (body.isEmpty()) {
        return {};
    }

    QStringList sections;
    if (!title.trimmed().isEmpty()) {
        sections.append(QStringLiteral("# %1").arg(title.trimmed()));
    }
    sections.append(body);
    return normalizedMarkdown(sections.join(QStringLiteral("\n\n")));
}

} // namespace MyChatty
