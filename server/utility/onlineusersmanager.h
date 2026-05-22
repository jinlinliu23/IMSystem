#pragma once

#include "../utility/singleton.h"
#include "onlineuserproxy.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

class TcpConnection;

/**
 * @brief 在线用户表：account -> OnlineUserProxy（缓冲代理），fd 反查 account
 */
class OnlineUsersManager : public Singleton<OnlineUsersManager>
{
    friend class Singleton<OnlineUsersManager>;

public:
    /** 从缓存或数据库加载代理（不标记在线） */
    std::shared_ptr<OnlineUserProxy> getOrLoadProxy(const std::string &account);

    void bindOnline(const std::string &account,
                    int64_t userId,
                    const std::string &nickname,
                    std::shared_ptr<TcpConnection> connection);

    void unbindByConnectionFd(int connectionFd);

    bool isOnline(const std::string &account) const;

    /** 是否已在其它连接上在线（connectionFd 为当前登录连接，同 fd 视为可登录） */
    bool isOnlineOnOtherConnection(const std::string &account, int connectionFd) const;

    std::shared_ptr<TcpConnection> connectionFor(const std::string &account) const;

private:
    OnlineUsersManager() = default;

    void refreshProxyFromDb(const std::shared_ptr<OnlineUserProxy> &proxy, const std::string &account);

    std::unordered_map<std::string, std::shared_ptr<OnlineUserProxy>> proxiesByAccount_;
    std::unordered_map<int, std::string> accountByConnectionFd_;
};
