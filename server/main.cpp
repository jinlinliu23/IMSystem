#include "database/databasemanager.h"
#include "network/eventloop.h"
#include "network/tcpserver.h"

#include <iostream>

int main()
{
    // 嵌入式 SQLite：在服务端工作目录下创建/打开 data/im.db（非独立数据库服务器）
    if (!DatabaseManager::GetInstance()->init("data/im.db")) {
        std::cerr << "Failed to initialize database (data/im.db)" << std::endl;
        return 1;
    }

    std::cout << "Database ready: data/im.db" << std::endl;

    EventLoop eventloop{};
    TcpServer tcpserver{&eventloop, 16701};
    tcpserver.start();
    std::cout << "IM server listening on port 16701" << std::endl;
    eventloop.run();
    return 0;
}
