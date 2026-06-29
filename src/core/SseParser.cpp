#include "SseParser.h"

namespace MyChatty {

SseParser::SseParser(QObject *parent)
    : QObject(parent)
{
}

void SseParser::feed(const QByteArray &chunk)
{
    m_buffer += chunk;
    while (true) {
        qsizetype pos = m_buffer.indexOf("\n\n");
        qsizetype separatorLength = 2;
        const qsizetype crlfPos = m_buffer.indexOf("\r\n\r\n");
        if (crlfPos >= 0 && (pos < 0 || crlfPos < pos)) {
            pos = crlfPos;
            separatorLength = 4;
        }
        if (pos < 0) {
            return;
        }
        const QByteArray block = m_buffer.left(pos);
        m_buffer.remove(0, pos + separatorLength);
        parseBlock(block);
    }
}

void SseParser::finish()
{
    if (!m_buffer.trimmed().isEmpty()) {
        parseBlock(m_buffer);
    }
    m_buffer.clear();
}

void SseParser::parseBlock(const QByteArray &block)
{
    QString eventName = "message";
    QByteArray data;
    const QList<QByteArray> lines = block.split('\n');
    for (QByteArray line : lines) {
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        if (line.startsWith(':') || line.isEmpty()) {
            continue;
        }
        const int colon = line.indexOf(':');
        const QByteArray field = colon >= 0 ? line.left(colon) : line;
        QByteArray value = colon >= 0 ? line.mid(colon + 1) : QByteArray();
        if (value.startsWith(' ')) {
            value.remove(0, 1);
        }
        if (field == "event") {
            eventName = QString::fromUtf8(value);
        } else if (field == "data") {
            if (!data.isEmpty()) {
                data += '\n';
            }
            data += value;
        }
    }
    if (!data.isEmpty()) {
        emit eventReceived(eventName, data);
    }
}

} // namespace MyChatty
