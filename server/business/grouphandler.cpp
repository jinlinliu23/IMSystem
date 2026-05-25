#include "grouphandler.h"

#include "../database/databasemanager.h"
#include "../network/msgnode.h"
#include "../utility/messagedispatcher.h"
#include "../utility/offlinemessagekind.h"
#include "msg_ids.h"

#include <json/json.h>
#include <set>
#include <vector>

namespace {

constexpr size_t kMaxGroupNameLen = 64;
constexpr size_t kMaxGroupMembers = 50;

void sendJsonResponse(std::shared_ptr<TcpConnection> tc, uint16_t rspMsgId, const Json::Value &root)
{
    const std::string body = root.toStyledString();
    auto sn = std::make_shared<SendNode>(body.data(), static_cast<uint32_t>(body.size()),
                                         static_cast<short>(rspMsgId));
    tc->send(std::string(sn->_data, sn->_total_len));
}

void pushGroupEvent(const std::string &fromAccount,
                    const std::vector<std::string> &toAccounts,
                    const Json::Value &payload)
{
    MessageDispatcher::GetInstance()->deliverToAccounts(
        fromAccount,
        toAccounts,
        OfflineMessageKind::GroupNotify,
        static_cast<uint16_t>(MSG_IDS::MSG_NOTIFY_GROUP_EVENT),
        payload.toStyledString());
}

std::vector<std::string> getGroupMemberAccounts(const GroupInfoRecord &info)
{
    std::vector<std::string> accounts;
    accounts.reserve(info.members.size());
    for (const auto &m : info.members) {
        accounts.push_back(m.account);
    }
    return accounts;
}

} // namespace

