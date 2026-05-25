#include "databasemanager.h"

#include <array>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>

namespace {

bool execSql(sqlite3 *db, const char *sql)
{
    char *errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "DatabaseManager SQL error: " << (errMsg ? errMsg : "unknown") << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool tableExists(sqlite3 *db, const char *tableName)
{
    sqlite3_stmt *stmt = nullptr;
    const char *sql = "SELECT 1 FROM sqlite_master WHERE type='table' AND name=? LIMIT 1;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, tableName, -1, SQLITE_TRANSIENT);
    const bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

bool usersTableHasColumn(sqlite3 *db, const char *columnName)
{
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, "PRAGMA table_info(users);", -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *name = sqlite3_column_text(stmt, 1);
        if (name && columnName == std::string(reinterpret_cast<const char *>(name))) {
            found = true;
            break;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

} // namespace

DatabaseManager::~DatabaseManager()
{
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

int DatabaseManager::currentUserVersion()
{
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, "PRAGMA user_version;", -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }
    int version = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return version;
}

bool DatabaseManager::init(const std::string &dbPath)
{
    dbPath_ = dbPath;

    const auto parent = std::filesystem::path(dbPath).parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            std::cerr << "DatabaseManager: failed to create directory: " << ec.message() << std::endl;
            return false;
        }
    }

    if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK) {
        std::cerr << "DatabaseManager: sqlite3_open failed: " << sqlite3_errmsg(db_) << std::endl;
        return false;
    }

    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
    return ensureSchema();
}

bool DatabaseManager::migrateUsernameCaseSensitiveIfNeeded()
{
    sqlite3_stmt *stmt = nullptr;
    const char *checkSql = "SELECT sql FROM sqlite_master WHERE type='table' AND name='users';";
    if (sqlite3_prepare_v2(db_, checkSql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    bool needsMigrate = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *ddl = sqlite3_column_text(stmt, 0);
        if (ddl) {
            const std::string ddlStr(reinterpret_cast<const char *>(ddl));
            needsMigrate = ddlStr.find("NOCASE") != std::string::npos;
        }
    }
    sqlite3_finalize(stmt);

    if (!needsMigrate) {
        if (currentUserVersion() < 2) {
            execSql(db_, "PRAGMA user_version = 2;");
        }
        return true;
    }

    std::cout << "DatabaseManager: migrating users to case-sensitive UNIQUE..." << std::endl;

    if (!execSql(db_, "BEGIN IMMEDIATE;")) {
        return false;
    }

    const char *col = usersTableHasColumn(db_, "account") ? "account" : "username";
    const char *migrateSql = R"SQL(
        CREATE TABLE users_new (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            account TEXT NOT NULL UNIQUE,
            nickname TEXT NOT NULL,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
        );
    )SQL";

    if (!execSql(db_, migrateSql)) {
        execSql(db_, "ROLLBACK;");
        return false;
    }

    std::string copySql = std::string("INSERT INTO users_new (id, account, nickname, password_hash, salt, created_at) "
                                      "SELECT id, ") + col + ", nickname, password_hash, salt, created_at FROM users;";
    if (!execSql(db_, copySql.c_str())) {
        execSql(db_, "ROLLBACK;");
        return false;
    }

    if (!execSql(db_, "DROP TABLE users; ALTER TABLE users_new RENAME TO users; COMMIT;")) {
        execSql(db_, "ROLLBACK;");
        return false;
    }

    execSql(db_, "PRAGMA user_version = 2;");
    return true;
}

bool DatabaseManager::migrateAccountColumnIfNeeded()
{
    if (!usersTableHasColumn(db_, "username")) {
        if (currentUserVersion() < 4) {
            execSql(db_, "PRAGMA user_version = 4;");
        }
        return true;
    }

    std::cout << "DatabaseManager: renaming users.username -> account" << std::endl;
    if (!execSql(db_, "ALTER TABLE users RENAME COLUMN username TO account;")) {
        return false;
    }
    execSql(db_, "PRAGMA user_version = 4;");
    return true;
}

bool DatabaseManager::ensureSchema()
{
    const char *sql = R"SQL(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            account TEXT NOT NULL UNIQUE,
            nickname TEXT NOT NULL,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
        );
    )SQL";

    if (!execSql(db_, sql)) {
        return false;
    }

    if (!migrateUsernameCaseSensitiveIfNeeded()) {
        return false;
    }
    if (!migrateAccountColumnIfNeeded()) {
        return false;
    }
    if (!ensureFriendSchema()) {
        return false;
    }
    if (!ensureOfflineMessagesSchema()) {
        return false;
    }
    if (!ensurePrivateMessagesSchema()) {
        return false;
    }
    return ensureGroupSchema();
}

