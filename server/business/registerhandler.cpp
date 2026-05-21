#include "registerhandler.h"

#include "../database/databasemanager.h"
#include "../network/msgnode.h"
#include "../utility/passwordhash.h"
#include "msg_ids.h"

#include <json/json.h>

#include <algorithm>
#include <cctype>
#include <iostream>

namespace {

/** 账号规则：类似微信号，3-32 位字母、数字或下划线，全局唯一且区分大小写 */
bool isValidAccount(const std::string &account)
{
    if (account.size() < 3 || account.size() > 32) {
        return false;
    }
    return std::all_of(account.begin(), account.end(), [](unsigned char c) {
        return std::isalnum(c) || c == '_';
    });
}

} // namespace

RegisterHandler::RegisterHandler() {}

void RegisterHandler::handle(std::shared_ptr<TcpConnection> tc,
                             const short & /*msg_id*/,
                             const std::string &msg_data)
{
    // ---------- 1. 解析 JSON 请求体 ----------
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        sendResponse(tc, static_cast<int>(API_CODE::INVALID_PARAMS), "请求格式错误");
        return;
    }

    // JSON 字段 username 表示「账号」（唯一标识）；nickname 表示「昵称」（展示名，可后续修改）
    if (!root.isMember("username") || !root.isMember("password") || !root.isMember("nickname")) {
        sendResponse(tc, static_cast<int>(API_CODE::INVALID_PARAMS), "缺少必填字段");
        return;
    }

    const std::string account = root["username"].asString();
    const std::string password = root["password"].asString();
    const std::string nickname = root["nickname"].asString();

    // ---------- 2. 业务校验 ----------
    if (!isValidAccount(account)) {
        sendResponse(tc, static_cast<int>(API_CODE::INVALID_PARAMS),
                     "账号需为 3-32 位字母、数字或下划线");
        return;
    }
    if (password.size() < 6 || password.size() > 64) {
        sendResponse(tc, static_cast<int>(API_CODE::INVALID_PARAMS), "密码长度需为 6-64 位");
        return;
    }
    if (nickname.empty() || nickname.size() > 32) {
        sendResponse(tc, static_cast<int>(API_CODE::INVALID_PARAMS), "昵称长度需为 1-32 位");
        return;
    }

    auto db = DatabaseManager::GetInstance();
    if (db->usernameExists(account)) {
        sendResponse(tc, static_cast<int>(API_CODE::USERNAME_EXISTS), "该账号已被注册，请更换其他账号");
        return;
    }

    // ---------- 3. 加盐哈希后写入 SQLite ----------
    try {
        const std::string salt = PasswordHash::generateSalt();
        const std::string passwordHash = PasswordHash::hashPassword(password, salt);
        const auto userId = db->insertUser(account, nickname, passwordHash, salt);
        if (!userId.has_value()) {
            sendResponse(tc, static_cast<int>(API_CODE::DB_ERROR), "注册失败，请稍后重试");
            return;
        }

        sendResponse(tc, static_cast<int>(API_CODE::OK), "注册成功", *userId);
    } catch (const std::exception &ex) {
        std::cerr << "RegisterHandler error: " << ex.what() << std::endl;
        sendResponse(tc, static_cast<int>(API_CODE::DB_ERROR), "服务器内部错误");
    }
}

void RegisterHandler::sendResponse(std::shared_ptr<TcpConnection> tc,
                                   int code,
                                   const std::string &msg,
                                   int64_t userId)
{
    // 响应 JSON 统一格式：{ "code", "msg", "user_id"? }
    Json::Value root;
    root["code"] = code;
    root["msg"] = msg;
    if (code == static_cast<int>(API_CODE::OK)) {
        root["user_id"] = static_cast<Json::Int64>(userId);
    }

    const std::string body = root.toStyledString();
    const auto rspId = static_cast<short>(MSG_IDS::MSG_REGISTER_RSP);
    auto sn = std::make_shared<SendNode>(body.data(), static_cast<uint32_t>(body.size()), rspId);
    tc->send(std::string(sn->_data, sn->_total_len));
}
