#ifndef ECHOHANDLER_H
#define ECHOHANDLER_H

#include "handler.h"

class EchoHandler : public Handler
{
public:
    EchoHandler();
    virtual void handle(std::shared_ptr<TcpConnection> _tc,const short& msg_id,const std::string& msg_data) override;
};

#endif // ECHOHANDLER_H
