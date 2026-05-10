#include "tcpconnection.h"
#include "../utility/logicsystem.h"
#include "networkconst.h"
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

void TcpConnection::setCloseCallback(std::function<void()> cb) {
    _closeCallback = std::move(cb);
}

void TcpConnection::send(const std::string& msgData) {//逻辑层调用
    std::lock_guard<std::mutex> lock(_send_lock);
    std::cerr<<"TcpConnection::send msgData size:"<<msgData.size()<<"\n";
    std::cerr<<"msgData:"<<msgData<<"\n";
    // 1. 入队
    _outputBuffer.append(msgData.data(), msgData.size());
    // 2. 确保写事件已注册，由 handleWrite 统一发送
    _channel->enableWriting();
}

void TcpConnection::handleRead() {//处理粘包
    char buf[4096];
    ssize_t n = read(_socket->getfd(), buf, sizeof(buf));
    std::cerr<<"fd:"<<_socket->getfd()<<"TcpConnection::handleRead\n";
    std::cerr<<"fd:"<<_socket->getfd()<<"n="<<n<<"\n";
    if (n > 0) {
        //将内核缓冲区的数据放入用户缓冲区
        _inputBuffer.append(buf, n);
        //处理_inputBuffer中的就绪数据
        while (true) {
            // 1. 头部长度
            if (_inputBuffer.readableBytes() < HEAD_TOTAL_LEN)
                break;

            // 2. 解析头部
            const char* header = _inputBuffer.peek();
            short msgId = 0;
            memcpy(&msgId, header, sizeof(msgId));
            msgId = ntohs(msgId);
            std::cerr<<"fd:"<<_socket->getfd()<<" "<<msgId<<"\n";

            uint32_t msgLen = 0;
            memcpy(&msgLen, header + HEAD_ID_LEN, sizeof(msgLen));
            msgLen = ntohl(msgLen);
            std::cerr<<"fd:"<<_socket->getfd()<<" "<<msgLen<<"\n";

            // 3. 校验合法性
            // if (msgId > MAX_LENGTH || msgLen > MAX_LENGTH) {
            //     handleClose();
            //     return;
            // }

            // 4. 检查整个帧是否接收完整
            const size_t frameLen = HEAD_TOTAL_LEN + msgLen;
            if (_inputBuffer.readableBytes() < frameLen)
                break;

            // 5. 取出消息体
            std::string msgData(_inputBuffer.peek() + HEAD_TOTAL_LEN, msgLen);
            // 6. 从缓冲区移除该帧
            _inputBuffer.retrieve(frameLen);

            std::cerr<<"fd:"<<_socket->getfd()<<" "<<msgId<<" "<<msgLen;
            // 7. 交付给逻辑层
            std::shared_ptr<RecvNode> rn=std::make_shared<RecvNode>(msgLen,msgId);
            std::memcpy(rn->_data,msgData.data(),msgLen);
            std::shared_ptr<LogicNode> ln=std::make_shared<LogicNode>(shared_from_this(),rn);
            LogicSystem::GetInstance()->PostMsgToQue(ln);
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
    std::lock_guard<std::mutex> lock(_send_lock);
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
