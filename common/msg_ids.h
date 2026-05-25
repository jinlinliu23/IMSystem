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

    MSG_SEARCH_USER = 1400,     ///< 搜索用户（加好友前，按账号精确匹配）
    MSG_SEARCH_USER_RSP = 1401,
    MSG_FRIEND_REQUEST = 1410,  ///< 发送好友申请
    MSG_FRIEND_REQUEST_RSP = 1411,
    MSG_FRIEND_REQUEST_LIST = 1420, ///< 拉取收到的好友申请（待处理）
    MSG_FRIEND_REQUEST_LIST_RSP = 1421,
    MSG_FRIEND_ACCEPT = 1422,       ///< 同意好友申请
    MSG_FRIEND_ACCEPT_RSP = 1423,
    MSG_FRIEND_REJECT = 1424,       ///< 拒绝好友申请
    MSG_FRIEND_REJECT_RSP = 1425,
    MSG_FRIEND_LIST = 1430,         ///< 拉取好友列表
    MSG_FRIEND_LIST_RSP = 1431,

    MSG_SEND_PRIVATE = 1300,        ///< 发送私聊
    MSG_SEND_PRIVATE_RSP = 1301,
    MSG_NOTIFY_PRIVATE = 1302,      ///< 服务端推送：私聊消息
    MSG_PRIVATE_HISTORY = 1303,     ///< 已废弃：历史由客户端本地 SQLite 保存
    MSG_PRIVATE_HISTORY_RSP = 1304,

    MSG_NOTIFY_FRIEND = 1412,      ///< 服务端推送：好友相关通知（离线落库）
    MSG_NOTIFY_GROUP = 1502,       ///< 服务端推送：群消息
    MSG_NOTIFY_GROUP_EVENT = 1503, ///< 服务端推送：群事件（离线落库）

    MSG_CREATE_GROUP = 1500,
    MSG_CREATE_GROUP_RSP = 1501,
    MSG_GROUP_LIST = 1520,
    MSG_GROUP_LIST_RSP = 1521,
    MSG_GROUP_INFO = 1530,
    MSG_GROUP_INFO_RSP = 1531,
    MSG_INVITE_GROUP = 1510,
    MSG_INVITE_GROUP_RSP = 1511,
    MSG_LEAVE_GROUP = 1540,
    MSG_LEAVE_GROUP_RSP = 1541,
    MSG_SEND_GROUP = 1550,
    MSG_SEND_GROUP_RSP = 1551,
};

/** 业务层 JSON 字段 code 的取值（注册/登录等通用） */
enum class API_CODE : int {
    OK = 0,              ///< 成功
    ACCOUNT_EXISTS = 1,  ///< 账号已被注册
    INVALID_PARAMS = 2,  ///< 参数不合法
    DB_ERROR = 3,        ///< 数据库或服务器内部错误
    AUTH_FAILED = 4,     ///< 账号或密码错误（登录）
    USER_NOT_FOUND = 5,  ///< 用户不存在
    ALREADY_FRIEND = 6,  ///< 已是好友
    FRIEND_REQUEST_PENDING = 7, ///< 好友申请已存在/待处理
    CANNOT_ADD_SELF = 8, ///< 不能添加自己
    NOT_FRIEND = 9,      ///< 非好友，无法发私聊
    ALREADY_ONLINE = 10, ///< 账号已在其它设备登录

    GROUP_NOT_FOUND = 11,
    NOT_GROUP_MEMBER = 12,
    ALREADY_IN_GROUP = 13,
    GROUP_DISSOLVED = 14,
    NOT_YOUR_FRIEND = 15,
};
