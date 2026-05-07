#include "tcpconnection.h"
#include <cstring>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>

TcpConnection::TcpConnection(EventLoop* loop, int conn_fd)
    : _loop(loop),
    _socket(std::make_unique<Socket>(conn_fd)),
    _channel(std::make_unique<Channel>(loop, conn_fd))
{
    // 绑定回调
    _channel->setReadCallback(std::bind(&TcpConnection::handleRead, this));
    _channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    _channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    // 立刻注册可读事件
    _channel->enableReading();
}

TcpConnection::~TcpConnection() {}

void TcpConnection::setMessageCallback(std::function<void(TcpConnection*, const std::string&)> cb) {
    _messageCallback = std::move(cb);
}

void TcpConnection::setCloseCallback(std::function<void()> cb) {
    _closeCallback = std::move(cb);
}

void TcpConnection::send(const std::string& msg) { //谁会调用它？
    // 1. 入队
    _outputBuffer.append(msg.data(), msg.size());
    // 2. 确保写事件已注册，由 handleWrite 统一发送
    _channel->enableWriting();
}

void TcpConnection::handleRead() {//处理粘包
    char buf[4096];
    ssize_t n = read(_socket->getfd(), buf, sizeof(buf));
    if (n > 0) {
        //将内核缓冲区的数据放入用户缓冲区
        _inputBuffer.append(buf, n);
        //处理_inputBuffer中的就绪数据
        while(1){
            // 1. 尝试按协议读包头 前4字节是长度
            if (_inputBuffer.readableBytes() < 4) break;
            // 2. 读出包体长度
            uint32_t bodyLen_net = 0;
            std::memcpy(&bodyLen_net, _inputBuffer.peek(), sizeof(bodyLen_net));
            uint32_t bodyLen = ntohl(bodyLen_net);

            // 3. 防止恶意超大包撑爆内存
            // const uint32_t MAX_BODY_LEN = 10 * 1024 * 1024; // 例如 10MB
            // if (bodyLen > MAX_BODY_LEN) {
            //     handleClose();   // 协议异常，关闭连接
            //     return;
            // }

            // 4. 检查包体是否完整到达
            const size_t frameLen = sizeof(uint32_t) + bodyLen;
            if (_inputBuffer.readableBytes() < frameLen)
                break;
            // 5. 跳过包头，取出包体数据，形成一条完整消息；从缓冲区中移除此帧
            std::string message(_inputBuffer.peek() + sizeof(uint32_t), bodyLen);

            _inputBuffer.retrieve(frameLen);
            // 6. 交给业务逻辑层
            if (_messageCallback) _messageCallback(this, message);
        }
        //查看输入缓冲区的字节大小，
        // 目前极简处理：假设每次收到的就是一条完整消息
        // 将来在这里添加协议解析
        std::string msg(_inputBuffer.peek(), _inputBuffer.readableBytes());
        _inputBuffer.retrieve(msg.size());
        if (_messageCallback) {
            _messageCallback(this, msg);
        }
    } else if (n == 0) {
        handleClose();
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            handleClose();
        }
    }
}

void TcpConnection::handleWrite() {
    //如果输出缓冲区没有要输出的数据，则关闭可写监听
    if (_outputBuffer.readableBytes() == 0) {
        _channel->disableWriting();
        return;
    }

    ssize_t n = write(_socket->getfd(), _outputBuffer.peek(), _outputBuffer.readableBytes());

    if (n > 0) {
        _outputBuffer.retrieve(n);
        if (_outputBuffer.readableBytes() == 0) {
            _channel->disableWriting();//关闭写监听。
        }
    } else {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            handleClose();
        }
    }
}

void TcpConnection::handleClose() {
    _channel->disableAll();//关闭监听
    if (_closeCallback) {
        //让TcpServer删掉自己（Connection）
        _closeCallback();
    }
}
