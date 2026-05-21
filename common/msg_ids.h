#pragma once

#include <cstdint>

/**
 * @file msg_ids.h
 * @brief 客户端与服务端共用的消息 ID 与业务错误码（双方必须一致）
 *
 * 传输层帧格式（TCP 二进制，与服务端 network/networkconst.h 一致）：
 *   [2 字节 msg_id  网络序 uint16]
 *   [4 字节 body 长度 网络序 uint32]
 *   [body 字节，当前统一为 UTF-8 JSON]
 */

enum class MSG_IDS : uint16_t {
    MSG_REGISTER = 1100,     ///< 客户端 → 服务端：注册请求
    MSG_REGISTER_RSP = 1101, ///< 服务端 → 客户端：注册响应
    MSG_LOGIN = 1200,        ///< 客户端 → 服务端：登录请求
    MSG_LOGIN_RSP = 1201,    ///< 服务端 → 客户端：登录响应
    // 后续扩展示例（尚未实现）：
    // MSG_SEND_PRIVATE = 1300,
    // MSG_RECV_PRIVATE = 1301,
};

/** 业务层 JSON 字段 code 的取值（注册/登录等通用） */
enum class API_CODE : int {
    OK = 0,              ///< 成功
    USERNAME_EXISTS = 1, ///< 账号已被注册（注册，JSON 字段名仍为 username）
    INVALID_PARAMS = 2,  ///< 参数不合法
    DB_ERROR = 3,        ///< 数据库或服务器内部错误
    AUTH_FAILED = 4,     ///< 账号或密码错误（登录）
};
