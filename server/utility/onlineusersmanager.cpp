#include "onlineusersmanager.h"

#include "../database/databasemanager.h"
#include "../network/tcpconnection.h"

#include <iostream>

std::shared_ptr<OnlineUserProxy> OnlineUsersManager::getOrLoadProxy(const std::string &account)
{
    const auto it = proxiesByAccount_.find(account);
    if (it != proxiesByAccount_.end()) {
        return it->second;
    }

    auto proxy = std::make_shared<OnlineUserProxy>();
    proxy->account = account;
    refreshProxyFromDb(proxy, account);
    proxiesByAccount_.emplace(account, proxy);
    return proxy;
}

void OnlineUsersManager::refreshProxyFromDb(const std::shared_ptr<OnlineUserProxy> &proxy,
                                           const std::string &account)
{
    const auto user = DatabaseManager::GetInstance()->findUserByAccount(account);
    if (!user.has_value()) {
        return;
    }
    proxy->userId = user->id;
    proxy->account = user->account;
    proxy->nickname = user->nickname;
}

void OnlineUsersManager::bindOnline(const std::string &account,
                                    int64_t userId,
                                    const std::string &nickname,
                                    std::shared_ptr<TcpConnection> connection)
{
    auto proxy = getOrLoadProxy(account);
    proxy->userId = userId;
    proxy->nickname = nickname;
    proxy->online = true;
    proxy->connection = connection;

    accountByConnectionFd_[connection->fd()] = account;
    std::cout << "OnlineUsersManager: " << account << " online fd=" << connection->fd() << std::endl;
}

void OnlineUsersManager::unbindByConnectionFd(int connectionFd)
{
    const auto fdIt = accountByConnectionFd_.find(connectionFd);
    if (fdIt == accountByConnectionFd_.end()) {
        return;
    }

    const std::string account = fdIt->second;
    accountByConnectionFd_.erase(fdIt);

    const auto proxyIt = proxiesByAccount_.find(account);
    if (proxyIt != proxiesByAccount_.end()) {
        proxyIt->second->online = false;
        proxyIt->second->connection.reset();
    }

    std::cout << "OnlineUsersManager: " << account << " offline fd=" << connectionFd << std::endl;
}

bool OnlineUsersManager::isOnline(const std::string &account) const
{
    const auto it = proxiesByAccount_.find(account);
    if (it == proxiesByAccount_.end()) {
        return false;
    }
    return it->second->online && !it->second->connection.expired();
}

bool OnlineUsersManager::isOnlineOnOtherConnection(const std::string &account,
                                                 int connectionFd) const
{
    const auto it = proxiesByAccount_.find(account);
    if (it == proxiesByAccount_.end() || !it->second->online) {
        return false;
    }

    const auto conn = it->second->connection.lock();
    if (!conn) {
        return false;
    }
    return conn->fd() != connectionFd;
}

std::shared_ptr<TcpConnection> OnlineUsersManager::connectionFor(const std::string &account) const
{
    const auto it = proxiesByAccount_.find(account);
    if (it == proxiesByAccount_.end()) {
        return nullptr;
    }
    return it->second->connection.lock();
}
