#pragma once

#include "ChatTypes.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <memory>

class QNetworkReply;

namespace MyChatty {

class ApiClient : public QObject {
    Q_OBJECT
public:
    explicit ApiClient(QObject *parent = nullptr);
    explicit ApiClient(QNetworkAccessManager *network, QObject *parent = nullptr);
    ~ApiClient() override = default;

    virtual void send(const ChatRequest &request) = 0;
    virtual void cancel();

signals:
    void textDelta(const QString &delta);
    void reasoningDelta(const QString &delta);
    void toolEvent(const QJsonObject &event);
    void completed(const MyChatty::ChatResult &result);
    void failed(const QString &message);
    void settled();

protected:
    static QNetworkRequest jsonRequest(const QUrl &url, const QString &apiKey);
    static QString networkErrorText(QNetworkReply *reply, const QByteArray &body);

    std::unique_ptr<QNetworkAccessManager> m_ownedNetwork;
    QNetworkAccessManager *m_network = nullptr;
    QPointer<QNetworkReply> m_reply;
};

} // namespace MyChatty
