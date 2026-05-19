#pragma once

#include <cstdint>
#include <cstring>
#include <iostream>
class MsgNode
{
public:
    MsgNode(short max_len) :_total_len(max_len), _cur_len(0) {
        _data = new char[_total_len + 1]();
        _data[_total_len] = '\0';
    }

    ~MsgNode() {
        std::cout << "destruct MsgNode" << std::endl;
        delete[] _data;
    }

    void Clear() {
        std::memset(_data, 0, _total_len);
        _cur_len = 0;
    }

    short _cur_len;
    uint32_t _total_len;
    char* _data;
};

class RecvNode :public MsgNode {
    friend class MesRout;
public:
    RecvNode(uint32_t max_len, short msg_id);
private:
    short _msg_id;
};

class SendNode:public MsgNode {
    friend class LogicSystem;
public:
    SendNode(const char* msg,uint32_t max_len, short msg_id);
private:
    short _msg_id;
};