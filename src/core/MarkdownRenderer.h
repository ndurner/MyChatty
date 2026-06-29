#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

namespace MyChatty {

class MarkdownRenderer : public QObject {
    Q_OBJECT
public:
    explicit MarkdownRenderer(QObject *parent = nullptr);

    Q_INVOKABLE QString toHtml(const QString &markdown) const;
    static QString render(const QString &markdown);
    static QVariantList renderBlocks(const QString &markdown);
};

} // namespace MyChatty
