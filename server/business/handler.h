#ifndef HANDLER_H
#define HANDLER_H

#include <memory>
#include "../network/tcpconnection.h"
class Handler
{
public:
    Handler();
    virtual ~Handler() = default;
    virtual void handle(std::shared_ptr<TcpConnection> _tc,const short& msg_id,const std::string& msg_data)=0;
};

#endif // HANDLER_H
