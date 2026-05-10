#include "logicnode.h"

LogicNode::LogicNode(std::shared_ptr<TcpConnection> tc, std::shared_ptr<RecvNode> rn)
    :_tcpconnection(tc),_recvnode(rn)
{}
