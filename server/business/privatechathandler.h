#pragma once

#include "handler.h"

class SendPrivateMessageHandler : public Handler
{
public:
    void handle(std::shared_ptr<TcpConnection> tc, const short &msg_id, const std::string &msg_data) override;
};
