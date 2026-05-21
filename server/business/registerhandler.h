#pragma once

#include "handler.h"

/**
 * @brief 处理 MSG_REGISTER (1100)：解析 JSON、校验、写库、回 MSG_REGISTER_RSP (1101)
 */
class RegisterHandler : public Handler
{
public:
    RegisterHandler();
    void handle(std::shared_ptr<TcpConnection> tc, const short &msg_id, const std::string &msg_data) override;

private:
    void sendResponse(std::shared_ptr<TcpConnection> tc, int code, const std::string &msg, int64_t userId = 0);
};
