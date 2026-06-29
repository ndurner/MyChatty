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

QString SettingsStore::customInstructions() const
{
    return m_customInstructions;
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

void SettingsStore::setCustomInstructions(const QString &value)
{
    if (m_customInstructions == value) {
        return;
    }
    m_customInstructions = value;
    emit customInstructionsChanged();
}

void SettingsStore::save()
{
    QSettings settings;
    settings.setValue("openAIKey", m_openAIKey);
    settings.setValue("openRouterKey", m_openRouterKey);
    settings.setValue("customInstructions", m_customInstructions);
}

void SettingsStore::reload()
{
    QSettings settings;
    m_openAIKey = settings.value("openAIKey").toString();
    m_openRouterKey = settings.value("openRouterKey").toString();
    m_customInstructions = settings.value("customInstructions").toString();
    emit openAIKeyChanged();
    emit openRouterKeyChanged();
    emit customInstructionsChanged();
}

} // namespace MyChatty
