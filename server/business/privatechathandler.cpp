#include "privatechathandler.h"

#include "../database/databasemanager.h"
#include "../network/msgnode.h"
#include "../utility/messagedispatcher.h"
#include "../utility/offlinemessagekind.h"
#include "msg_ids.h"

#include <chrono>
#include <functional>
#include <json/json.h>

namespace {

constexpr size_t kMaxPrivateContentLen = 2000;

void sendJsonResponse(std::shared_ptr<TcpConnection> tc, uint16_t rspMsgId, const Json::Value &root)
{
    const std::string body = root.toStyledString();
    auto sn = std::make_shared<SendNode>(body.data(), static_cast<uint32_t>(body.size()),
                                         static_cast<short>(rspMsgId));
    tc->send(std::string(sn->_data, sn->_total_len));
}

int64_t nowUnixSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

} // namespace

void SendPrivateMessageHandler::handle(std::shared_ptr<TcpConnection> tc,
                                       const short & /*msg_id*/,
                                       const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    Json::Value rsp;
    const auto rspId = static_cast<uint16_t>(MSG_IDS::MSG_SEND_PRIVATE_RSP);

    if (!reader.parse(msg_data, root)) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "请求格式错误";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!root.isMember("from_account") || !root.isMember("to_account")
        || !root.isMember("content")) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "缺少必填字段";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const std::string fromAccount = root["from_account"].asString();
    const std::string toAccount = root["to_account"].asString();
    const std::string content = root["content"].asString();

    if (fromAccount.empty() || toAccount.empty() || content.empty()) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "消息内容不能为空";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (content.size() > kMaxPrivateContentLen) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "消息过长";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

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
        rsp["msg"] = "不能给自己发消息";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!db->areFriends(fromUser->id, toUser->id)) {
        rsp["code"] = static_cast<int>(API_CODE::NOT_FRIEND);
        rsp["msg"] = "只能给好友发消息";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const int64_t createdAt = nowUnixSeconds();
    const int64_t messageId = createdAt * 1000000
                              + static_cast<int64_t>(std::hash<std::string>{}(content + fromAccount)
                                                     % 1000000);

    Json::Value push;
    push["from_account"] = fromAccount;
    push["from_nickname"] = fromUser->nickname;
    push["to_account"] = toAccount;
    push["content"] = content;
    push["message_id"] = static_cast<Json::Int64>(messageId);
    push["created_at"] = static_cast<Json::Int64>(createdAt);

    MessageDispatcher::GetInstance()->deliverToAccount(
        fromAccount,
        toAccount,
        OfflineMessageKind::PrivateChat,
        static_cast<uint16_t>(MSG_IDS::MSG_NOTIFY_PRIVATE),
        push.toStyledString());

    rsp["code"] = static_cast<int>(API_CODE::OK);
    rsp["msg"] = "ok";
    rsp["message_id"] = static_cast<Json::Int64>(messageId);
    rsp["created_at"] = static_cast<Json::Int64>(createdAt);
    sendJsonResponse(tc, rspId, rsp);
}
