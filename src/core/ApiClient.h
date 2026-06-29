#pragma once

#include "ChatTypes.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>

class QNetworkReply;

namespace MyChatty {

class ApiClient : public QObject {
    Q_OBJECT
public:
    explicit ApiClient(QObject *parent = nullptr);
    ~ApiClient() override = default;

    virtual void send(const ChatRequest &request) = 0;
    virtual void cancel();

signals:
    void textDelta(const QString &delta);
    void reasoningDelta(const QString &delta);
    void toolEvent(const QJsonObject &event);
    void completed(const MyChatty::ChatResult &result);
    void failed(const QString &message);

protected:
    static QNetworkRequest jsonRequest(const QUrl &url, const QString &apiKey);
    static QString networkErrorText(QNetworkReply *reply, const QByteArray &body);

    QNetworkAccessManager m_network;
    QPointer<QNetworkReply> m_reply;
};

} // namespace MyChatty
