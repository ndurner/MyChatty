#pragma once

#include "ChatTypes.h"

#include <QObject>
#include <QStringList>

namespace MyChatty {

class ConversationFileStore : public QObject {
    Q_OBJECT
public:
    explicit ConversationFileStore(QString conversationId, QObject *parent = nullptr);

    QString rootPath() const;
    QString importAttachment(const Attachment &attachment) const;

    Q_INVOKABLE QStringList list(const QString &path = QStringLiteral("/")) const;
    Q_INVOKABLE QString readText(const QString &path) const;
    Q_INVOKABLE bool writeText(const QString &path, const QString &content);
    Q_INVOKABLE bool exists(const QString &path) const;
    Q_INVOKABLE bool remove(const QString &path);

private:
    QString resolvePath(const QString &path, bool allowCreate = false) const;
    QString safeFileName(const QString &fileName) const;

    QString m_conversationId;
};

} // namespace MyChatty
