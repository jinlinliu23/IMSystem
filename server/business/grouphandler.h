#pragma once

#include "handler.h"

class CreateGroupHandler : public Handler
{
public:
    void handle(std::shared_ptr<TcpConnection> tc, const short &msg_id, const std::string &msg_data) override;
};

class GroupListHandler : public Handler
{
public:
    void handle(std::shared_ptr<TcpConnection> tc, const short &msg_id, const std::string &msg_data) override;
};

class GroupInfoHandler : public Handler
{
public:
    void handle(std::shared_ptr<TcpConnection> tc, const short &msg_id, const std::string &msg_data) override;
};

class InviteGroupHandler : public Handler
{
public:
    void handle(std::shared_ptr<TcpConnection> tc, const short &msg_id, const std::string &msg_data) override;
};

class LeaveGroupHandler : public Handler
{
public:
    void handle(std::shared_ptr<TcpConnection> tc, const short &msg_id, const std::string &msg_data) override;
};

class SendGroupMessageHandler : public Handler
{
public:
    void handle(std::shared_ptr<TcpConnection> tc, const short &msg_id, const std::string &msg_data) override;
};
