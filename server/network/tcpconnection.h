#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "buffer.h"
#include "eventloop.h"
#include "socket.h"
#include <memory>
#include <mutex>
#include <string>
class TcpConnection: public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, int conn_fd);
    ~TcpConnection();

    int fd() const { return _socket->getfd(); }

    void setCloseCallback(std::function<void()> cb);

    void send(const std::string& msg);   // 上层用这个来发送消息

private:
    EventLoop* _loop;
    std::unique_ptr<Socket> _socket;
    std::unique_ptr<Channel> _channel;
    std::mutex _send_lock;

    Buffer _inputBuffer;//输入缓冲区
    Buffer _outputBuffer;//输出缓冲区

    std::function<void()> _closeCallback;
private:
    void handleRead();
    void handleWrite();
    void handleClose();
};

#endif // TCPCONNECTION_H
