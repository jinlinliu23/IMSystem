#include "loginhandler.h"

#include "../database/databasemanager.h"
#include "../network/msgnode.h"
#include "../utility/passwordhash.h"
#include "msg_ids.h"

#include <json/json.h>

LoginHandler::LoginHandler() {}

void LoginHandler::handle(std::shared_ptr<TcpConnection> tc,
                          const short & /*msg_id*/,
                          const std::string &msg_data)
{
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(msg_data, root)) {
        sendResponse(tc, static_cast<int>(API_CODE::INVALID_PARAMS), "请求格式错误");
        return;
    }

    if (!root.isMember("username") || !root.isMember("password")) {
        sendResponse(tc, static_cast<int>(API_CODE::INVALID_PARAMS), "缺少必填字段");
        return;
    }

    const std::string username = root["username"].asString();
    const std::string password = root["password"].asString();

    if (username.empty() || password.empty()) {
        sendResponse(tc, static_cast<int>(API_CODE::INVALID_PARAMS), "账号和密码不能为空");
        return;
    }

    auto db = DatabaseManager::GetInstance();
    const auto user = db->findUserAuthByUsername(username);
    if (!user.has_value()) {
        sendResponse(tc, static_cast<int>(API_CODE::AUTH_FAILED), "账号或密码错误");
        return;
    }

    try {
        const std::string hash = PasswordHash::hashPassword(password, user->salt);
        if (hash != user->passwordHash) {
            sendResponse(tc, static_cast<int>(API_CODE::AUTH_FAILED), "账号或密码错误");
            return;
        }

        sendResponse(tc,
                     static_cast<int>(API_CODE::OK),
                     "登录成功",
                     user->id,
                     user->username,
                     user->nickname);
    } catch (const std::exception &ex) {
        sendResponse(tc, static_cast<int>(API_CODE::DB_ERROR), "服务器内部错误");
    }
}

void LoginHandler::sendResponse(std::shared_ptr<TcpConnection> tc,
                                int code,
                                const std::string &msg,
                                int64_t userId,
                                const std::string &username,
                                const std::string &nickname)
{
    Json::Value root;
    root["code"] = code;
    root["msg"] = msg;
    if (code == static_cast<int>(API_CODE::OK)) {
        root["user_id"] = static_cast<Json::Int64>(userId);
        root["username"] = username;
        root["nickname"] = nickname;
    }

    const std::string body = root.toStyledString();
    const auto rspId = static_cast<short>(MSG_IDS::MSG_LOGIN_RSP);
    auto sn = std::make_shared<SendNode>(body.data(), static_cast<uint32_t>(body.size()), rspId);
    tc->send(std::string(sn->_data, sn->_total_len));
}
