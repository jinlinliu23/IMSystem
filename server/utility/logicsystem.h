#pragma once

#include "logicnode.h"
#include "singleton.h"
#include <condition_variable>
#include <queue>
#include <thread>
typedef std::function<void(std::shared_ptr<TcpConnection>,const short& msg_id,const std::string& msg_data)> FunCallBack;

class LogicSystem : public Singleton<LogicSystem>
{
    //让父类可以调用子类的构造函数
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    void PostMsgToQue(std::shared_ptr<LogicNode> ln);
private:
    LogicSystem();

    std::queue<std::shared_ptr<LogicNode>> _msg_que;
    std::mutex _mutex; //网络线程（PostMsgToQue）和逻辑线程（执行具体业务）的对他的使用
    std::condition_variable _consume;//判断队列是否为空，为空则挂起逻辑线程
    //工作线程，从队列里面取数据
    std::thread _worker_thread;
    //停止消息 在逻辑队列还有数据的情况下，及时处理数据。
    bool _b_stop;
    std::map<short,FunCallBack> _fun_callacks;

    void RegisterCallback();
    void HelloWordCallback(std::shared_ptr<TcpConnection> tc,const short& msg_id,const std::string& msg_data);
    void DealMsg();//交给工作线程调用
};

