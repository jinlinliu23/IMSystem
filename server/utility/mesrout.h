#ifndef MESROUT_H
#define MESROUT_H

#include "logicnode.h"
#include "../business/handler.h"
class MesRout
{
public:
    MesRout();
public:
    void RoutMessage(std::shared_ptr<LogicNode>);

private:
    void RegisterHandler();//处理器注册
    std::map<short,std::shared_ptr<Handler>> _fun_Handlers;
};

#endif // MESROUT_H
