#include "SpeechService.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QUrl>

namespace MyChatty {

SpeechService::SpeechService(QObject *parent)
    : QObject(parent)
{
}

void SpeechService::speak(const QString &text, const QString &apiKey)
{
    if (apiKey.trimmed().isEmpty()) {
        emit failed(QStringLiteral("OpenAI API key required for read-aloud."));
        return;
    }
    if (text.trimmed().isEmpty()) {
        emit failed(QStringLiteral("No assistant text to read."));
        return;
    }

    QNetworkRequest request(QUrl("https://api.openai.com/v1/audio/speech"));
#if defined(Q_OS_IOS)
    // Keep the iOS transport workaround consistent with chat requests.
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
#endif
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());
    const QJsonObject body{
        {"model", "gpt-4o-mini-tts"},
        {"voice", "alloy"},
        {"input", text.left(12000)},
        {"response_format", "mp3"},
    };

    QNetworkReply *reply = m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray bytes = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit failed(QStringLiteral("Text-to-speech failed: %1").arg(QString::fromUtf8(bytes.left(2048))));
            reply->deleteLater();
            return;
        }
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/mychatty";
        QDir().mkpath(dir);
        const QString path = QStringLiteral("%1/read-aloud-%2.mp3")
                                 .arg(dir, QString::number(QDateTime::currentMSecsSinceEpoch()));
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            emit failed(QStringLiteral("Could not write audio file."));
            reply->deleteLater();
            return;
        }
        file.write(bytes);
        file.close();
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        emit ready(path);
        reply->deleteLater();
    });
}

} // namespace MyChatty
