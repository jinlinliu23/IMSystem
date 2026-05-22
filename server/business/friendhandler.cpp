#include "friendhandler.h"

#include "../database/databasemanager.h"
#include "../network/msgnode.h"
#include "../utility/messagedispatcher.h"
#include "../utility/offlinemessagekind.h"
#include "msg_ids.h"

#include <json/json.h>

namespace {

void sendJsonResponse(std::shared_ptr<TcpConnection> tc, uint16_t rspMsgId, const Json::Value &root)
{
    const std::string body = root.toStyledString();
    auto sn = std::make_shared<SendNode>(body.data(), static_cast<uint32_t>(body.size()),
                                         static_cast<short>(rspMsgId));
    tc->send(std::string(sn->_data, sn->_total_len));
}

void notifyFriendEvent(const std::string &fromAccount,
                       const std::string &toAccount,
                       const std::string &type,
                       int64_t requestId,
                       const std::string &fromNickname)
{
    Json::Value push;
    push["type"] = type;
    push["from_account"] = fromAccount;
    push["from_nickname"] = fromNickname;
    if (requestId > 0) {
        push["request_id"] = static_cast<Json::Int64>(requestId);
    }

    MessageDispatcher::GetInstance()->deliverToAccount(
        fromAccount,
        toAccount,
        OfflineMessageKind::FriendNotify,
        static_cast<uint16_t>(MSG_IDS::MSG_NOTIFY_FRIEND),
        push.toStyledString());
}

} // namespace

