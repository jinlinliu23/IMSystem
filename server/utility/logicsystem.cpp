#include <json/json.h>
#include "logicsystem.h"
#include "../network/msgnode.h"

LogicSystem::LogicSystem():_b_stop(false) {
    _worker_thread=std::thread(&LogicSystem::DealMsg,this);
}

LogicSystem::~LogicSystem(){
    _b_stop = true;
    _consume.notify_one();
    _worker_thread.join();
}

void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> ln)
{
    std::unique_lock<std::mutex> unique_lk(_mutex);
    _msg_que.push(ln);

    if(_msg_que.size()==1){
        _consume.notify_one();
    }
}

// void LogicSystem::RegisterCallback()
// {
//     _fun_callacks[static_cast<uint32_t>(MSG_IDS::MSG_HELLO_WORD)]=std::bind(&LogicSystem::HelloWordCallback,
//         this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3);
// }

void LogicSystem::DealMsg()
{
    for(;;){
        //_mutex进行加锁操作
        std::unique_lock<std::mutex> unique_lk(_mutex);

        //判断队列为空 用条件变量等待
        while(_msg_que.empty()&&!_b_stop){
            _consume.wait(unique_lk);//将线程挂起，并释放资源，进行解锁操作。
        }

        //如果为关闭状态 处理数据
        if(_b_stop){
            while(!_msg_que.empty()){
                std::shared_ptr<LogicNode> msg_node=_msg_que.front();
                //交给消息分发 处理业务
                _mesRout.RoutMessage(msg_node);
                _msg_que.pop();
            }
        }

        //如果不是关闭状态
        std::shared_ptr<LogicNode> msg_node=_msg_que.front();
        //交给消息分发 处理业务
        _mesRout.RoutMessage(msg_node);
        _msg_que.pop();
    }
}

// void LogicSystem::HelloWordCallback(std::shared_ptr<TcpConnection> tc,const short& msg_id,const std::string& msg_data)
// {
//     Json::Reader reader;
//     Json::Value root;
//     reader.parse(msg_data,root);
//     std::cout<<"recevive msg id is "<<root["id"].asInt()<<"msg data is "<<root["data"].asString()<<std::endl;
//     root["data"]="server has receive msg ,msg data is "+ root["data"].asString();

//     std::string return_str=root.toStyledString();
//     std::cerr<<"return_str:"<<return_str<<"\n";
//     std::cerr<<"return_str.size:"<<return_str.size()<<"\n";
//     //构建发送节点
//     std::shared_ptr<SendNode> sn=std::make_shared<SendNode>(return_str.data(),static_cast<uint32_t>(return_str.size()),root["id"].asInt());

//     //注意字节序
//     tc->send(std::string(sn->_data, sn->_total_len));
// }