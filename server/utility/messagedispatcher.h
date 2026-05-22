#pragma once

#include "../utility/singleton.h"
#include "offlinemessagekind.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

/**
 * @brief 消息投递：在线直接推送，离线写入 offline_messages，登录后 flush
 */
class MessageDispatcher : public Singleton<MessageDispatcher>
{
    friend class Singleton<MessageDispatcher>;

public:
    /** 推送到 to_account；离线则落库（body 为完整 JSON 字符串） */
    void deliverToAccount(const std::string &fromAccount,
                        const std::string &toAccount,
                        OfflineMessageKind kind,
                        uint16_t pushMsgId,
                        const std::string &jsonBody,
                        int64_t groupId = 0);

    /** 群聊：对多个成员账号投递（内部逐个 deliverToAccount） */
    void deliverToAccounts(const std::string &fromAccount,
                           const std::vector<std::string> &toAccounts,
                           OfflineMessageKind kind,
                           uint16_t pushMsgId,
                           const std::string &jsonBody,
                           int64_t groupId = 0);

    /** 用户上线后拉取并发送未投递消息 */
    void flushOfflineMessages(const std::string &toAccount);

private:
    MessageDispatcher() = default;

    void pushFrame(const std::shared_ptr<class TcpConnection> &tc,
                   uint16_t msgId,
                   const std::string &jsonBody);
};
