#include "echohandler.h"
#include <iostream>
#include <json/reader.h>
#include "../network/msgnode.h"

EchoHandler::EchoHandler() {}

void EchoHandler::handle(std::shared_ptr<TcpConnection> _tc, const short &msg_id, const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    reader.parse(msg_data,root);
    std::cerr<<"recevive msg id is "<<root["id"].asInt()<<"msg data is "<<root["data"].asString()<<std::endl;

    //root["data"]="server has receive msg ,msg data is "+ root["data"].asString();

    std::string return_str=root.toStyledString();
    std::cerr<<"return_str:"<<return_str<<"\n";
    std::cerr<<"return_str.size:"<<return_str.size()<<"\n";
    //构建发送节点
    std::shared_ptr<SendNode> sn=std::make_shared<SendNode>(return_str.data(),static_cast<uint32_t>(return_str.size()),root["id"].asInt());

    //注意字节序
    _tc->send(std::string(sn->_data, sn->_total_len));
}
