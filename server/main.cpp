#include "network/eventloop.h"
#include "network/tcpserver.h"

int main()
{
    //创建TcpServer
    EventLoop eventloop{};
    TcpServer tcpserver{&eventloop,16701};
    //启动事件循环
    tcpserver.start();
    eventloop.run();
    return 0;
}