void SendFriendRequestHandler::handle(std::shared_ptr<TcpConnection> tc,
                                      const short & /*msg_id*/,
                                      const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    Json::Value rsp;
    const auto rspId = static_cast<uint16_t>(MSG_IDS::MSG_FRIEND_REQUEST_RSP);

    if (!reader.parse(msg_data, root)) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "请求格式错误";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!root.isMember("from_account") || !root.isMember("to_account")) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "缺少必填字段";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const std::string fromAccount = root["from_account"].asString();
    const std::string toAccount = root["to_account"].asString();

    auto db = DatabaseManager::GetInstance();
    const auto fromUser = db->findUserByAccount(fromAccount);
    const auto toUser = db->findUserByAccount(toAccount);

    if (!fromUser.has_value() || !toUser.has_value()) {
        rsp["code"] = static_cast<int>(API_CODE::USER_NOT_FOUND);
        rsp["msg"] = "用户不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (fromUser->id == toUser->id) {
        rsp["code"] = static_cast<int>(API_CODE::CANNOT_ADD_SELF);
        rsp["msg"] = "不能添加自己";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (db->areFriends(fromUser->id, toUser->id)) {
        rsp["code"] = static_cast<int>(API_CODE::ALREADY_FRIEND);
        rsp["msg"] = "你们已经是好友";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (db->hasPendingFriendRequest(fromUser->id, toUser->id)) {
        rsp["code"] = static_cast<int>(API_CODE::FRIEND_REQUEST_PENDING);
        rsp["msg"] = "好友申请已存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!db->insertFriendRequest(fromUser->id, toUser->id)) {
        rsp["code"] = static_cast<int>(API_CODE::DB_ERROR);
        rsp["msg"] = "发送失败";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const auto pending = db->listPendingIncomingRequests(toUser->id);
    int64_t requestId = 0;
    for (const auto &row : pending) {
        if (row.fromUserId == fromUser->id) {
            requestId = row.requestId;
            break;
        }
    }

    notifyFriendEvent(fromAccount, toAccount, "friend_request", requestId, fromUser->nickname);

    rsp["code"] = static_cast<int>(API_CODE::OK);
    rsp["msg"] = "好友申请已发送";
    sendJsonResponse(tc, rspId, rsp);
}

void FriendRequestListHandler::handle(std::shared_ptr<TcpConnection> tc,
                                      const short & /*msg_id*/,
                                      const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    Json::Value rsp;
    const auto rspId = static_cast<uint16_t>(MSG_IDS::MSG_FRIEND_REQUEST_LIST_RSP);

    if (!reader.parse(msg_data, root)) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "请求格式错误";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!root.isMember("account")) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "缺少 account";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const auto user = DatabaseManager::GetInstance()->findUserByAccount(root["account"].asString());
    if (!user.has_value()) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "用户无效";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const auto rows = DatabaseManager::GetInstance()->listPendingIncomingRequests(user->id);
    Json::Value requests(Json::arrayValue);
    for (const auto &row : rows) {
        Json::Value item;
        item["request_id"] = static_cast<Json::Int64>(row.requestId);
        item["from_user_id"] = static_cast<Json::Int64>(row.fromUserId);
        item["from_account"] = row.fromAccount;
        item["from_nickname"] = row.fromNickname;
        requests.append(item);
    }

    rsp["code"] = static_cast<int>(API_CODE::OK);
    rsp["msg"] = "ok";
    rsp["requests"] = requests;
    sendJsonResponse(tc, rspId, rsp);
}

void AcceptFriendRequestHandler::handle(std::shared_ptr<TcpConnection> tc,
                                        const short & /*msg_id*/,
                                        const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    Json::Value rsp;
    const auto rspId = static_cast<uint16_t>(MSG_IDS::MSG_FRIEND_ACCEPT_RSP);

    if (!reader.parse(msg_data, root)) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "请求格式错误";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!root.isMember("account") || !root.isMember("from_account")) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "缺少必填字段";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const std::string myAccount = root["account"].asString();
    const std::string fromAccount = root["from_account"].asString();

    auto db = DatabaseManager::GetInstance();
    const auto me = db->findUserByAccount(myAccount);
    const auto fromUser = db->findUserByAccount(fromAccount);

    if (!me.has_value() || !fromUser.has_value()) {
        rsp["code"] = static_cast<int>(API_CODE::USER_NOT_FOUND);
        rsp["msg"] = "用户不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!db->hasPendingFriendRequest(fromUser->id, me->id)) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "没有待处理的好友申请";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!db->acceptFriendRequest(fromUser->id, me->id)) {
        rsp["code"] = static_cast<int>(API_CODE::DB_ERROR);
        rsp["msg"] = "操作失败";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    notifyFriendEvent(me->account, fromAccount, "friend_accepted", 0, me->nickname);

    rsp["code"] = static_cast<int>(API_CODE::OK);
    rsp["msg"] = "已成为好友";
    sendJsonResponse(tc, rspId, rsp);
}

void RejectFriendRequestHandler::handle(std::shared_ptr<TcpConnection> tc,
                                        const short & /*msg_id*/,
                                        const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    Json::Value rsp;
    const auto rspId = static_cast<uint16_t>(MSG_IDS::MSG_FRIEND_REJECT_RSP);

    if (!reader.parse(msg_data, root)) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "请求格式错误";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!root.isMember("account") || !root.isMember("from_account")) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "缺少必填字段";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const std::string myAccount = root["account"].asString();
    const std::string fromAccount = root["from_account"].asString();

    auto db = DatabaseManager::GetInstance();
    const auto me = db->findUserByAccount(myAccount);
    const auto fromUser = db->findUserByAccount(fromAccount);

    if (!me.has_value() || !fromUser.has_value()) {
        rsp["code"] = static_cast<int>(API_CODE::USER_NOT_FOUND);
        rsp["msg"] = "用户不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!db->hasPendingFriendRequest(fromUser->id, me->id)) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "没有待处理的好友申请";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!db->rejectFriendRequest(fromUser->id, me->id)) {
        rsp["code"] = static_cast<int>(API_CODE::DB_ERROR);
        rsp["msg"] = "操作失败";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    notifyFriendEvent(me->account, fromAccount, "friend_rejected", 0, me->nickname);

    rsp["code"] = static_cast<int>(API_CODE::OK);
    rsp["msg"] = "已拒绝好友申请";
    sendJsonResponse(tc, rspId, rsp);
}

void FriendListHandler::handle(std::shared_ptr<TcpConnection> tc,
                               const short & /*msg_id*/,
                               const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    Json::Value rsp;
    const auto rspId = static_cast<uint16_t>(MSG_IDS::MSG_FRIEND_LIST_RSP);

    if (!reader.parse(msg_data, root)) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "请求格式错误";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!root.isMember("account")) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "缺少 account";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const auto user = DatabaseManager::GetInstance()->findUserByAccount(root["account"].asString());
    if (!user.has_value()) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "用户无效";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const auto friends = DatabaseManager::GetInstance()->listFriends(user->id);
    Json::Value arr(Json::arrayValue);
    for (const auto &f : friends) {
        Json::Value item;
        item["user_id"] = static_cast<Json::Int64>(f.id);
        item["account"] = f.account;
        item["nickname"] = f.nickname;
        arr.append(item);
    }

    rsp["code"] = static_cast<int>(API_CODE::OK);
    rsp["msg"] = "ok";
    rsp["friends"] = arr;
    sendJsonResponse(tc, rspId, rsp);
}