void CreateGroupHandler::handle(std::shared_ptr<TcpConnection> tc,
                                const short & /*msg_id*/,
                                const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    Json::Value rsp;
    const auto rspId = static_cast<uint16_t>(MSG_IDS::MSG_CREATE_GROUP_RSP);

    if (!reader.parse(msg_data, root)) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "请求格式错误";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!root.isMember("creator_account") || !root.isMember("name")
        || !root.isMember("member_accounts")) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "缺少必填字段";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const std::string creatorAccount = root["creator_account"].asString();
    const std::string name = root["name"].asString();
    const Json::Value &memberArr = root["member_accounts"];

    if (creatorAccount.empty() || name.empty() || !memberArr.isArray()) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "参数不合法";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (name.size() > kMaxGroupNameLen) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "群名过长";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    auto db = DatabaseManager::GetInstance();
    const auto creator = db->findUserByAccount(creatorAccount);
    if (!creator.has_value()) {
        rsp["code"] = static_cast<int>(API_CODE::USER_NOT_FOUND);
        rsp["msg"] = "创建者不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    std::set<int64_t> memberIds;
    memberIds.insert(creator->id);

    for (const auto &item : memberArr) {
        if (!item.isString()) {
            continue;
        }
        const std::string account = item.asString();
        if (account.empty() || account == creatorAccount) {
            continue;
        }

        const auto user = db->findUserByAccount(account);
        if (!user.has_value()) {
            rsp["code"] = static_cast<int>(API_CODE::USER_NOT_FOUND);
            rsp["msg"] = "成员不存在: " + account;
            sendJsonResponse(tc, rspId, rsp);
            return;
        }

        if (!db->areFriends(creator->id, user->id)) {
            rsp["code"] = static_cast<int>(API_CODE::NOT_YOUR_FRIEND);
            rsp["msg"] = "只能邀请自己的好友: " + account;
            sendJsonResponse(tc, rspId, rsp);
            return;
        }

        memberIds.insert(user->id);
    }

    if (memberIds.size() < 2) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "群聊至少需要 2 人（含创建者），请勾选好友";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (memberIds.size() > kMaxGroupMembers) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "群成员过多";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    std::vector<int64_t> extraMembers;
    extraMembers.reserve(memberIds.size());
    for (const int64_t id : memberIds) {
        if (id != creator->id) {
            extraMembers.push_back(id);
        }
    }

    const auto groupId = db->createGroup(name, creator->id, extraMembers);
    if (!groupId.has_value()) {
        rsp["code"] = static_cast<int>(API_CODE::DB_ERROR);
        rsp["msg"] = "创建群失败";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    std::vector<std::string> memberAccounts;
    memberAccounts.reserve(memberIds.size());
    for (const int64_t id : memberIds) {
        if (id == creator->id) {
            continue;
        }
        const auto user = db->findUserById(id);
        if (user.has_value()) {
            memberAccounts.push_back(user->account);
        }
    }

    Json::Value event;
    event["type"] = "group_created";
    event["group_id"] = static_cast<Json::Int64>(*groupId);
    event["group_name"] = name;
    event["creator_account"] = creatorAccount;
    pushGroupEvent(creatorAccount, memberAccounts, event);

    rsp["code"] = static_cast<int>(API_CODE::OK);
    rsp["msg"] = "创建成功";
    rsp["group_id"] = static_cast<Json::Int64>(*groupId);
    rsp["name"] = name;
    sendJsonResponse(tc, rspId, rsp);
}

void GroupListHandler::handle(std::shared_ptr<TcpConnection> tc,
                              const short & /*msg_id*/,
                              const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    Json::Value rsp;
    const auto rspId = static_cast<uint16_t>(MSG_IDS::MSG_GROUP_LIST_RSP);

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
        rsp["code"] = static_cast<int>(API_CODE::USER_NOT_FOUND);
        rsp["msg"] = "用户不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const auto groups = DatabaseManager::GetInstance()->listMyGroups(user->id);
    Json::Value arr(Json::arrayValue);
    for (const auto &g : groups) {
        Json::Value item;
        item["group_id"] = static_cast<Json::Int64>(g.groupId);
        item["name"] = g.name;
        item["member_count"] = g.memberCount;
        arr.append(item);
    }

    rsp["code"] = static_cast<int>(API_CODE::OK);
    rsp["msg"] = "ok";
    rsp["groups"] = arr;
    sendJsonResponse(tc, rspId, rsp);
}

void GroupInfoHandler::handle(std::shared_ptr<TcpConnection> tc,
                              const short & /*msg_id*/,
                              const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    Json::Value rsp;
    const auto rspId = static_cast<uint16_t>(MSG_IDS::MSG_GROUP_INFO_RSP);

    if (!reader.parse(msg_data, root)) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "请求格式错误";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!root.isMember("group_id") || !root.isMember("account")) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "缺少必填字段";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const int64_t groupId = root["group_id"].asInt64();
    const std::string account = root["account"].asString();

    auto db = DatabaseManager::GetInstance();
    const auto user = db->findUserByAccount(account);
    if (!user.has_value()) {
        rsp["code"] = static_cast<int>(API_CODE::USER_NOT_FOUND);
        rsp["msg"] = "用户不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!db->groupExists(groupId)) {
        rsp["code"] = static_cast<int>(API_CODE::GROUP_NOT_FOUND);
        rsp["msg"] = "群不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const auto info = db->getGroupInfo(groupId, user->id);
    if (!info.has_value()) {
        rsp["code"] = static_cast<int>(API_CODE::NOT_GROUP_MEMBER);
        rsp["msg"] = "你不是群成员";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    Json::Value members(Json::arrayValue);
    for (const auto &m : info->members) {
        Json::Value item;
        item["account"] = m.account;
        item["nickname"] = m.nickname;
        item["role"] = m.role;
        members.append(item);
    }

    rsp["code"] = static_cast<int>(API_CODE::OK);
    rsp["msg"] = "ok";
    rsp["group_id"] = static_cast<Json::Int64>(info->groupId);
    rsp["name"] = info->name;
    rsp["dissolved"] = info->dissolved;
    rsp["members"] = members;
    sendJsonResponse(tc, rspId, rsp);
}

void InviteGroupHandler::handle(std::shared_ptr<TcpConnection> tc,
                                const short & /*msg_id*/,
                                const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    Json::Value rsp;
    const auto rspId = static_cast<uint16_t>(MSG_IDS::MSG_INVITE_GROUP_RSP);

    if (!reader.parse(msg_data, root)) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "请求格式错误";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }
    if (!root.isMember("group_id") || !root.isMember("inviter_account")
        || !root.isMember("invitee_accounts")) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "缺少必填字段";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const int64_t groupId = root["group_id"].asInt64();
    const std::string inviterAccount = root["inviter_account"].asString();
    const Json::Value &inviteeArr = root["invitee_accounts"];
    if (groupId <= 0 || inviterAccount.empty() || !inviteeArr.isArray()) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "参数不合法";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    auto db = DatabaseManager::GetInstance();
    const auto inviter = db->findUserByAccount(inviterAccount);
    if (!inviter.has_value()) {
        rsp["code"] = static_cast<int>(API_CODE::USER_NOT_FOUND);
        rsp["msg"] = "邀请人不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }
    if (!db->groupExists(groupId)) {
        rsp["code"] = static_cast<int>(API_CODE::GROUP_NOT_FOUND);
        rsp["msg"] = "群不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }
    if (!db->isGroupMember(groupId, inviter->id)) {
        rsp["code"] = static_cast<int>(API_CODE::NOT_GROUP_MEMBER);
        rsp["msg"] = "你不是群成员";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const auto info = db->getGroupInfo(groupId, inviter->id);
    if (!info.has_value()) {
        rsp["code"] = static_cast<int>(API_CODE::GROUP_NOT_FOUND);
        rsp["msg"] = "群不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    std::vector<int64_t> inviteeIds;
    std::vector<std::string> notifyAccounts;
    for (const auto &v : inviteeArr) {
        if (!v.isString()) continue;
        const std::string account = v.asString();
        if (account.empty()) continue;
        const auto user = db->findUserByAccount(account);
        if (!user.has_value()) {
            rsp["code"] = static_cast<int>(API_CODE::USER_NOT_FOUND);
            rsp["msg"] = "成员不存在: " + account;
            sendJsonResponse(tc, rspId, rsp);
            return;
        }
        if (!db->areFriends(inviter->id, user->id)) {
            rsp["code"] = static_cast<int>(API_CODE::NOT_YOUR_FRIEND);
            rsp["msg"] = "只能邀请自己的好友: " + account;
            sendJsonResponse(tc, rspId, rsp);
            return;
        }
        if (db->isGroupMember(groupId, user->id)) {
            rsp["code"] = static_cast<int>(API_CODE::ALREADY_IN_GROUP);
            rsp["msg"] = "已在群中: " + account;
            sendJsonResponse(tc, rspId, rsp);
            return;
        }
        inviteeIds.push_back(user->id);
        notifyAccounts.push_back(account);
    }

    if (inviteeIds.empty()) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "没有可邀请的成员";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!db->inviteUsersToGroup(groupId, inviter->id, inviteeIds)) {
        rsp["code"] = static_cast<int>(API_CODE::DB_ERROR);
        rsp["msg"] = "邀请失败";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const int64_t createdAt = static_cast<int64_t>(std::time(nullptr));
    Json::Value event;
    event["type"] = "member_joined";
    event["group_id"] = static_cast<Json::Int64>(groupId);
    event["group_name"] = info->name;
    event["account"] = inviterAccount;
    event["created_at"] = static_cast<Json::Int64>(createdAt);
    pushGroupEvent(inviterAccount, notifyAccounts, event);

    rsp["code"] = static_cast<int>(API_CODE::OK);
    rsp["msg"] = "邀请成功";
    sendJsonResponse(tc, rspId, rsp);
}

void LeaveGroupHandler::handle(std::shared_ptr<TcpConnection> tc,
                               const short & /*msg_id*/,
                               const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    Json::Value rsp;
    const auto rspId = static_cast<uint16_t>(MSG_IDS::MSG_LEAVE_GROUP_RSP);

    if (!reader.parse(msg_data, root)) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "请求格式错误";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }
    if (!root.isMember("group_id") || !root.isMember("account")) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "缺少必填字段";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const int64_t groupId = root["group_id"].asInt64();
    const std::string account = root["account"].asString();
    if (groupId <= 0 || account.empty()) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "参数不合法";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    auto db = DatabaseManager::GetInstance();
    const auto user = db->findUserByAccount(account);
    if (!user.has_value()) {
        rsp["code"] = static_cast<int>(API_CODE::USER_NOT_FOUND);
        rsp["msg"] = "用户不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }
    if (!db->groupExists(groupId)) {
        rsp["code"] = static_cast<int>(API_CODE::GROUP_NOT_FOUND);
        rsp["msg"] = "群不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }
    if (!db->isGroupMember(groupId, user->id)) {
        rsp["code"] = static_cast<int>(API_CODE::NOT_GROUP_MEMBER);
        rsp["msg"] = "你不是群成员";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const auto info = db->getGroupInfo(groupId, user->id);
    if (!info.has_value()) {
        rsp["code"] = static_cast<int>(API_CODE::GROUP_NOT_FOUND);
        rsp["msg"] = "群不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    bool dissolved = false;
    if (!db->leaveGroup(groupId, user->id, &dissolved)) {
        rsp["code"] = static_cast<int>(API_CODE::DB_ERROR);
        rsp["msg"] = "退群失败";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    std::vector<std::string> notifyAccounts;
    for (const auto &m : info->members) {
        if (m.account != account) {
            notifyAccounts.push_back(m.account);
        }
    }

    Json::Value event;
    event["type"] = dissolved ? "group_dissolved" : "member_left";
    event["group_id"] = static_cast<Json::Int64>(groupId);
    event["group_name"] = info->name;
    event["account"] = account;
    event["created_at"] = static_cast<Json::Int64>(std::time(nullptr));
    pushGroupEvent(account, notifyAccounts, event);

    rsp["code"] = static_cast<int>(API_CODE::OK);
    rsp["msg"] = dissolved ? "群已解散" : "已退群";
    rsp["dissolved"] = dissolved;
    sendJsonResponse(tc, rspId, rsp);
}

void SendGroupMessageHandler::handle(std::shared_ptr<TcpConnection> tc,
                                     const short & /*msg_id*/,
                                     const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    Json::Value rsp;
    const auto rspId = static_cast<uint16_t>(MSG_IDS::MSG_SEND_GROUP_RSP);

    if (!reader.parse(msg_data, root)) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "请求格式错误";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!root.isMember("group_id") || !root.isMember("from_account") || !root.isMember("content")) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "缺少必填字段";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const int64_t groupId = root["group_id"].asInt64();
    const std::string fromAccount = root["from_account"].asString();
    const std::string content = root["content"].asString();
    if (groupId <= 0 || fromAccount.empty() || content.empty()) {
        rsp["code"] = static_cast<int>(API_CODE::INVALID_PARAMS);
        rsp["msg"] = "参数不合法";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    auto db = DatabaseManager::GetInstance();
    const auto sender = db->findUserByAccount(fromAccount);
    if (!sender.has_value()) {
        rsp["code"] = static_cast<int>(API_CODE::USER_NOT_FOUND);
        rsp["msg"] = "发送者不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!db->groupExists(groupId)) {
        rsp["code"] = static_cast<int>(API_CODE::GROUP_NOT_FOUND);
        rsp["msg"] = "群不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    if (!db->isGroupMember(groupId, sender->id)) {
        rsp["code"] = static_cast<int>(API_CODE::NOT_GROUP_MEMBER);
        rsp["msg"] = "你不是群成员";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    const auto info = db->getGroupInfo(groupId, sender->id);
    if (!info.has_value()) {
        rsp["code"] = static_cast<int>(API_CODE::GROUP_NOT_FOUND);
        rsp["msg"] = "群不存在";
        sendJsonResponse(tc, rspId, rsp);
        return;
    }

    std::vector<std::string> targets;
    targets.reserve(info->members.size());
    for (const auto &m : info->members) {
        if (m.account != fromAccount) {
            targets.push_back(m.account);
        }
    }

    const int64_t createdAt = static_cast<int64_t>(std::time(nullptr));
    const int64_t messageId = createdAt * 1000 + (groupId % 1000);

    Json::Value push;
    push["group_id"] = static_cast<Json::Int64>(groupId);
    push["group_name"] = info->name;
    push["from_account"] = fromAccount;
    push["from_nickname"] = sender->nickname;
    push["content"] = content;
    push["message_id"] = static_cast<Json::Int64>(messageId);
    push["created_at"] = static_cast<Json::Int64>(createdAt);

    MessageDispatcher::GetInstance()->deliverToAccounts(
        fromAccount,
        targets,
        OfflineMessageKind::GroupChat,
        static_cast<uint16_t>(MSG_IDS::MSG_NOTIFY_GROUP),
        push.toStyledString(),
        groupId);

    rsp["code"] = static_cast<int>(API_CODE::OK);
    rsp["msg"] = "发送成功";
    rsp["group_id"] = static_cast<Json::Int64>(groupId);
    rsp["message_id"] = static_cast<Json::Int64>(messageId);
    rsp["created_at"] = static_cast<Json::Int64>(createdAt);
    sendJsonResponse(tc, rspId, rsp);
}
