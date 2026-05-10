#include "acceptor.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>

Acceptor::Acceptor(EventLoop* loop, int port)
    : _loop(loop), _port(port)
{
    int listen_fd = createListenFd(port);
    _listenSocket = std::make_unique<Socket>(listen_fd);
    _acceptChannel = std::make_unique<Channel>(loop, listen_fd);

    // 绑定自己的 handleRead 为可读回调
    _acceptChannel->setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor() {}

void Acceptor::listen() {
    // 此时才真正开始监听 epoll 的读事件
    _acceptChannel->enableReading();
}

void Acceptor::setNewConnectionCallback(std::function<void(int)> cb) {
    _newConnectionCallback = std::move(cb);
}

int Acceptor::createListenFd(int port) {
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) {
        perror("Acceptor::createListenFd socket");
        exit(1);
    }

    //SO_REUSEADDR	允许重用本地地址和端口 方便服务器重启
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Acceptor bind");
        exit(1);
    }

    if (::listen(fd, SOMAXCONN) < 0) {
        perror("Acceptor listen");
        exit(1);
    }

    return fd;
}

void Acceptor::handleRead() {
    //循环accept直到没有新连接
    while (true) {
        sockaddr_in peer{};
        socklen_t len = sizeof(peer);
        int conn_fd = accept4(_listenSocket->getfd(),
                              (sockaddr*)&peer, &len,
                              SOCK_NONBLOCK); // 直接设置非阻塞
        if (conn_fd >= 0) {
            if (_newConnectionCallback) {
                _newConnectionCallback(conn_fd);
            } else {
                close(conn_fd); // 没有回调则直接关闭
            }
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                perror("Acceptor accept");
                break;
            }
        }
        std::cerr<<"Acceptor::handleRead()循环一次";
    }
}