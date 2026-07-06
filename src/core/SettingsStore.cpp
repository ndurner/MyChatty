#include "SettingsStore.h"

#include <QSettings>

namespace MyChatty {

SettingsStore::SettingsStore(QObject *parent)
    : QObject(parent)
{
    reload();
}

QString SettingsStore::openAIKey() const
{
    return m_openAIKey;
}

QString SettingsStore::openRouterKey() const
{
    return m_openRouterKey;
}

QString SettingsStore::exaKey() const
{
    return m_exaKey;
}

QString SettingsStore::openRouterPdfEngine() const
{
    return m_openRouterPdfEngine;
}

QString SettingsStore::customInstructions() const
{
    return m_customInstructions;
}

bool SettingsStore::webSearchEnabled() const
{
    return m_webSearchEnabled;
}

bool SettingsStore::exaSearchEnabled() const
{
    return m_exaSearchEnabled;
}

bool SettingsStore::javaScriptUseEnabled() const
{
    return m_javaScriptUseEnabled;
}

void SettingsStore::setOpenAIKey(const QString &value)
{
    if (m_openAIKey == value) {
        return;
    }
    m_openAIKey = value;
    emit openAIKeyChanged();
}

void SettingsStore::setOpenRouterKey(const QString &value)
{
    if (m_openRouterKey == value) {
        return;
    }
    m_openRouterKey = value;
    emit openRouterKeyChanged();
}

void SettingsStore::setExaKey(const QString &value)
{
    if (m_exaKey == value) {
        return;
    }
    m_exaKey = value;
    emit exaKeyChanged();
}

void SettingsStore::setOpenRouterPdfEngine(const QString &value)
{
    const QString normalized = value == QStringLiteral("native") ? value : QStringLiteral("cloudflare-ai");
    if (m_openRouterPdfEngine == normalized) {
        return;
    }
    m_openRouterPdfEngine = normalized;
    emit openRouterPdfEngineChanged();
}

void SettingsStore::setCustomInstructions(const QString &value)
{
    if (m_customInstructions == value) {
        return;
    }
    m_customInstructions = value;
    emit customInstructionsChanged();
}

void SettingsStore::setWebSearchEnabled(bool value)
{
    if (m_webSearchEnabled == value) {
        return;
    }
    m_webSearchEnabled = value;
    emit webSearchEnabledChanged();
}

void SettingsStore::setExaSearchEnabled(bool value)
{
    if (m_exaSearchEnabled == value) {
        return;
    }
    m_exaSearchEnabled = value;
    emit exaSearchEnabledChanged();
}

void SettingsStore::setJavaScriptUseEnabled(bool value)
{
    if (m_javaScriptUseEnabled == value) {
        return;
    }
    m_javaScriptUseEnabled = value;
    emit javaScriptUseEnabledChanged();
}

void SettingsStore::save()
{
    QSettings settings;
    settings.setValue("openAIKey", m_openAIKey);
    settings.setValue("openRouterKey", m_openRouterKey);
    settings.setValue("exaKey", m_exaKey);
    settings.setValue("openRouterPdfEngine", m_openRouterPdfEngine);
    settings.setValue("customInstructions", m_customInstructions);
    settings.setValue("webSearchEnabled", m_webSearchEnabled);
    settings.setValue("exaSearchEnabled", m_exaSearchEnabled);
    settings.setValue("javaScriptUseEnabled", m_javaScriptUseEnabled);
}

void SettingsStore::reload()
{
    QSettings settings;
    m_openAIKey = settings.value("openAIKey").toString();
    m_openRouterKey = settings.value("openRouterKey").toString();
    m_exaKey = settings.value("exaKey").toString();
    m_openRouterPdfEngine = settings.value("openRouterPdfEngine", QStringLiteral("cloudflare-ai")).toString();
    if (m_openRouterPdfEngine != QStringLiteral("native")) {
        m_openRouterPdfEngine = QStringLiteral("cloudflare-ai");
    }
    m_customInstructions = settings.value("customInstructions").toString();
    m_webSearchEnabled = settings.value("webSearchEnabled", true).toBool();
    m_exaSearchEnabled = settings.value("exaSearchEnabled", false).toBool();
    m_javaScriptUseEnabled = settings.value("javaScriptUseEnabled", false).toBool();
    emit openAIKeyChanged();
    emit openRouterKeyChanged();
    emit exaKeyChanged();
    emit openRouterPdfEngineChanged();
    emit customInstructionsChanged();
    emit webSearchEnabledChanged();
    emit exaSearchEnabledChanged();
    emit javaScriptUseEnabledChanged();
}

} // namespace MyChatty
