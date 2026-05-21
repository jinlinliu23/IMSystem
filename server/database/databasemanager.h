#pragma once

#include "../utility/singleton.h"

#include <optional>
#include <sqlite3.h>
#include <string>

/**
 * @brief 用户表一行（查询结果，不含密码）
 */
struct UserRecord {
    int64_t id = 0;
    std::string username; ///< 账号（唯一标识，协议 JSON 字段名 username）
    std::string nickname; ///< 昵称（展示名）
};

/** 登录校验用：包含密码哈希与盐（仅服务端内部使用） */
struct UserAuthRecord {
    int64_t id = 0;
    std::string username;
    std::string nickname;
    std::string passwordHash;
    std::string salt;
};

/**
 * @brief SQLite 数据库单例，运行在服务端进程内
 *
 * 数据库文件路径由 main.cpp 传入（默认 server/data/im.db）。
 * 不是独立安装的 MySQL 服务；首次启动时自动建表。
 */
class DatabaseManager : public Singleton<DatabaseManager>
{
    friend class Singleton<DatabaseManager>;

public:
    ~DatabaseManager();

    /** 打开/创建数据库文件，并执行 ensureSchema() */
    bool init(const std::string &dbPath);

    bool usernameExists(const std::string &username);

    /** 插入新用户，成功返回自增 user_id */
    std::optional<int64_t> insertUser(const std::string &username,
                                      const std::string &nickname,
                                      const std::string &passwordHash,
                                      const std::string &salt);

    /** 按用户名查询（不含密码字段） */
    std::optional<UserRecord> findUserByUsername(const std::string &username);

    /** 按用户名查询，含 password_hash、salt，供登录校验 */
    std::optional<UserAuthRecord> findUserAuthByUsername(const std::string &username);

private:
    DatabaseManager() = default;

    /** 创建 users 表（若不存在），并按需迁移为区分大小写的账号唯一约束 */
    bool ensureSchema();
    bool migrateUsernameCaseSensitiveIfNeeded();

    sqlite3 *db_ = nullptr;
    std::string dbPath_;
};
