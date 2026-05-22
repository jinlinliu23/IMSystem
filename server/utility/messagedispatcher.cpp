#include "messagedispatcher.h"

#include "../database/databasemanager.h"
#include "../network/msgnode.h"
#include "../network/tcpconnection.h"
#include "onlineusersmanager.h"

#include <iostream>

void MessageDispatcher::deliverToAccount(const std::string &fromAccount,
                                         const std::string &toAccount,
                                         OfflineMessageKind kind,
                                         uint16_t pushMsgId,
                                         const std::string &jsonBody,
                                         int64_t groupId)
{
    if (toAccount.empty()) {
        return;
    }

    auto onlineMgr = OnlineUsersManager::GetInstance();
    if (onlineMgr->isOnline(toAccount)) {
        if (auto tc = onlineMgr->connectionFor(toAccount)) {
            pushFrame(tc, pushMsgId, jsonBody);
            return;
        }
    }

    DatabaseManager::GetInstance()->insertOfflineMessage(
        fromAccount, toAccount, kind, pushMsgId, jsonBody, groupId);
    std::cout << "MessageDispatcher: queued offline -> " << toAccount
              << " msgId=" << pushMsgId << std::endl;
}

void MessageDispatcher::deliverToAccounts(const std::string &fromAccount,
                                          const std::vector<std::string> &toAccounts,
                                          OfflineMessageKind kind,
                                          uint16_t pushMsgId,
                                          const std::string &jsonBody,
                                          int64_t groupId)
{
    for (const auto &account : toAccounts) {
        deliverToAccount(fromAccount, account, kind, pushMsgId, jsonBody, groupId);
    }
}

void MessageDispatcher::flushOfflineMessages(const std::string &toAccount)
{
    auto onlineMgr = OnlineUsersManager::GetInstance();
    if (!onlineMgr->isOnline(toAccount)) {
        return;
    }

    auto tc = onlineMgr->connectionFor(toAccount);
    if (!tc) {
        return;
    }

    auto db = DatabaseManager::GetInstance();
    const auto pending = db->fetchUndeliveredOfflineMessages(toAccount);
    for (const auto &msg : pending) {
        pushFrame(tc, static_cast<uint16_t>(msg.pushMsgId), msg.body);
        db->markOfflineMessageDelivered(msg.id);
    }

    if (!pending.empty()) {
        std::cout << "MessageDispatcher: flushed " << pending.size()
                  << " offline message(s) to " << toAccount << std::endl;
    }
}

void MessageDispatcher::pushFrame(const std::shared_ptr<TcpConnection> &tc,
                                  uint16_t msgId,
                                  const std::string &jsonBody)
{
    auto sn = std::make_shared<SendNode>(jsonBody.data(),
                                         static_cast<uint32_t>(jsonBody.size()),
                                         static_cast<short>(msgId));
    tc->send(std::string(sn->_data, sn->_total_len));
}
