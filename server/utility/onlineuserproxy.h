#pragma once

#include <cstdint>
#include <memory>
#include <string>

class TcpConnection;

/**
 * @brief 用户缓冲代理：缓存 account / nickname 等，减少查库；在线时持有连接
 */
class OnlineUserProxy
{
public:
    int64_t userId = 0;
    std::string account;
    std::string nickname;
    bool online = false;
    std::weak_ptr<TcpConnection> connection;
};