bool DatabaseManager::ensureFriendSchema()
{
    if (tableExists(db_, "friend_requests")) {
        return true;
    }

    const char *sql = R"SQL(
        CREATE TABLE IF NOT EXISTS friend_requests (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            from_user_id INTEGER NOT NULL,
            to_user_id INTEGER NOT NULL,
            status INTEGER NOT NULL DEFAULT 0,
            created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
            UNIQUE(from_user_id, to_user_id),
            FOREIGN KEY (from_user_id) REFERENCES users(id),
            FOREIGN KEY (to_user_id) REFERENCES users(id)
        );
        CREATE TABLE IF NOT EXISTS friendships (
            user_id INTEGER NOT NULL,
            friend_user_id INTEGER NOT NULL,
            created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
            PRIMARY KEY (user_id, friend_user_id),
            FOREIGN KEY (user_id) REFERENCES users(id),
            FOREIGN KEY (friend_user_id) REFERENCES users(id)
        );
    )SQL";

    if (!execSql(db_, sql)) {
        return false;
    }

    std::cout << "DatabaseManager: friend_requests / friendships tables ready" << std::endl;
    return true;
}

bool DatabaseManager::ensureOfflineMessagesSchema()
{
    if (tableExists(db_, "offline_messages")) {
        return true;
    }

    const char *sql = R"SQL(
        CREATE TABLE IF NOT EXISTS offline_messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            kind INTEGER NOT NULL,
            from_account TEXT NOT NULL,
            to_account TEXT NOT NULL,
            group_id INTEGER NOT NULL DEFAULT 0,
            push_msg_id INTEGER NOT NULL,
            body TEXT NOT NULL,
            created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
            delivered INTEGER NOT NULL DEFAULT 0
        );
        CREATE INDEX IF NOT EXISTS idx_offline_to_delivered
            ON offline_messages(to_account, delivered);
    )SQL";

    if (!execSql(db_, sql)) {
        return false;
    }

    execSql(db_, "PRAGMA user_version = 5;");
    std::cout << "DatabaseManager: offline_messages table ready (stores from_account / to_account)" << std::endl;
    return true;
}

bool DatabaseManager::ensurePrivateMessagesSchema()
{
    if (tableExists(db_, "private_messages")) {
        return true;
    }

    const char *sql = R"SQL(
        CREATE TABLE IF NOT EXISTS private_messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            from_user_id INTEGER NOT NULL,
            to_user_id INTEGER NOT NULL,
            content TEXT NOT NULL,
            created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
            FOREIGN KEY (from_user_id) REFERENCES users(id),
            FOREIGN KEY (to_user_id) REFERENCES users(id)
        );
        CREATE INDEX IF NOT EXISTS idx_private_messages_pair
            ON private_messages(from_user_id, to_user_id, id);
    )SQL";

    if (!execSql(db_, sql)) {
        return false;
    }

    std::cout << "DatabaseManager: private_messages table ready" << std::endl;
    return true;
}

