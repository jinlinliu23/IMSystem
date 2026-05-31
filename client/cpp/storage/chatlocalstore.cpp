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

// ============================================================================
// [Feature 1] 用户待办事项 CRUD
// ============================================================================

bool ChatLocalStore::insertTask(const QString &conversationId, const QString &peerAccount,
                                const QString &originalText, const QString &fromNickname)
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen() || ownerAccount_.isEmpty()) return false;
    if (originalText.isEmpty()) return false;

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO user_tasks "
        "(owner_account, conversation_id, peer_account, original_text, from_nickname, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?)"));
    q.addBindValue(ownerAccount_);
    q.addBindValue(conversationId);
    q.addBindValue(peerAccount);
    q.addBindValue(originalText);
    q.addBindValue(fromNickname);
    q.addBindValue(QDateTime::currentSecsSinceEpoch());

    if (!q.exec()) {
        qWarning() << "ChatLocalStore insertTask:" << q.lastError().text();
        return false;
    }
    return true;
}

bool ChatLocalStore::removeTask(int taskId)
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen() || ownerAccount_.isEmpty()) return false;

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "DELETE FROM user_tasks WHERE id = ? AND owner_account = ?"));
    q.addBindValue(taskId);
    q.addBindValue(ownerAccount_);
    return q.exec();
}

bool ChatLocalStore::setTaskCompleted(int taskId, bool completed)
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen() || ownerAccount_.isEmpty()) return false;

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "UPDATE user_tasks SET is_completed = ? WHERE id = ? AND owner_account = ?"));
    q.addBindValue(completed ? 1 : 0);
    q.addBindValue(taskId);
    q.addBindValue(ownerAccount_);
    return q.exec();
}

bool ChatLocalStore::updateTaskExtracted(int taskId, const QString &action,
                                         const QString &time, const QString &place)
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen() || ownerAccount_.isEmpty()) return false;

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "UPDATE user_tasks SET extracted_action = ?, extracted_time = ?, "
        "extracted_place = ?, ai_processed = 1 WHERE id = ? AND owner_account = ?"));
    q.addBindValue(action);
    q.addBindValue(time);
    q.addBindValue(place);
    q.addBindValue(taskId);
    q.addBindValue(ownerAccount_);
    return q.exec();
}

