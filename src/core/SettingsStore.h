#pragma once

#include <QObject>

namespace MyChatty {

class SettingsStore : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString openAIKey READ openAIKey WRITE setOpenAIKey NOTIFY openAIKeyChanged)
    Q_PROPERTY(QString openRouterKey READ openRouterKey WRITE setOpenRouterKey NOTIFY openRouterKeyChanged)
    Q_PROPERTY(QString nvidiaKey READ nvidiaKey WRITE setNvidiaKey NOTIFY nvidiaKeyChanged)
    Q_PROPERTY(QString exaKey READ exaKey WRITE setExaKey NOTIFY exaKeyChanged)
    Q_PROPERTY(QString openRouterPdfEngine READ openRouterPdfEngine WRITE setOpenRouterPdfEngine NOTIFY openRouterPdfEngineChanged)
    Q_PROPERTY(QString customInstructions READ customInstructions WRITE setCustomInstructions NOTIFY customInstructionsChanged)
    Q_PROPERTY(bool webSearchEnabled READ webSearchEnabled WRITE setWebSearchEnabled NOTIFY webSearchEnabledChanged)
    Q_PROPERTY(bool exaSearchEnabled READ exaSearchEnabled WRITE setExaSearchEnabled NOTIFY exaSearchEnabledChanged)
    Q_PROPERTY(bool javaScriptUseEnabled READ javaScriptUseEnabled WRITE setJavaScriptUseEnabled NOTIFY javaScriptUseEnabledChanged)
    Q_PROPERTY(bool webBrowserEnabled READ webBrowserEnabled WRITE setWebBrowserEnabled NOTIFY webBrowserEnabledChanged)
    Q_PROPERTY(bool pageScreenshotsEnabled READ pageScreenshotsEnabled WRITE setPageScreenshotsEnabled NOTIFY pageScreenshotsEnabledChanged)
public:
    explicit SettingsStore(QObject *parent = nullptr);

    QString openAIKey() const;
    QString openRouterKey() const;
    QString nvidiaKey() const;
    QString exaKey() const;
    QString openRouterPdfEngine() const;
    QString customInstructions() const;
    bool webSearchEnabled() const;
    bool exaSearchEnabled() const;
    bool javaScriptUseEnabled() const;
    bool webBrowserEnabled() const;
    bool pageScreenshotsEnabled() const;

    void setOpenAIKey(const QString &value);
    void setOpenRouterKey(const QString &value);
    void setNvidiaKey(const QString &value);
    void setExaKey(const QString &value);
    void setOpenRouterPdfEngine(const QString &value);
    void setCustomInstructions(const QString &value);
    void setWebSearchEnabled(bool value);
    void setExaSearchEnabled(bool value);
    void setJavaScriptUseEnabled(bool value);
    void setWebBrowserEnabled(bool value);
    void setPageScreenshotsEnabled(bool value);

    Q_INVOKABLE void save();
    Q_INVOKABLE void reload();

signals:
    void openAIKeyChanged();
    void openRouterKeyChanged();
    void nvidiaKeyChanged();
    void exaKeyChanged();
    void openRouterPdfEngineChanged();
    void customInstructionsChanged();
    void webSearchEnabledChanged();
    void exaSearchEnabledChanged();
    void javaScriptUseEnabledChanged();
    void webBrowserEnabledChanged();
    void pageScreenshotsEnabledChanged();

private:
    QString m_openAIKey;
    QString m_openRouterKey;
    QString m_nvidiaKey;
    QString m_exaKey;
    QString m_openRouterPdfEngine = QStringLiteral("cloudflare-ai");
    QString m_customInstructions;
    bool m_webSearchEnabled = true;
    bool m_exaSearchEnabled = false;
    bool m_javaScriptUseEnabled = false;
    bool m_webBrowserEnabled = false;
    bool m_pageScreenshotsEnabled = false;
};

} // namespace MyChatty
