#include "mesrout.h"
#include "msgIdconst.h"
#include "../business/loginhandler.h"
#include "../business/registerhandler.h"

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
}
