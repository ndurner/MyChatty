#include "ApiClient.h"

#include <QJsonDocument>
#include <QNetworkReply>

namespace MyChatty {

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
    , m_ownedNetwork(std::make_unique<QNetworkAccessManager>())
    , m_network(m_ownedNetwork.get())
{
}

ApiClient::ApiClient(QNetworkAccessManager *network, QObject *parent)
    : QObject(parent)
    , m_network(network)
{
    Q_ASSERT(m_network);
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
#if defined(Q_OS_IOS)
    // Qt's HTTP/2 connection can lose its HPACK header-compression state on
    // iOS. Both OpenAI and OpenRouter then fail before either service receives
    // a usable request. HTTP/1.1 avoids that shared transport failure.
    request.setAttribute(QNetworkRequest::Http2AllowedAttribute, false);
#endif
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
            const QJsonObject errorObject = error.toObject();
            message = errorObject.value("metadata").toObject().value("raw").toString();
            if (message.isEmpty()) {
                message = errorObject.value("message").toString();
            }
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
