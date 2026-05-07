#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "buffer.h"
#include "eventloop.h"
#include "socket.h"
#include <memory>
#include <string>
class TcpConnection
{
public:
    TcpConnection(EventLoop* loop, int conn_fd);
    ~TcpConnection();

    int fd() const { return _socket->getfd(); }

    void setMessageCallback(std::function<void(TcpConnection*, const std::string&)> cb);
    void setCloseCallback(std::function<void()> cb);

    void send(const std::string& msg);   // 上层用这个来发送消息

    // 暂时公开，后面可以设为 private，由 Channel 调用
    void handleRead();
    void handleWrite();
    void handleClose();

private:
    EventLoop* _loop;
    std::unique_ptr<Socket> _socket;
    std::unique_ptr<Channel> _channel;

    Buffer _inputBuffer;//输入缓冲区
    Buffer _outputBuffer;//输出缓冲区

    std::function<void(TcpConnection*, const std::string&)> _messageCallback;
    std::function<void()> _closeCallback;
};

#endif // TCPCONNECTION_H
