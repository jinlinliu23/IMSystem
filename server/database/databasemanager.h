#pragma once

#include "../utility/offlinemessagekind.h"
#include "../utility/singleton.h"

#include <cstdint>
#include <optional>
#include <sqlite3.h>
#include <string>
#include <vector>

struct UserRecord {
    int64_t id = 0;
    std::string account;
    std::string nickname;
};

struct UserAuthRecord {
    int64_t id = 0;
    std::string account;
    std::string nickname;
    std::string passwordHash;
    std::string salt;
};

struct FriendRequestRecord {
    int64_t requestId = 0;
    int64_t fromUserId = 0;
    std::string fromAccount;
    std::string fromNickname;
};

struct PrivateMessageRecord {
    int64_t messageId = 0;
    std::string fromAccount;
    std::string content;
    int64_t createdAt = 0;
};

struct OfflineMessageRecord {
    int64_t id = 0;
    int kind = 0;
    std::string fromAccount;
    std::string toAccount;
    int64_t groupId = 0;
    int pushMsgId = 0;
    std::string body;
};

class DatabaseManager : public Singleton<DatabaseManager>
{
    friend class Singleton<DatabaseManager>;

public:
    ~DatabaseManager();

    bool init(const std::string &dbPath);

    bool accountExists(const std::string &account);

    std::optional<int64_t> insertUser(const std::string &account,
                                        const std::string &nickname,
                                        const std::string &passwordHash,
                                        const std::string &salt);

    std::optional<UserRecord> findUserByAccount(const std::string &account);
    std::optional<UserRecord> findUserById(int64_t userId);
    std::optional<UserAuthRecord> findUserAuthByAccount(const std::string &account);

    bool insertOfflineMessage(const std::string &fromAccount,
                              const std::string &toAccount,
                              OfflineMessageKind kind,
                              uint16_t pushMsgId,
                              const std::string &body,
                              int64_t groupId = 0);

    std::vector<OfflineMessageRecord> fetchUndeliveredOfflineMessages(const std::string &toAccount);

    bool markOfflineMessageDelivered(int64_t messageId);

    bool areFriends(int64_t userId, int64_t friendUserId);

    bool hasPendingFriendRequest(int64_t fromUserId, int64_t toUserId);

    bool insertFriendRequest(int64_t fromUserId, int64_t toUserId);

    std::vector<FriendRequestRecord> listPendingIncomingRequests(int64_t toUserId);

    bool acceptFriendRequest(int64_t fromUserId, int64_t toUserId);

    bool rejectFriendRequest(int64_t fromUserId, int64_t toUserId);

    std::vector<UserRecord> listFriends(int64_t userId);

    std::optional<int64_t> insertPrivateMessage(int64_t fromUserId,
                                                int64_t toUserId,
                                                const std::string &content);

    std::vector<PrivateMessageRecord> listPrivateMessagesBetween(int64_t userId,
                                                                 int64_t peerUserId,
                                                                 int limit = 50);

    std::optional<int64_t> getPrivateMessageCreatedAt(int64_t messageId);

private:
    DatabaseManager() = default;

    bool ensureSchema();
    bool migrateUsernameCaseSensitiveIfNeeded();
    bool migrateAccountColumnIfNeeded();
    bool ensureFriendSchema();
    bool ensureOfflineMessagesSchema();
    bool ensurePrivateMessagesSchema();

    int currentUserVersion();

    sqlite3 *db_ = nullptr;
    std::string dbPath_;
};