bool DatabaseManager::ensureGroupSchema()
{
    if (tableExists(db_, "groups") && tableExists(db_, "group_members")) {
        return true;
    }

    const char *sql = R"SQL(
        CREATE TABLE IF NOT EXISTS groups (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            owner_user_id INTEGER NOT NULL,
            created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
            FOREIGN KEY (owner_user_id) REFERENCES users(id)
        );
        CREATE TABLE IF NOT EXISTS group_members (
            group_id INTEGER NOT NULL,
            user_id INTEGER NOT NULL,
            role TEXT NOT NULL DEFAULT 'member',
            joined_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now')),
            PRIMARY KEY (group_id, user_id),
            FOREIGN KEY (group_id) REFERENCES groups(id),
            FOREIGN KEY (user_id) REFERENCES users(id)
        );
        CREATE INDEX IF NOT EXISTS idx_group_members_user
            ON group_members(user_id);
    )SQL";

    if (!execSql(db_, sql)) {
        return false;
    }

    execSql(db_, "PRAGMA user_version = 6;");
    std::cout << "DatabaseManager: groups / group_members tables ready" << std::endl;
    return true;
}

bool DatabaseManager::accountExists(const std::string &account)
{
    const char *sql = "SELECT 1 FROM users WHERE account = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_TRANSIENT);
    const bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

std::optional<int64_t> DatabaseManager::insertUser(const std::string &account,
                                                   const std::string &nickname,
                                                   const std::string &passwordHash,
                                                   const std::string &salt)
{
    const char *sql = R"SQL(
        INSERT INTO users (account, nickname, password_hash, salt)
        VALUES (?, ?, ?, ?);
    )SQL";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, nickname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, passwordHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, salt.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    const int64_t userId = sqlite3_last_insert_rowid(db_);
    sqlite3_finalize(stmt);
    return userId;
}

std::optional<UserAuthRecord> DatabaseManager::findUserAuthByAccount(const std::string &account)
{
    const char *sql = R"SQL(
        SELECT id, account, nickname, password_hash, salt
        FROM users WHERE account = ? LIMIT 1;
    )SQL";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    UserAuthRecord user;
    user.id = sqlite3_column_int64(stmt, 0);
    user.account = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    user.nickname = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    user.passwordHash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
    user.salt = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
    sqlite3_finalize(stmt);
    return user;
}

std::optional<UserRecord> DatabaseManager::findUserByAccount(const std::string &account)
{
    const char *sql = "SELECT id, account, nickname FROM users WHERE account = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, account.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    UserRecord user;
    user.id = sqlite3_column_int64(stmt, 0);
    user.account = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    user.nickname = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    sqlite3_finalize(stmt);
    return user;
}

std::optional<UserRecord> DatabaseManager::findUserById(int64_t userId)
{
    const char *sql = "SELECT id, account, nickname FROM users WHERE id = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_int64(stmt, 1, userId);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    UserRecord user;
    user.id = sqlite3_column_int64(stmt, 0);
    user.account = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    user.nickname = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    sqlite3_finalize(stmt);
    return user;
}

bool DatabaseManager::insertOfflineMessage(const std::string &fromAccount,
                                           const std::string &toAccount,
                                           OfflineMessageKind kind,
                                           uint16_t pushMsgId,
                                           const std::string &body,
                                           int64_t groupId)
{
    const char *sql = R"SQL(
        INSERT INTO offline_messages
            (kind, from_account, to_account, group_id, push_msg_id, body)
        VALUES (?, ?, ?, ?, ?, ?);
    )SQL";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, static_cast<int>(kind));
    sqlite3_bind_text(stmt, 2, fromAccount.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, toAccount.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, groupId);
    sqlite3_bind_int(stmt, 5, static_cast<int>(pushMsgId));
    sqlite3_bind_text(stmt, 6, body.c_str(), -1, SQLITE_TRANSIENT);

    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<OfflineMessageRecord> DatabaseManager::fetchUndeliveredOfflineMessages(
    const std::string &toAccount)
{
    std::vector<OfflineMessageRecord> result;
    const char *sql = R"SQL(
        SELECT id, kind, from_account, to_account, group_id, push_msg_id, body
        FROM offline_messages
        WHERE to_account = ? AND delivered = 0
        ORDER BY id ASC;
    )SQL";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return result;
    }

    sqlite3_bind_text(stmt, 1, toAccount.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        OfflineMessageRecord row;
        row.id = sqlite3_column_int64(stmt, 0);
        row.kind = sqlite3_column_int(stmt, 1);
        row.fromAccount = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        row.toAccount = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        row.groupId = sqlite3_column_int64(stmt, 4);
        row.pushMsgId = sqlite3_column_int(stmt, 5);
        row.body = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 6));
        result.push_back(std::move(row));
    }
    sqlite3_finalize(stmt);
    return result;
}