QVector<UserTask> ChatLocalStore::listTasks() const
{
    QVector<UserTask> result;
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen() || ownerAccount_.isEmpty()) return result;

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, conversation_id, peer_account, original_text, from_nickname, "
        "extracted_action, extracted_time, extracted_place, is_completed, ai_processed, created_at "
        "FROM user_tasks WHERE owner_account = ? ORDER BY is_completed ASC, created_at DESC"));
    q.addBindValue(ownerAccount_);

    if (!q.exec()) {
        qWarning() << "ChatLocalStore listTasks:" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        UserTask t;
        t.id = q.value(0).toInt();
        t.conversationId = q.value(1).toString();
        t.peerAccount = q.value(2).toString();
        t.originalText = q.value(3).toString();
        t.fromNickname = q.value(4).toString();
        t.extractedAction = q.value(5).toString();
        t.extractedTime = q.value(6).toString();
        t.extractedPlace = q.value(7).toString();
        t.isCompleted = q.value(8).toInt() != 0;
        t.aiProcessed = q.value(9).toInt() != 0;
        t.createdAt = q.value(10).toLongLong();
        result.append(t);
    }
    return result;
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

    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS chat_conversations ("
            "  owner_account TEXT NOT NULL,"
            "  conversation_id TEXT NOT NULL,"
            "  peer_account TEXT NOT NULL,"
            "  title TEXT NOT NULL,"
            "  is_group INTEGER NOT NULL DEFAULT 0,"
            "  group_id INTEGER NOT NULL DEFAULT 0,"
            "  group_name TEXT NOT NULL DEFAULT '',"
            "  dissolved INTEGER NOT NULL DEFAULT 0,"
            "  last_message_at INTEGER NOT NULL DEFAULT 0,"
            "  PRIMARY KEY(owner_account, conversation_id)"
            ");"))) {
        qWarning() << "ChatLocalStore conv schema:" << q.lastError().text();
        return false;
    }

    if (!q.exec(QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_chat_conversations_owner "
            "ON chat_conversations(owner_account, is_group, last_message_at DESC);"))) {
        qWarning() << "ChatLocalStore conv index:" << q.lastError().text();
        return false;
    }

    // [Feature 1] user_tasks 待办事项表
    if (!q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS user_tasks ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "  owner_account TEXT NOT NULL,"
            "  conversation_id TEXT NOT NULL,"
            "  peer_account TEXT NOT NULL,"
            "  original_text TEXT NOT NULL DEFAULT '',"
            "  from_nickname TEXT NOT NULL DEFAULT '',"
            "  extracted_action TEXT NOT NULL DEFAULT '',"
            "  extracted_time TEXT NOT NULL DEFAULT '',"
            "  extracted_place TEXT NOT NULL DEFAULT '',"
            "  is_completed INTEGER NOT NULL DEFAULT 0,"
            "  ai_processed INTEGER NOT NULL DEFAULT 0,"
            "  created_at INTEGER NOT NULL DEFAULT 0,"
            "  UNIQUE(owner_account, peer_account, original_text)"
            ");"))) {
        qWarning() << "ChatLocalStore tasks schema:" << q.lastError().text();
        return false;
    }

    // 迁移：加 from_nickname 列（如果从旧版本升级）
    bool hasFromNickCol = false;
    QSqlQuery pragmaTasks(db);
    if (pragmaTasks.exec(QStringLiteral("PRAGMA table_info(user_tasks)"))) {
        while (pragmaTasks.next()) {
            if (pragmaTasks.value(1).toString() == QStringLiteral("from_nickname")) {
                hasFromNickCol = true;
                break;
            }
        }
    }
    if (!hasFromNickCol
        && !q.exec(QStringLiteral(
               "ALTER TABLE user_tasks ADD COLUMN from_nickname TEXT NOT NULL DEFAULT ''"))) {
        qWarning() << "ChatLocalStore migrate from_nickname on user_tasks:" << q.lastError().text();
        return false;
    }

    QSqlQuery pragma(db);
    bool hasNicknameColumn = false;
    if (pragma.exec(QStringLiteral("PRAGMA table_info(chat_messages)"))) {
        while (pragma.next()) {
            if (pragma.value(1).toString() == QStringLiteral("from_nickname")) {
                hasNicknameColumn = true;
                break;
            }
        }
    }
    if (!hasNicknameColumn
        && !q.exec(QStringLiteral(
               "ALTER TABLE chat_messages ADD COLUMN from_nickname TEXT NOT NULL DEFAULT ''"))) {
        qWarning() << "ChatLocalStore migrate from_nickname:" << q.lastError().text();
        return false;
    }

    bool hasUnreadColumn = false;
    QSqlQuery pragmaConv(db);
    if (pragmaConv.exec(QStringLiteral("PRAGMA table_info(chat_conversations)"))) {
        while (pragmaConv.next()) {
            if (pragmaConv.value(1).toString() == QStringLiteral("unread_count")) {
                hasUnreadColumn = true;
                break;
            }
        }
    }
    if (!hasUnreadColumn
        && !q.exec(QStringLiteral(
               "ALTER TABLE chat_conversations ADD COLUMN unread_count INTEGER NOT NULL DEFAULT 0"))) {
        qWarning() << "ChatLocalStore migrate unread_count:" << q.lastError().text();
        return false;
    }
    return true;
}

bool ChatLocalStore::insertMessage(const QString &peerAccount,
                                   const QString &fromAccount,
                                   const QString &content,
                                   qint64 createdAt,
                                   qint64 serverMessageId,
                                   const QString &fromNickname)
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
        "(owner_account, peer_account, from_account, from_nickname, content, created_at, server_message_id) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"));
    q.addBindValue(ownerAccount_);
    q.addBindValue(peerAccount);
    q.addBindValue(fromAccount);
    q.addBindValue(fromNickname);
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
        "SELECT from_account, from_nickname, content, created_at, server_message_id "
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
        row.fromNickname = q.value(1).toString();
        row.content = q.value(2).toString();
        row.createdAt = q.value(3).toLongLong();
        row.serverMessageId = q.value(4).toLongLong();
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

