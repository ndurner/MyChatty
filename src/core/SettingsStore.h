#pragma once

#include <QObject>

namespace MyChatty {

class SettingsStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString openAIKey READ openAIKey WRITE setOpenAIKey NOTIFY openAIKeyChanged)
    Q_PROPERTY(QString openRouterKey READ openRouterKey WRITE setOpenRouterKey NOTIFY openRouterKeyChanged)
    Q_PROPERTY(QString exaKey READ exaKey WRITE setExaKey NOTIFY exaKeyChanged)
    Q_PROPERTY(QString customInstructions READ customInstructions WRITE setCustomInstructions NOTIFY customInstructionsChanged)
    Q_PROPERTY(bool webSearchEnabled READ webSearchEnabled WRITE setWebSearchEnabled NOTIFY webSearchEnabledChanged)
    Q_PROPERTY(bool exaSearchEnabled READ exaSearchEnabled WRITE setExaSearchEnabled NOTIFY exaSearchEnabledChanged)
public:
    explicit SettingsStore(QObject *parent = nullptr);

    QString openAIKey() const;
    QString openRouterKey() const;
    QString exaKey() const;
    QString customInstructions() const;
    bool webSearchEnabled() const;
    bool exaSearchEnabled() const;

    void setOpenAIKey(const QString &value);
    void setOpenRouterKey(const QString &value);
    void setExaKey(const QString &value);
    void setCustomInstructions(const QString &value);
    void setWebSearchEnabled(bool value);
    void setExaSearchEnabled(bool value);

    Q_INVOKABLE void save();
    Q_INVOKABLE void reload();

signals:
    void openAIKeyChanged();
    void openRouterKeyChanged();
    void exaKeyChanged();
    void customInstructionsChanged();
    void webSearchEnabledChanged();
    void exaSearchEnabledChanged();

private:
    QString m_openAIKey;
    QString m_openRouterKey;
    QString m_exaKey;
    QString m_customInstructions;
    bool m_webSearchEnabled = true;
    bool m_exaSearchEnabled = false;
};

} // namespace MyChatty