bool DatabaseManager::markOfflineMessageDelivered(int64_t messageId)
{
    const char *sql = "UPDATE offline_messages SET delivered = 1 WHERE id = ?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, messageId);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

bool DatabaseManager::areFriends(int64_t userId, int64_t friendUserId)
{
    const char *sql =
        "SELECT 1 FROM friendships WHERE user_id = ? AND friend_user_id = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, userId);
    sqlite3_bind_int64(stmt, 2, friendUserId);
    const bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

bool DatabaseManager::hasPendingFriendRequest(int64_t fromUserId, int64_t toUserId)
{
    const char *sql =
        "SELECT 1 FROM friend_requests WHERE from_user_id = ? AND to_user_id = ? "
        "AND status = 0 LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, fromUserId);
    sqlite3_bind_int64(stmt, 2, toUserId);
    const bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

bool DatabaseManager::insertFriendRequest(int64_t fromUserId, int64_t toUserId)
{
    const char *reopenSql = R"SQL(
        UPDATE friend_requests
        SET status = 0, created_at = strftime('%s', 'now')
        WHERE from_user_id = ? AND to_user_id = ? AND status = 2;
    )SQL";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, reopenSql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, fromUserId);
    sqlite3_bind_int64(stmt, 2, toUserId);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }
    const bool reopened = sqlite3_changes(db_) > 0;
    sqlite3_finalize(stmt);
    if (reopened) {
        return true;
    }

    const char *sql =
        "INSERT INTO friend_requests (from_user_id, to_user_id, status) VALUES (?, ?, 0);";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, fromUserId);
    sqlite3_bind_int64(stmt, 2, toUserId);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<FriendRequestRecord> DatabaseManager::listPendingIncomingRequests(int64_t toUserId)
{
    std::vector<FriendRequestRecord> result;
    const char *sql = R"SQL(
        SELECT fr.id, u.id, u.account, u.nickname
        FROM friend_requests fr
        JOIN users u ON u.id = fr.from_user_id
        WHERE fr.to_user_id = ? AND fr.status = 0
        ORDER BY fr.id ASC;
    )SQL";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return result;
    }

    sqlite3_bind_int64(stmt, 1, toUserId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        FriendRequestRecord row;
        row.requestId = sqlite3_column_int64(stmt, 0);
        row.fromUserId = sqlite3_column_int64(stmt, 1);
        row.fromAccount = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        row.fromNickname = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
        result.push_back(std::move(row));
    }
    sqlite3_finalize(stmt);
    return result;
}

