#pragma once

#include "handler.h"

class SendFriendRequestHandler : public Handler
{
public:
    void handle(std::shared_ptr<TcpConnection> tc, const short &msg_id, const std::string &msg_data) override;
};

class FriendRequestListHandler : public Handler
{
public:
    void handle(std::shared_ptr<TcpConnection> tc, const short &msg_id, const std::string &msg_data) override;
};

class AcceptFriendRequestHandler : public Handler
{
public:
    void handle(std::shared_ptr<TcpConnection> tc, const short &msg_id, const std::string &msg_data) override;
};

class RejectFriendRequestHandler : public Handler
{
public:
    void handle(std::shared_ptr<TcpConnection> tc, const short &msg_id, const std::string &msg_data) override;
};

class FriendListHandler : public Handler
{
public:
    void handle(std::shared_ptr<TcpConnection> tc, const short &msg_id, const std::string &msg_data) override;
};
