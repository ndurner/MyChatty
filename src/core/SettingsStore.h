#pragma once

#include <QObject>

namespace MyChatty {

class SettingsStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString openAIKey READ openAIKey WRITE setOpenAIKey NOTIFY openAIKeyChanged)
    Q_PROPERTY(QString openRouterKey READ openRouterKey WRITE setOpenRouterKey NOTIFY openRouterKeyChanged)
    Q_PROPERTY(QString customInstructions READ customInstructions WRITE setCustomInstructions NOTIFY customInstructionsChanged)
public:
    explicit SettingsStore(QObject *parent = nullptr);

    QString openAIKey() const;
    QString openRouterKey() const;
    QString customInstructions() const;

    void setOpenAIKey(const QString &value);
    void setOpenRouterKey(const QString &value);
    void setCustomInstructions(const QString &value);

    Q_INVOKABLE void save();
    Q_INVOKABLE void reload();

signals:
    void openAIKeyChanged();
    void openRouterKeyChanged();
    void customInstructionsChanged();

private:
    QString m_openAIKey;
    QString m_openRouterKey;
    QString m_customInstructions;
};

} // namespace MyChatty
