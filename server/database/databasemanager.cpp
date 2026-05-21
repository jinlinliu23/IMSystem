#include "databasemanager.h"

#include <filesystem>
#include <iostream>
#include <string>

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

} // namespace

DatabaseManager::~DatabaseManager()
{
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
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
        execSql(db_, "PRAGMA user_version = 2;");
        return true;
    }

    std::cout << "DatabaseManager: migrating users.username to case-sensitive UNIQUE..." << std::endl;

    if (!execSql(db_, "BEGIN IMMEDIATE;")) {
        return false;
    }

    const char *migrateSql = R"SQL(
        CREATE TABLE users_new (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            nickname TEXT NOT NULL,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
        );
        INSERT INTO users_new SELECT * FROM users;
        DROP TABLE users;
        ALTER TABLE users_new RENAME TO users;
    )SQL";

    if (!execSql(db_, migrateSql)) {
        execSql(db_, "ROLLBACK;");
        std::cerr << "DatabaseManager: migration failed. If you have both 'Alice' and 'alice', "
                     "delete server/data/im.db and register again." << std::endl;
        return false;
    }

    if (!execSql(db_, "COMMIT;")) {
        execSql(db_, "ROLLBACK;");
        return false;
    }

    execSql(db_, "PRAGMA user_version = 2;");
    return true;
}

bool DatabaseManager::ensureSchema()
{
    // username：账号，BINARY 默认排序即区分大小写；Alice 与 alice 可同时存在
    const char *sql = R"SQL(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            nickname TEXT NOT NULL,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            created_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))
        );
    )SQL";

    if (!execSql(db_, sql)) {
        return false;
    }

    return migrateUsernameCaseSensitiveIfNeeded();
}

bool DatabaseManager::usernameExists(const std::string &username)
{
    const char *sql = "SELECT 1 FROM users WHERE username = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    const bool exists = sqlite3_step(stmt) == SQLITE_ROW;
    sqlite3_finalize(stmt);
    return exists;
}

std::optional<int64_t> DatabaseManager::insertUser(const std::string &username,
                                                   const std::string &nickname,
                                                   const std::string &passwordHash,
                                                   const std::string &salt)
{
    const char *sql = R"SQL(
        INSERT INTO users (username, nickname, password_hash, salt)
        VALUES (?, ?, ?, ?);
    )SQL";

    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "DatabaseManager: prepare insert failed: " << sqlite3_errmsg(db_) << std::endl;
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, nickname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, passwordHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, salt.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "DatabaseManager: insert failed: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    const int64_t userId = sqlite3_last_insert_rowid(db_);
    sqlite3_finalize(stmt);
    return userId;
}

std::optional<UserAuthRecord> DatabaseManager::findUserAuthByUsername(const std::string &username)
{
    const char *sql = R"SQL(
        SELECT id, username, nickname, password_hash, salt
        FROM users WHERE username = ? LIMIT 1;
    )SQL";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    UserAuthRecord user;
    user.id = sqlite3_column_int64(stmt, 0);
    user.username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    user.nickname = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    user.passwordHash = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 3));
    user.salt = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 4));
    sqlite3_finalize(stmt);
    return user;
}

std::optional<UserRecord> DatabaseManager::findUserByUsername(const std::string &username)
{
    const char *sql = "SELECT id, username, nickname FROM users WHERE username = ? LIMIT 1;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }

    UserRecord user;
    user.id = sqlite3_column_int64(stmt, 0);
    user.username = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
    user.nickname = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 2));
    sqlite3_finalize(stmt);
    return user;
}
