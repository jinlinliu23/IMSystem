#pragma once

#include "handler.h"

/**
 * @brief 处理 MSG_LOGIN (1200)：校验用户名密码，回 MSG_LOGIN_RSP (1201)
 */
class LoginHandler : public Handler
{
public:
    LoginHandler();
    void handle(std::shared_ptr<TcpConnection> tc, const short &msg_id, const std::string &msg_data) override;

private:
    void sendResponse(std::shared_ptr<TcpConnection> tc,
                      int code,
                      const std::string &msg,
                      int64_t userId = 0,
                      const std::string &username = {},
                      const std::string &nickname = {});
};
