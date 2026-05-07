#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include "eventloop.h"
#include "channel.h"
#include "socket.h"
#include <memory>
class Acceptor
{
public:
    Acceptor(EventLoop* loop, int port);
    ~Acceptor();

    void listen();
    int port() const { return _port; }

    void setNewConnectionCallback(std::function<void(int)> cb);

private:
    void handleRead();
    int createListenFd(int port);
private:
    EventLoop* _loop;
    int _port;

    std::unique_ptr<Socket> _listenSocket;
    std::unique_ptr<Channel> _acceptChannel;
    std::function<void(int)> _newConnectionCallback;
};

#endif // ACCEPTOR_H
