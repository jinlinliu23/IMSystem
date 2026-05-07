#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "channel.h"
#include <map>
class EventLoop
{
public:
    //创建epoll
    EventLoop();
    //关闭epoll
    ~EventLoop();

    //添加,删除,修改 监听描述符
    void addChannel(Channel* ch);
    void updateChannel(Channel* ch);
    void removeChannel(Channel* ch);

    //启动阻塞监听
    void run();
public:

private:
    //文件描述符
    int m_epollfd;
    //所有的epoll_data指向的实体节点 事件数组
    std::map<int ,Channel*> _channels;
    //就绪事件数组
    std::vector<struct epoll_event> _events; // 就绪事件数组
};

#endif // EVENTLOOP_H