bool ChatLocalStore::upsertConversationMeta(const QString &conversationId,
                                           const QString &peerAccount,
                                           const QString &title,
                                           bool isGroup,
                                           qint64 groupId,
                                           const QString &groupName,
                                           bool dissolved,
                                           qint64 lastMessageAt)
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen() || ownerAccount_.isEmpty() || conversationId.isEmpty()) {
        return false;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "INSERT INTO chat_conversations "
        "(owner_account, conversation_id, peer_account, title, is_group, group_id, group_name, dissolved, last_message_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(owner_account, conversation_id) DO UPDATE SET "
        "peer_account=excluded.peer_account, title=excluded.title, is_group=excluded.is_group, "
        "group_id=excluded.group_id, group_name=excluded.group_name, dissolved=excluded.dissolved, "
        "last_message_at=excluded.last_message_at"));
    q.addBindValue(ownerAccount_);
    q.addBindValue(conversationId);
    q.addBindValue(peerAccount);
    q.addBindValue(title);
    q.addBindValue(isGroup ? 1 : 0);
    q.addBindValue(groupId);
    q.addBindValue(groupName);
    q.addBindValue(dissolved ? 1 : 0);
    q.addBindValue(lastMessageAt);

    if (!q.exec()) {
        qWarning() << "ChatLocalStore upsertConversationMeta:" << q.lastError().text();
        return false;
    }
    return true;
}

int ChatLocalStore::unreadCount(const QString &conversationId) const
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen() || ownerAccount_.isEmpty() || conversationId.isEmpty()) {
        return 0;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT unread_count FROM chat_conversations "
        "WHERE owner_account = ? AND conversation_id = ? LIMIT 1"));
    q.addBindValue(ownerAccount_);
    q.addBindValue(conversationId);
    if (q.exec() && q.next()) {
        return q.value(0).toInt();
    }
    return 0;
}

bool ChatLocalStore::setUnreadCount(const QString &conversationId, int count)
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen() || ownerAccount_.isEmpty() || conversationId.isEmpty()) {
        return false;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "UPDATE chat_conversations SET unread_count = ? "
        "WHERE owner_account = ? AND conversation_id = ?"));
    q.addBindValue(qMax(0, count));
    q.addBindValue(ownerAccount_);
    q.addBindValue(conversationId);
    if (!q.exec()) {
        qWarning() << "ChatLocalStore setUnreadCount:" << q.lastError().text();
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool ChatLocalStore::incrementUnread(const QString &conversationId)
{
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen() || ownerAccount_.isEmpty() || conversationId.isEmpty()) {
        return false;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "UPDATE chat_conversations SET unread_count = unread_count + 1 "
        "WHERE owner_account = ? AND conversation_id = ?"));
    q.addBindValue(ownerAccount_);
    q.addBindValue(conversationId);
    if (!q.exec()) {
        qWarning() << "ChatLocalStore incrementUnread:" << q.lastError().text();
        return false;
    }
    return q.numRowsAffected() > 0;
}

QVector<ChatConversationInfo> ChatLocalStore::listConversations() const
{
    QVector<ChatConversationInfo> result;
    QSqlDatabase db = database();
    if (!db.isValid() || !db.isOpen() || ownerAccount_.isEmpty()) {
        return result;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT conversation_id, peer_account, title, is_group, group_id, group_name, dissolved, "
        "last_message_at, unread_count "
        "FROM chat_conversations WHERE owner_account = ? "
        "ORDER BY is_group DESC, last_message_at DESC, conversation_id ASC"));
    q.addBindValue(ownerAccount_);

    if (!q.exec()) {
        qWarning() << "ChatLocalStore listConversations:" << q.lastError().text();
        return result;
    }

    while (q.next()) {
        ChatConversationInfo info;
        info.conversationId = q.value(0).toString();
        info.peerAccount = q.value(1).toString();
        info.title = q.value(2).toString();
        info.isGroup = q.value(3).toInt() != 0;
        info.groupId = q.value(4).toLongLong();
        info.groupName = q.value(5).toString();
        info.dissolved = q.value(6).toInt() != 0;
        info.lastMessageAt = q.value(7).toLongLong();
        info.unreadCount = q.value(8).toInt();
        result.append(info);
    }
    return result;
}
