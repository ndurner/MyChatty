#include "ApiClient.h"

#include <QJsonDocument>
#include <QNetworkReply>

namespace MyChatty {

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
{
}

void ApiClient::cancel()
{
    if (m_reply) {
        m_reply->abort();
    }
}

QNetworkRequest ApiClient::jsonRequest(const QUrl &url, const QString &apiKey)
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());
    return request;
}

QString ApiClient::networkErrorText(QNetworkReply *reply, const QByteArray &body)
{
    QString message;
    const QJsonDocument doc = QJsonDocument::fromJson(body);
    if (doc.isObject()) {
        const QJsonObject object = doc.object();
        const QJsonValue error = object.value("error");
        if (error.isObject()) {
            message = error.toObject().value("message").toString();
        } else if (error.isString()) {
            message = error.toString();
        }
    }
    if (message.isEmpty() && !body.trimmed().isEmpty()) {
        message = QString::fromUtf8(body.left(4096));
    }
    if (message.isEmpty() && reply) {
        message = reply->errorString();
    }
    return message.isEmpty() ? QStringLiteral("Network request failed") : message;
}

} // namespace MyChatty
