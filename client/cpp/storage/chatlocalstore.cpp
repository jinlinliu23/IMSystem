#include "storage/chatlocalstore.h"

#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

namespace {

QString formatTimeText(qint64 unixSec)
{
    if (unixSec <= 0) {
        return {};
    }
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(unixSec);
    const QDateTime now = QDateTime::currentDateTime();
    if (dt.date() == now.date()) {
        return dt.toString(QStringLiteral("HH:mm"));
    }
    return dt.toString(QStringLiteral("MM/dd HH:mm"));
}

QString connectionNameFor(const QString &account)
{
    return QStringLiteral("im_chat_%1").arg(account);
}

} // namespace

ChatLocalStore::ChatLocalStore(QObject *parent)
    : QObject(parent)
{
}

ChatLocalStore::~ChatLocalStore()
{
    close();
}

bool ChatLocalStore::isOpen() const
{
    return !connectionName_.isEmpty() && QSqlDatabase::contains(connectionName_)
           && QSqlDatabase::database(connectionName_).isOpen();
}

QSqlDatabase ChatLocalStore::database() const
{
    if (connectionName_.isEmpty() || !QSqlDatabase::contains(connectionName_)) {
        return {};
    }
    return QSqlDatabase::database(connectionName_);
}

bool ChatLocalStore::openForAccount(const QString &ownerAccount)
{
    close();

    const QString account = ownerAccount.trimmed();
    if (account.isEmpty()) {
        return false;
    }

    ownerAccount_ = account;
    connectionName_ = connectionNameFor(account);

    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(baseDir);
    dbPath_ = baseDir + QStringLiteral("/chat_") + account + QStringLiteral(".sqlite");

    if (!QSqlDatabase::contains(connectionName_)) {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
        db.setDatabaseName(dbPath_);
    } else {
        QSqlDatabase::database(connectionName_).setDatabaseName(dbPath_);
    }

    QSqlDatabase db = database();
    if (!db.isValid() || !db.open()) {
        qWarning() << "ChatLocalStore open failed:" << db.lastError().text() << dbPath_;
        close();
        return false;
    }

    if (!ensureSchema()) {
        close();
        return false;
    }
    return true;
}

void ChatLocalStore::close()
{
    if (!connectionName_.isEmpty() && QSqlDatabase::contains(connectionName_)) {
        {
            QSqlDatabase db = QSqlDatabase::database(connectionName_);
            if (db.isOpen()) {
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(connectionName_);
    }
    ownerAccount_.clear();
    dbPath_.clear();
    connectionName_.clear();
}

bool ChatLocalStore::ensureSchema()
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }

    QSqlQuery q(db);
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS chat_messages ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  owner_account TEXT NOT NULL,"
            "  peer_account TEXT NOT NULL,"
            "  from_account TEXT NOT NULL,"
            "  content TEXT NOT NULL,"
            "  created_at INTEGER NOT NULL,"
            "  server_message_id INTEGER NOT NULL,"
            "  UNIQUE(owner_account, server_message_id)"
            ");"))) {
        qWarning() << "ChatLocalStore schema:" << q.lastError().text();
        return false;
    }

    if (!q.exec(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_chat_peer "
            "ON chat_messages(owner_account, peer_account, id);"))) {
        qWarning() << "ChatLocalStore index:" << q.lastError().text();
        return false;
    }
    return true;
}

bool ChatLocalStore::insertMessage(const QString &peerAccount,
                                   const QString &fromAccount,
                                   const QString &content,
                                   qint64 createdAt,
                                   qint64 serverMessageId)
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen() || ownerAccount_.isEmpty()) {
        return false;
    }
    if (peerAccount.isEmpty() || content.isEmpty() || serverMessageId <= 0) {
        return false;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO chat_messages "
        "(owner_account, peer_account, from_account, content, created_at, server_message_id) "
        "VALUES (?, ?, ?, ?, ?, ?)"));
    q.addBindValue(ownerAccount_);
    q.addBindValue(peerAccount);
    q.addBindValue(fromAccount);
    q.addBindValue(content);
    q.addBindValue(createdAt);
    q.addBindValue(serverMessageId);

    if (!q.exec()) {
        qWarning() << "ChatLocalStore insert:" << q.lastError().text();
        return false;
    }
    return true;
}

QVector<ChatLocalRecord> ChatLocalStore::listMessages(const QString &peerAccount, int limit) const
{
    QVector<ChatLocalRecord> result;
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen() || peerAccount.isEmpty()) {
        return result;
    }
    if (limit <= 0) {
        limit = 500;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT from_account, content, created_at, server_message_id "
        "FROM chat_messages "
        "WHERE owner_account = ? AND peer_account = ? "
        "ORDER BY id ASC "
        "LIMIT ?"));
    q.addBindValue(ownerAccount_);
    q.addBindValue(peerAccount);
    q.addBindValue(limit);

    if (!q.exec()) {
        qWarning() << "ChatLocalStore list:" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        ChatLocalRecord row;
        row.fromAccount = q.value(0).toString();
        row.content = q.value(1).toString();
        row.createdAt = q.value(2).toLongLong();
        row.serverMessageId = q.value(3).toLongLong();
        result.append(row);
    }
    return result;
}

ChatPreview ChatLocalStore::previewForPeer(const QString &peerAccount) const
{
    ChatPreview preview;
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen() || peerAccount.isEmpty()) {
        return preview;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT content, created_at FROM chat_messages "
        "WHERE owner_account = ? AND peer_account = ? "
        "ORDER BY id DESC LIMIT 1"));
    q.addBindValue(ownerAccount_);
    q.addBindValue(peerAccount);

    if (q.exec() && q.next()) {
        preview.lastMessage = q.value(0).toString();
        preview.createdAt = q.value(1).toLongLong();
        preview.timeText = formatTimeText(preview.createdAt);
    }
    return preview;
}

QHash<QString, ChatPreview> ChatLocalStore::allPeerPreviews() const
{
    QHash<QString, ChatPreview> result;
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen()) {
        return result;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT m.peer_account, m.content, m.created_at "
        "FROM chat_messages m "
        "INNER JOIN ("
        "  SELECT peer_account, MAX(id) AS max_id "
        "  FROM chat_messages WHERE owner_account = ? "
        "  GROUP BY peer_account"
        ") t ON m.peer_account = t.peer_account AND m.id = t.max_id "
        "WHERE m.owner_account = ?"));
    q.addBindValue(ownerAccount_);
    q.addBindValue(ownerAccount_);

    if (!q.exec()) {
        qWarning() << "ChatLocalStore previews:" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        const QString peer = q.value(0).toString();
        ChatPreview preview;
        preview.lastMessage = q.value(1).toString();
        preview.createdAt = q.value(2).toLongLong();
        preview.timeText = formatTimeText(preview.createdAt);
        result.insert(peer, preview);
    }
    return result;
}
