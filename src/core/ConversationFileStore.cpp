#include "ConversationFileStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>

#include <utility>

namespace MyChatty {
namespace {

static constexpr qint64 MaxTextBytes = 2 * 1024 * 1024;

bool isInsideRoot(const QString &path, const QString &root)
{
    return path == root || path.startsWith(root + QDir::separator());
}

QString filesRoot()
{
    const QString root = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                             .filePath(QStringLiteral("conversation-files"));
    QDir().mkpath(root);
    return root;
}

} // namespace

ConversationFileStore::ConversationFileStore(QString conversationId, QObject *parent)
    : QObject(parent)
    , m_conversationId(std::move(conversationId))
{
    QDir().mkpath(QDir(rootPath()).filePath(QStringLiteral("uploads")));
    QDir().mkpath(QDir(rootPath()).filePath(QStringLiteral("tmp")));
    QDir().mkpath(QDir(rootPath()).filePath(QStringLiteral("repos")));
}

QString ConversationFileStore::rootPath() const
{
    QString id = m_conversationId;
    id.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_.-]")), QStringLiteral("_"));
    if (id.isEmpty()) {
        id = QStringLiteral("unsaved");
    }
    return QDir(filesRoot()).filePath(id);
}

QString ConversationFileStore::safeFileName(const QString &fileName) const
{
    QString name = QFileInfo(fileName).fileName();
    name.replace(QRegularExpression(QStringLiteral("[/\\\\:]")), QStringLiteral("_"));
    return name.isEmpty() ? QStringLiteral("attachment") : name;
}

QString ConversationFileStore::resolvePath(const QString &path, bool allowCreate) const
{
    QString normalized = QDir::cleanPath(path.trimmed());
    if (normalized.isEmpty() || normalized == QStringLiteral(".")) {
        normalized = QStringLiteral("/");
    }
    while (normalized.startsWith('/')) {
        normalized.remove(0, 1);
    }
    if (normalized.startsWith(QStringLiteral("../")) || normalized == QStringLiteral("..")) {
        return {};
    }

    const QDir root(rootPath());
    const QString absoluteRoot = QFileInfo(rootPath()).canonicalFilePath();
    const QString candidate = root.filePath(normalized);
    if (allowCreate) {
        QDir().mkpath(QFileInfo(candidate).absolutePath());
        const QString parent = QFileInfo(candidate).absoluteDir().canonicalPath();
        if (absoluteRoot.isEmpty() || parent.isEmpty() || !isInsideRoot(parent, absoluteRoot)) {
            return {};
        }
        return candidate;
    }

    const QFileInfo info(candidate);
    const QString canonical = info.exists() ? info.canonicalFilePath() : QFileInfo(info.absolutePath()).canonicalFilePath();
    if (absoluteRoot.isEmpty() || canonical.isEmpty() || !isInsideRoot(canonical, absoluteRoot)) {
        return {};
    }
    return candidate;
}

QString ConversationFileStore::importAttachment(const Attachment &attachment) const
{
    if (attachment.localPath.isEmpty()) {
        return {};
    }
    QFile source(attachment.localPath);
    if (!source.open(QIODevice::ReadOnly)) {
        return {};
    }

    const QString relative = QStringLiteral("/uploads/%1").arg(safeFileName(attachment.fileName));
    const QString targetPath = resolvePath(relative, true);
    if (targetPath.isEmpty()) {
        return {};
    }
    QFile target(targetPath);
    if (!target.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return {};
    }
    target.write(source.readAll());
    return relative;
}

QStringList ConversationFileStore::list(const QString &path) const
{
    const QString absolute = resolvePath(path);
    if (absolute.isEmpty()) {
        return {};
    }
    QDir dir(absolute);
    if (!dir.exists()) {
        return {};
    }
    return dir.entryList(QDir::NoDotAndDotDot | QDir::AllEntries, QDir::Name);
}

QString ConversationFileStore::readText(const QString &path) const
{
    const QString absolute = resolvePath(path);
    if (absolute.isEmpty()) {
        return {};
    }
    QFile file(absolute);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return QString::fromUtf8(file.read(MaxTextBytes + 1).left(MaxTextBytes));
}

bool ConversationFileStore::writeText(const QString &path, const QString &content)
{
    const QString normalized = QDir::cleanPath(path).startsWith(QStringLiteral("tmp/")) || QDir::cleanPath(path).startsWith(QStringLiteral("/tmp/"))
        ? path
        : QStringLiteral("/tmp/%1").arg(QFileInfo(path).fileName());
    const QString absolute = resolvePath(normalized, true);
    if (absolute.isEmpty()) {
        return false;
    }
    QFile file(absolute);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(content.left(MaxTextBytes).toUtf8());
    return true;
}

bool ConversationFileStore::exists(const QString &path) const
{
    const QString absolute = resolvePath(path);
    return !absolute.isEmpty() && QFileInfo::exists(absolute);
}

bool ConversationFileStore::remove(const QString &path)
{
    const QString normalized = QDir::cleanPath(path);
    if (!normalized.startsWith(QStringLiteral("tmp/")) && !normalized.startsWith(QStringLiteral("/tmp/"))) {
        return false;
    }
    const QString absolute = resolvePath(path);
    return !absolute.isEmpty() && QFile::remove(absolute);
}

} // namespace MyChatty
