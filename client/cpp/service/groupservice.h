#pragma once

#include <QObject>

class ClientMessageRouter;

/**
 * @brief 群聊（后续实现）
 */
class GroupService : public QObject
{
    Q_OBJECT
public:
    explicit GroupService(ClientMessageRouter *router, QObject *parent = nullptr);

private:
    ClientMessageRouter *router_ = nullptr;
};
