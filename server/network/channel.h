#ifndef CHANNEL_H
#define CHANNEL_H

#include <cstdint>
#include <functional>
class EventLoop;
class Channel
{
public:
    Channel(EventLoop *el,int fd);
    //注销自己在epoll中的监听
    ~Channel();

    //对外调用接口，事件分发
    void handleEvent(uint32_t kevents);
    void setReadCallback(std::function<void()> cb);//注册读
    void setWriteCallback(std::function<void()> cb);//注册写
    void setCloseCallback(std::function<void()> cb);//注册关闭

    //事件控制 负责向epoll中注册事件，或修改监听事件
    void enableReading();      // 监听可读
    void enableWriting();      // 监听可写
    void disableWriting();     // 停止监听可写
    void disableAll();         // 停止监听所有事件
    void remove();             // 从 EventLoop中移除

    uint32_t getEvents();

    int getfd();
private:
    EventLoop *_el;
    int _fd;
    uint32_t m_events;//监听事件
    bool _added;     // 是否已添加到 EventLoop

    //读操作
    std::function<void()> readCallback;
    //写操作
    std::function<void()> writeCallback;
    //关闭操作
    std::function<void()> closeCallback;
private:
    void update();  //通知 EventLoop 更新 epoll 事件

};

#endif // CHANNEL_H
