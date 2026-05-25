# IMSystem

即时通讯（IM）课程项目：C++ 自研 TCP 服务端 + Qt6 Quick 客户端。

> **详细开发文档**（架构、协议、群聊分阶段计划、代码索引、待办）：见 [`PROJECT_STATUS.md`](PROJECT_STATUS.md)。

---

## 功能概览

| 模块 | 状态 | 说明 |
|------|------|------|
| 注册 / 登录 | ✅ | 1100/1101、1200/1201；密码盐 + OpenSSL |
| 好友 | ✅ | 搜索、申请、同意/拒绝、列表；1412 推送 |
| 单聊 | ✅ | 1300/1301 发送，1302 推送/离线；聊天记录仅存客户端 |
| 单端在线 | ✅ | 同账号仅允许一台设备登录（`ALREADY_ONLINE`） |
| 群聊 | ✅ 阶段 3 已完成 | 建群 / 群列表 / 群详情 / 群消息收发（1550/1502）；邀请/退群/防重复加群已实现 |
| 会话与未读 | ✅ | 消息列表实时更新；未读红点（右侧角标）；底部「消息」Tab 汇总 |

**业务规则**：先加好友再私聊；群邀请仅能邀请自己的好友（**阶段 3 实现**）；已是群成员不可重复加群（**阶段 3 实现**）。

---

## 架构摘要

```
客户端：QML → ClientFacade → Auth / Contact / Chat / Group → ClientMessageRouter → TcpClient
                              ↘ ChatLocalStore（Qt Sql，本地 sqlite）

服务端：TcpConnection → MesRout → Handler → DatabaseManager（im.db）
        OnlineUsersManager + MessageDispatcher（在线推送 + offline_messages）
```

TCP **16701**，帧格式：`[uint16 msg_id][uint32 len][UTF-8 JSON]`，定义见 `common/msg_ids.h`。

---

## 存储

| 位置 | 内容 |
|------|------|
| `server/data/im.db` | 用户、好友、`groups` / `group_members`、`offline_messages`（含 `group_id`） |
| 客户端 QSettings | 服务器地址、登录账号、`session_password`（`resumeSession`） |
| 客户端本地 | `AppDataLocation/chat_<账号>.sqlite`（`chat_messages`、`chat_conversations`） |

- **聊天历史**：私聊与群聊（会话键 `peer_account`，群为 `g:<group_id>`）**只存客户端**。
- **离线消息**：未送达写入 `offline_messages`；登录 / `resumeSession` 后 flush 为 1302 / 1502 / 1503 推送。

```bash
sqlite3 server/data/im.db "SELECT id, account, nickname FROM users;"
```

---

## 消息协议摘要

完整 ID 与错误码以 `common/msg_ids.h` 为准。

| 范围 | 用途 |
|------|------|
| 1100–1101 | 注册 |
| 1200–1201 | 登录 |
| 1300–1302 | 私聊 |
| 1400–1431、1412 | 好友 |
| 1500–1501 | 建群 |
| 1520–1521 | 群列表 |
| 1530–1531 | 群详情 |
| 1550–1551 | 发群消息 |
| 1502 | 群消息推送 |
| 1503 | 群事件推送（建群通知等，离线可投递） |

**API_CODE（常用）**：0 成功；4 认证失败；9 非好友；10 已在其它设备登录；11–15 群聊相关，见 `PROJECT_STATUS.md`。

---

## 构建与运行

### 服务端

依赖：jsoncpp、sqlite3、openssl（pkg-config）。

```bash
cd server
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/server
```

监听 **16701**；数据库首次运行创建 `server/data/im.db`。

### 客户端

推荐 **Qt Creator**（Qt 6.10+）：模块 **Quick**、**Network**、**Sql**。

```bash
cd client
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/appclient
```

| 环境 | 默认服务器地址 |
|------|----------------|
| 桌面 | `127.0.0.1:16701` |
| Android 模拟器 | `10.0.2.2:16701` |
| Android 真机 | 电脑局域网 IP；QML 可改 `ClientFacade.serverHost` |

修改 `CMakeLists.txt` 或 QML 模块后，在 Qt Creator 中 **重新运行 CMake** 再构建。

---

## 客户端页面

| 页面 | 说明 |
|------|------|
| Login / Register | 登录注册 |
| Main | Tab：消息 / 联系人 / 我（消息 Tab 显示未读汇总） |
| Messages | 会话列表（头像、最后消息、时间、未读角标）；「+」添加好友 / 创建群聊 |
| Chat | 私聊 / 群聊；气泡旁为昵称文字头像（`ImAvatar`） |
| Contacts | 好友与群列表；群入口进 `GroupInfoPage` |
| CreateGroup / GroupInfo | 建群、查看群成员 |
| AddFriend / FriendRequests | 好友 |
| Profile | 个人信息、退出登录 |

---

## 当前进度与下一步

| 阶段 | 内容 | 状态 |
|------|------|------|
| 1 | 建群 + 群列表 + 群详情 | ✅ 已完成 |
| 2 | 群消息收发 + 本地存储 + 会话列表 | ✅ 已完成 |
| 3 | 邀请 / 退群 / 防重复加群 | ⏳ 待做 |
| 4 | 空群销毁 + 本地归档（已解散） | ⏳ 待做 |
| 5 | 体验优化（Router 队列、心跳等） | 部分（未读、排序已有） |


