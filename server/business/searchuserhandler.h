#pragma once

#include "../database/databasemanager.h"
#include "handler.h"

/**
 * @brief 处理 MSG_SEARCH_USER (1400)：按账号搜索用户（加好友前）
 */
class SearchUserHandler : public Handler
{
public:
    SearchUserHandler();
    void handle(std::shared_ptr<TcpConnection> tc, const short &msg_id, const std::string &msg_data) override;

private:
    void sendResponse(std::shared_ptr<TcpConnection> tc,
                      int code,
                      const std::string &msg,
                      const UserRecord *user = nullptr);
};
