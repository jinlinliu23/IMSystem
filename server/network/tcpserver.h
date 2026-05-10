#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "acceptor.h"
#include "eventloop.h"
#include "tcpconnection.h"
#include <memory>

class TcpServer
{
public:
    TcpServer(EventLoop* loop, int port);
    ~TcpServer();

    // 启动服务器（让 Acceptor 开始监听）
    void start();

private:
    // 新连接到达时由 Acceptor 回调
    void onNewConnection(int conn_fd);
    // 连接关闭时由 TcpConnection 回调
    void onConnectionClose(TcpConnection* conn);

private:
    std::function<void(TcpConnection*, const std::string&)> _messageCallback;//与协议层的接口

    EventLoop* _loop;

    std::unique_ptr<Acceptor> _acceptor;
    std::map<int, std::shared_ptr<TcpConnection>> _connections;// 管理所有活跃连接，键是 conn_fd
};

#endif // TCPSERVER_H
