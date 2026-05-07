#include "socket.h"
#include <iostream>
#include <unistd.h>

Socket::Socket(int fd) :m_fd(fd){}

Socket::~Socket(){
    if(m_fd >= 0&&close(m_fd)==-1){
        std::cerr<<"关闭socket失败\n";
    }
}

int Socket::getfd(){
    return m_fd;
}