#ifndef LOGICNODE_H
#define LOGICNODE_H
#include "../network/tcpconnection.h"
#include "../network/msgnode.h"
#include <memory>
class LogicNode
{
    friend class MesRout;
public:
    LogicNode(std::shared_ptr<TcpConnection>,std::shared_ptr<RecvNode>);
private:
    std::shared_ptr<TcpConnection> _tcpconnection;
    std::shared_ptr<RecvNode> _recvnode;
};

#endif // LOGICNODE_H
