#include "tcpserver.h"

#include <iostream>

TcpServer::TcpServer(EventLoop* loop, int port)
    : _loop(loop),
    _acceptor(new Acceptor(loop, port))
{
    // 把 Acceptor 的新连接回调绑定到 TcpServer::onNewConnection
    _acceptor->setNewConnectionCallback(
        std::bind(&TcpServer::onNewConnection, this, std::placeholders::_1)
        );
}

TcpServer::~TcpServer() {
    _acceptor.reset();
    _connections.clear();
}

void TcpServer::setMessageCallback(std::function<void(TcpConnection*, const std::string&)> cb) {
    _messageCallback = std::move(cb);
}

void TcpServer::start() {
    _acceptor->listen();  // Acceptor 开始监听（内部注册 EPOLLIN）
    std::cout << "TcpServer started on port " << _acceptor->port() << std::endl;
}

void TcpServer::onNewConnection(int conn_fd) {
    // 创建 TcpConnection（构造时自动注册到 epoll）
    auto conn = std::make_unique<TcpConnection>(_loop, conn_fd);

    // 设置消息回调
    conn->setMessageCallback(_messageCallback);

    // 设置关闭回调：通知 TcpServer 删掉自己
    TcpConnection* raw_ptr = conn.get();
    raw_ptr->setCloseCallback([this, raw_ptr] {
        onConnectionClose(raw_ptr);
    });

    // 把连接加入管理列表
    _connections.emplace(conn_fd, std::move(conn));
    std::cout << "New connection, fd=" << conn_fd
              << " total=" << _connections.size() << std::endl;
}

void TcpServer::onConnectionClose(TcpConnection* conn) {
    int fd = conn->fd();
    std::cout << "Connection closed, fd=" << fd << std::endl;
    // 从 map 中移除 -> unique_ptr 析构 -> TcpConnection 析构（自动清理 epoll 和 fd）
    _connections.erase(fd);
}