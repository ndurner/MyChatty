#pragma once

#include <QNetworkAccessManager>
#include <QObject>

namespace MyChatty {

class SpeechService : public QObject {
    Q_OBJECT
public:
    explicit SpeechService(QObject *parent = nullptr);

    void speak(const QString &text, const QString &apiKey);

signals:
    void ready(const QString &filePath);
    void failed(const QString &message);

private:
    QNetworkAccessManager m_network;
};

} // namespace MyChatty