bool DatabaseManager::acceptFriendRequest(int64_t fromUserId, int64_t toUserId)
{
    if (!execSql(db_, "BEGIN IMMEDIATE;")) {
        return false;
    }

    const char *updateSql =
        "UPDATE friend_requests SET status = 1 "
        "WHERE from_user_id = ? AND to_user_id = ? AND status = 0;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, updateSql, -1, &stmt, nullptr) != SQLITE_OK) {
        execSql(db_, "ROLLBACK;");
        return false;
    }
    sqlite3_bind_int64(stmt, 1, fromUserId);
    sqlite3_bind_int64(stmt, 2, toUserId);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        execSql(db_, "ROLLBACK;");
        return false;
    }
    sqlite3_finalize(stmt);

    const char *insertSql =
        "INSERT OR IGNORE INTO friendships (user_id, friend_user_id) VALUES (?, ?);";
    for (const auto pair : std::array<std::pair<int64_t, int64_t>, 2>{
             std::pair{fromUserId, toUserId}, std::pair{toUserId, fromUserId}}) {
        if (sqlite3_prepare_v2(db_, insertSql, -1, &stmt, nullptr) != SQLITE_OK) {
            execSql(db_, "ROLLBACK;");
            return false;
        }
        sqlite3_bind_int64(stmt, 1, pair.first);
        sqlite3_bind_int64(stmt, 2, pair.second);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            execSql(db_, "ROLLBACK;");
            return false;
        }
        sqlite3_finalize(stmt);
    }

    if (!execSql(db_, "COMMIT;")) {
        execSql(db_, "ROLLBACK;");
        return false;
    }
    return true;
}

bool DatabaseManager::rejectFriendRequest(int64_t fromUserId, int64_t toUserId)
{
    const char *sql =
        "UPDATE friend_requests SET status = 2 "
        "WHERE from_user_id = ? AND to_user_id = ? AND status = 0;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, fromUserId);
    sqlite3_bind_int64(stmt, 2, toUserId);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }
    const bool ok = sqlite3_changes(db_) > 0;
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<UserRecord> DatabaseManager::listFriends(int64_t userId)
{
    std::vector<UserRecord> result;
    const char *sql = R"SQL(
        SELECT u.id, u.account, u.nickname
        FROM friendships f
        JOIN users u ON u.id = f.friend_user_id
        WHERE f.user_id = ?
        ORDER BY u.account ASC;
    )SQL";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return result;
    }

    sqlite3_bind_int64(stmt, 1, userId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        UserRecord row;
        row.id = sqlite3_column_int64(stmt, 0);
        row.account = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        row.nickname = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        result.push_back(std::move(row));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<int64_t> DatabaseManager::insertPrivateMessage(int64_t fromUserId,
                                                             int64_t toUserId,
                                                             const std::string &content)
{
    const char *sql =
        "INSERT INTO private_messages (from_user_id, to_user_id, content) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int64(stmt, 1, fromUserId);
    sqlite3_bind_int64(stmt, 2, toUserId);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }
    const int64_t messageId = sqlite3_last_insert_rowid(db_);
    sqlite3_finalize(stmt);
    return messageId;
}

