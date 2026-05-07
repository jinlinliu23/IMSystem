#include "channel.h"
#include "eventloop.h"
#include <sys/epoll.h>

Channel::Channel(EventLoop *el,int fd)
    :_el(el),_fd(fd),m_events(0),_added(false){}

Channel::~Channel(){
    if (_added) {
        remove();
    }
}
//设置回调
void Channel::setReadCallback(std::function<void()> cb) {
    readCallback = std::move(cb);
}

void Channel::setWriteCallback(std::function<void()> cb) {
    writeCallback = std::move(cb);
}

void Channel::setCloseCallback(std::function<void()> cb) {
    closeCallback = std::move(cb);
}
//向epoll中设置监听事件 委托epoll处理
void Channel::enableReading() {
    m_events |= EPOLLIN;
    update();
}

void Channel::enableWriting() {
    m_events |= EPOLLOUT;
    update();
}

void Channel::disableWriting() {
    m_events &= ~EPOLLOUT;
    update();
}

void Channel::disableAll() {
    m_events = 0;
    update();
}

void Channel::remove() {
    if (_added) {
        _el->removeChannel(this);
        _added = false;
    }
}

void Channel::update() {
    if (!_added) {
        _el->addChannel(this);     // 第一次添加
        _added = true;
    } else {
        _el->updateChannel(this);  // 已有，修改事件
    }
}

void Channel::handleEvent(uint32_t revents) {

    // 处理关闭和错误事件 对端挂断（Hang Up）对方调用了close；
    if ((revents & EPOLLHUP) && !(revents & EPOLLIN)) {
        if (closeCallback) closeCallback();
        return;
    }

    if (revents & (EPOLLERR)) {//发生错误，RST 包到达等。
        if (closeCallback) closeCallback();
        return;
    }
    // 读事件
    if (revents & (EPOLLIN | EPOLLPRI)) {
        if (readCallback) readCallback();
    }
    // 写事件
    if (revents & EPOLLOUT) {
        if (writeCallback) writeCallback();
    }
}

//
int Channel::getfd(){
    return _fd;
}
//
uint32_t Channel::getEvents(){
    return m_events;
}