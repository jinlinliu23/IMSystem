#include "service/groupservice.h"

#include "network/clientmessagerouter.h"

GroupService::GroupService(ClientMessageRouter *router, QObject *parent)
    : QObject(parent)
    , router_(router)
{
    Q_UNUSED(router_);
}
