#include "searchuserhandler.h"

#include "../database/databasemanager.h"
#include "../network/msgnode.h"
#include "msg_ids.h"

#include <json/json.h>

SearchUserHandler::SearchUserHandler() {}

void SearchUserHandler::handle(std::shared_ptr<TcpConnection> tc,
                               const short & /*msg_id*/,
                               const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        sendResponse(tc, static_cast<int>(API_CODE::INVALID_PARAMS), "请求格式错误");
        return;
    }

    if (!root.isMember("from_account") || !root.isMember("account")) {
        sendResponse(tc, static_cast<int>(API_CODE::INVALID_PARAMS), "缺少必填字段");
        return;
    }

    const std::string fromAccount = root["from_account"].asString();
    const std::string account = root["account"].asString();

    if (fromAccount.empty() || account.empty()) {
        sendResponse(tc, static_cast<int>(API_CODE::INVALID_PARAMS), "参数不合法");
        return;
    }

    auto db = DatabaseManager::GetInstance();
    if (!db->findUserByAccount(fromAccount).has_value()) {
        sendResponse(tc, static_cast<int>(API_CODE::INVALID_PARAMS), "当前用户无效");
        return;
    }

    const auto target = db->findUserByAccount(account);
    if (!target.has_value()) {
        sendResponse(tc, static_cast<int>(API_CODE::USER_NOT_FOUND), "未找到该账号");
        return;
    }

    if (target->account == fromAccount) {
        sendResponse(tc, static_cast<int>(API_CODE::CANNOT_ADD_SELF), "不能添加自己为好友");
        return;
    }

    sendResponse(tc, static_cast<int>(API_CODE::OK), "ok", &(*target));
}

void SearchUserHandler::sendResponse(std::shared_ptr<TcpConnection> tc,
                                     int code,
                                     const std::string &msg,
                                     const UserRecord *user)
{
    Json::Value root;
    root["code"] = code;
    root["msg"] = msg;
    if (code == static_cast<int>(API_CODE::OK) && user != nullptr) {
        Json::Value u;
        u["user_id"] = static_cast<Json::Int64>(user->id);
        u["account"] = user->account;
        u["nickname"] = user->nickname;
        root["user"] = u;
    }

    const std::string body = root.toStyledString();
    const auto rspId = static_cast<short>(MSG_IDS::MSG_SEARCH_USER_RSP);
    auto sn = std::make_shared<SendNode>(body.data(), static_cast<uint32_t>(body.size()), rspId);
    tc->send(std::string(sn->_data, sn->_total_len));
}
