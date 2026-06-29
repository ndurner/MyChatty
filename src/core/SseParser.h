#pragma once

#include <QByteArray>
#include <QObject>
#include <QString>

namespace MyChatty {

class SseParser : public QObject {
    Q_OBJECT
public:
    explicit SseParser(QObject *parent = nullptr);

    void feed(const QByteArray &chunk);
    void finish();

signals:
    void eventReceived(const QString &eventName, const QByteArray &data);

private:
    void parseBlock(const QByteArray &block);

    QByteArray m_buffer;
};

} // namespace MyChatty
