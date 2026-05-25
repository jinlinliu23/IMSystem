#include "mesrout.h"
#include "msgIdconst.h"
#include "../business/loginhandler.h"
#include "../business/registerhandler.h"
#include "../business/friendhandler.h"
#include "../business/searchuserhandler.h"
#include "../business/privatechathandler.h"
#include "../business/grouphandler.h"

MesRout::MesRout()
{
    initHandlers();
}

void MesRout::RoutMessage(std::shared_ptr<LogicNode> ln)
{
    auto _handler_iter = _fun_Handlers.find(ln->_recvnode->_msg_id);
    if (_handler_iter == _fun_Handlers.end()) {
        return;
    }
    _handler_iter->second->handle(ln->_tcpconnection,
                                  ln->_recvnode->_msg_id,
                                  ln->_recvnode->_data);
}

void MesRout::initHandlers()
{
    _fun_Handlers[static_cast<uint32_t>(MSG_IDS::MSG_REGISTER)] = std::make_shared<RegisterHandler>();
    _fun_Handlers[static_cast<uint32_t>(MSG_IDS::MSG_LOGIN)] = std::make_shared<LoginHandler>();
    _fun_Handlers[static_cast<uint32_t>(MSG_IDS::MSG_SEND_PRIVATE)] =
        std::make_shared<SendPrivateMessageHandler>();
    _fun_Handlers[static_cast<uint32_t>(MSG_IDS::MSG_SEARCH_USER)] = std::make_shared<SearchUserHandler>();
    _fun_Handlers[static_cast<uint32_t>(MSG_IDS::MSG_FRIEND_REQUEST)] =
        std::make_shared<SendFriendRequestHandler>();
    _fun_Handlers[static_cast<uint32_t>(MSG_IDS::MSG_FRIEND_REQUEST_LIST)] =
        std::make_shared<FriendRequestListHandler>();
    _fun_Handlers[static_cast<uint32_t>(MSG_IDS::MSG_FRIEND_ACCEPT)] =
        std::make_shared<AcceptFriendRequestHandler>();
    _fun_Handlers[static_cast<uint32_t>(MSG_IDS::MSG_FRIEND_REJECT)] =
        std::make_shared<RejectFriendRequestHandler>();
    _fun_Handlers[static_cast<uint32_t>(MSG_IDS::MSG_FRIEND_LIST)] = std::make_shared<FriendListHandler>();
    _fun_Handlers[static_cast<uint32_t>(MSG_IDS::MSG_CREATE_GROUP)] =
        std::make_shared<CreateGroupHandler>();
    _fun_Handlers[static_cast<uint32_t>(MSG_IDS::MSG_GROUP_LIST)] =
        std::make_shared<GroupListHandler>();
    _fun_Handlers[static_cast<uint32_t>(MSG_IDS::MSG_GROUP_INFO)] =
        std::make_shared<GroupInfoHandler>();
    _fun_Handlers[static_cast<uint32_t>(MSG_IDS::MSG_INVITE_GROUP)] =
        std::make_shared<InviteGroupHandler>();
    _fun_Handlers[static_cast<uint32_t>(MSG_IDS::MSG_LEAVE_GROUP)] =
        std::make_shared<LeaveGroupHandler>();
    _fun_Handlers[static_cast<uint32_t>(MSG_IDS::MSG_SEND_GROUP)] =
        std::make_shared<SendGroupMessageHandler>();
}