std::vector<PrivateMessageRecord> DatabaseManager::listPrivateMessagesBetween(int64_t userId,
                                                                              int64_t peerUserId,
                                                                              int limit)
{
    std::vector<PrivateMessageRecord> result;
    if (limit <= 0) {
        limit = 50;
    }
    if (limit > 200) {
        limit = 200;
    }

    const char *sql = R"SQL(
        SELECT pm.id, u.account, pm.content, pm.created_at
        FROM private_messages pm
        JOIN users u ON u.id = pm.from_user_id
        WHERE (pm.from_user_id = ? AND pm.to_user_id = ?)
           OR (pm.from_user_id = ? AND pm.to_user_id = ?)
        ORDER BY pm.id ASC
        LIMIT ?;
    )SQL";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return result;
    }

    sqlite3_bind_int64(stmt, 1, userId);
    sqlite3_bind_int64(stmt, 2, peerUserId);
    sqlite3_bind_int64(stmt, 3, peerUserId);
    sqlite3_bind_int64(stmt, 4, userId);
    sqlite3_bind_int(stmt, 5, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PrivateMessageRecord row;
        row.messageId = sqlite3_column_int64(stmt, 0);
        row.fromAccount = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        row.content = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        row.createdAt = sqlite3_column_int64(stmt, 3);
        result.push_back(std::move(row));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<int64_t> DatabaseManager::getPrivateMessageCreatedAt(int64_t messageId)
{
    const char *sql = "SELECT created_at FROM private_messages WHERE id = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int64(stmt, 1, messageId);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }
    const int64_t ts = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return ts;
}

bool DatabaseManager::groupExists(int64_t groupId)
{
    const char *sql = "SELECT 1 FROM groups WHERE id = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, groupId);
    const bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

bool DatabaseManager::isGroupMember(int64_t groupId, int64_t userId)
{
    const char *sql =
        "SELECT 1 FROM group_members WHERE group_id = ? AND user_id = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int64(stmt, 1, groupId);
    sqlite3_bind_int64(stmt, 2, userId);
    const bool member = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return member;
}

std::optional<int64_t> DatabaseManager::createGroup(const std::string &name,
                                                    int64_t ownerUserId,
                                                    const std::vector<int64_t> &memberUserIds)
{
    if (!execSql(db_, "BEGIN IMMEDIATE;")) {
        return std::nullopt;
    }

    const char *insertGroupSql =
        "INSERT INTO groups (name, owner_user_id) VALUES (?, ?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, insertGroupSql, -1, &stmt, nullptr) != SQLITE_OK) {
        execSql(db_, "ROLLBACK;");
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, ownerUserId);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        execSql(db_, "ROLLBACK;");
        return std::nullopt;
    }
    sqlite3_finalize(stmt);

    const int64_t groupId = sqlite3_last_insert_rowid(db_);

    const char *insertMemberSql =
        "INSERT INTO group_members (group_id, user_id, role) VALUES (?, ?, ?);";

    auto insertMember = [&](int64_t userId, const char *role) -> bool {
        if (sqlite3_prepare_v2(db_, insertMemberSql, -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_int64(stmt, 1, groupId);
        sqlite3_bind_int64(stmt, 2, userId);
        sqlite3_bind_text(stmt, 3, role, -1, SQLITE_TRANSIENT);
        const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return ok;
    };

    if (!insertMember(ownerUserId, "owner")) {
        execSql(db_, "ROLLBACK;");
        return std::nullopt;
    }

    for (const int64_t memberId : memberUserIds) {
        if (memberId == ownerUserId) {
            continue;
        }
        if (!insertMember(memberId, "member")) {
            execSql(db_, "ROLLBACK;");
            return std::nullopt;
        }
    }

    if (!execSql(db_, "COMMIT;")) {
        execSql(db_, "ROLLBACK;");
        return std::nullopt;
    }
    return groupId;
}

bool DatabaseManager::inviteUsersToGroup(int64_t groupId,
                                         int64_t inviterUserId,
                                         const std::vector<int64_t> &inviteeUserIds)
{
    if (groupId <= 0 || inviterUserId <= 0 || inviteeUserIds.empty()) {
        return false;
    }
    if (!isGroupMember(groupId, inviterUserId)) {
        return false;
    }

    if (!execSql(db_, "BEGIN IMMEDIATE;")) {
        return false;
    }

    const char *insertSql =
        "INSERT INTO group_members (group_id, user_id, role) VALUES (?, ?, 'member');";
    sqlite3_stmt *stmt = nullptr;
    for (const int64_t inviteeId : inviteeUserIds) {
        if (inviteeId <= 0 || inviteeId == inviterUserId || isGroupMember(groupId, inviteeId)) {
            continue;
        }
        if (sqlite3_prepare_v2(db_, insertSql, -1, &stmt, nullptr) != SQLITE_OK) {
            execSql(db_, "ROLLBACK;");
            return false;
        }
        sqlite3_bind_int64(stmt, 1, groupId);
        sqlite3_bind_int64(stmt, 2, inviteeId);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            execSql(db_, "ROLLBACK;");
            return false;
        }
        sqlite3_finalize(stmt);
    }

    if (!execSql(db_, "COMMIT;")) {
        execSql(db_, "ROLLBACK;");
        return false;
    }
    return true;
}

bool DatabaseManager::leaveGroup(int64_t groupId, int64_t userId, bool *dissolved)
{
    if (dissolved) {
        *dissolved = false;
    }
    if (groupId <= 0 || userId <= 0 || !isGroupMember(groupId, userId)) {
        return false;
    }
    if (!execSql(db_, "BEGIN IMMEDIATE;")) {
        return false;
    }

    sqlite3_stmt *stmt = nullptr;
    const char *deleteSql = "DELETE FROM group_members WHERE group_id = ? AND user_id = ?;";
    if (sqlite3_prepare_v2(db_, deleteSql, -1, &stmt, nullptr) != SQLITE_OK) {
        execSql(db_, "ROLLBACK;");
        return false;
    }
    sqlite3_bind_int64(stmt, 1, groupId);
    sqlite3_bind_int64(stmt, 2, userId);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        execSql(db_, "ROLLBACK;");
        return false;
    }
    sqlite3_finalize(stmt);

    const char *countSql = "SELECT COUNT(*) FROM group_members WHERE group_id = ?;";
    if (sqlite3_prepare_v2(db_, countSql, -1, &stmt, nullptr) != SQLITE_OK) {
        execSql(db_, "ROLLBACK;");
        return false;
    }
    sqlite3_bind_int64(stmt, 1, groupId);
    int memberCount = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        memberCount = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    if (memberCount < 0) {
        execSql(db_, "ROLLBACK;");
        return false;
    }

    if (memberCount == 0) {
        const char *deleteGroupSql = "DELETE FROM groups WHERE id = ?;";
        if (sqlite3_prepare_v2(db_, deleteGroupSql, -1, &stmt, nullptr) != SQLITE_OK) {
            execSql(db_, "ROLLBACK;");
            return false;
        }
        sqlite3_bind_int64(stmt, 1, groupId);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            execSql(db_, "ROLLBACK;");
            return false;
        }
        sqlite3_finalize(stmt);
        if (dissolved) {
            *dissolved = true;
        }
    }

    if (!execSql(db_, "COMMIT;")) {
        execSql(db_, "ROLLBACK;");
        return false;
    }
    return true;
}

