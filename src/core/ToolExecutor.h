#pragma once

#include "SettingsStore.h"

#include <QJsonObject>
#include <QObject>
#include <QStringList>

namespace MyChatty {

class ToolExecutor {
public:
    explicit ToolExecutor(SettingsStore *settings = nullptr, QString conversationId = {});

    QJsonObject execute(const QString &name, const QJsonObject &arguments) const;

private:
    QJsonObject evalJavaScript(const QJsonObject &arguments) const;
    QJsonObject openPage(const QJsonObject &arguments) const;
    QJsonObject readPageText(const QJsonObject &arguments) const;
    QJsonObject getNextScreenshot(const QJsonObject &arguments) const;

    SettingsStore *m_settings = nullptr;
    QString m_conversationId;
};

} // namespace MyChatty