std::vector<GroupSummaryRecord> DatabaseManager::listMyGroups(int64_t userId)
{
    std::vector<GroupSummaryRecord> result;
    const char *sql = R"SQL(
        SELECT g.id, g.name, COUNT(gm.user_id) AS member_count
        FROM groups g
        INNER JOIN group_members self ON self.group_id = g.id AND self.user_id = ?
        INNER JOIN group_members gm ON gm.group_id = g.id
        GROUP BY g.id
        ORDER BY g.id DESC;
    )SQL";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return result;
    }
    sqlite3_bind_int64(stmt, 1, userId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        GroupSummaryRecord row;
        row.groupId = sqlite3_column_int64(stmt, 0);
        row.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        row.memberCount = sqlite3_column_int(stmt, 2);
        result.push_back(std::move(row));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<GroupInfoRecord> DatabaseManager::getGroupInfo(int64_t groupId, int64_t userId)
{
    if (!groupExists(groupId)) {
        return std::nullopt;
    }
    if (!isGroupMember(groupId, userId)) {
        return std::nullopt;
    }

    GroupInfoRecord info;
    info.groupId = groupId;
    info.dissolved = false;

    const char *nameSql = "SELECT name FROM groups WHERE id = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, nameSql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int64(stmt, 1, groupId);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }
    info.name = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);

    const char *membersSql = R"SQL(
        SELECT u.account, u.nickname, gm.role
        FROM group_members gm
        JOIN users u ON u.id = gm.user_id
        WHERE gm.group_id = ?
        ORDER BY CASE gm.role WHEN 'owner' THEN 0 ELSE 1 END, u.account ASC;
    )SQL";

    if (sqlite3_prepare_v2(db_, membersSql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int64(stmt, 1, groupId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        GroupMemberDetail m;
        m.account = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
        m.nickname = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        m.role = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
        info.members.push_back(std::move(m));
    }
    sqlite3_finalize(stmt);
    return info;
}
