# IMSystem

即时通讯（IM）课程项目：C++ 自研 TCP 服务端 + Qt6 Quick 客户端。

> **详细开发文档**（架构、协议草案、群聊分阶段计划、代码索引）：见 [`PROJECT_STATUS.md`](PROJECT_STATUS.md)。

---

## 功能概览

| 模块 | 状态 | 说明 |
|------|------|------|
| 注册 / 登录 | ✅ | 1100/1101、1200/1201；密码盐 + OpenSSL |
| 好友 | ✅ | 搜索、申请、同意/拒绝、列表；1412 推送 |
| 单聊 | ✅ | 1300/1301 发送，1302 推送/离线；聊天记录仅存客户端 |
| 单端在线 | ✅ | 同账号仅允许一台设备登录（`ALREADY_ONLINE`） |
| 群聊 | 🚧 第一阶段进行中 | 已完成群创建 / 群列表 / 群详情的基础接入；群消息收发与离线通知仍在完善中，详见 `PROJECT_STATUS.md` |

**业务规则**：先加好友再私聊；群邀请仅能邀请自己的好友；已是群成员不可重复加群（群聊实现后生效）。

---

## 架构摘要

```
客户端：QML → ClientFacade → Auth / Contact / Chat / Group* → ClientMessageRouter → TcpClient
                              ↘ ChatLocalStore（Qt Sql，本地 sqlite 文件）

服务端：TcpConnection → MesRout → Handler → DatabaseManager（im.db）
        OnlineUsersManager + MessageDispatcher（在线推送 + offline_messages）
```

TCP **16701**，帧格式：`[uint16 msg_id][uint32 len][UTF-8 JSON]`，定义见 `common/msg_ids.h`。

\* `GroupService` 已接入群创建 / 群列表 / 群详情，后续继续补群消息收发与离线投递。

---

## 存储

| 位置 | 内容 |
|------|------|
| `server/data/im.db` | 用户、好友关系、`offline_messages`（含 `group_id` 字段） |
| 客户端 QSettings | 服务器地址、当前登录账号、`session_password`（用于启动时 `resumeSession`） |
| 客户端本地 | `AppDataLocation/chat_<账号>.sqlite`（Qt **QSQLITE** 驱动） |

- **聊天历史**：私聊（及规划中的群聊）**只存客户端**；服务端不保存聊天内容表。
- **离线消息**：未送达写入 `offline_messages`，登录或 `resumeSession` 后 flush 为 1302/1502/1503 推送（群事件也走离线队列）。

查看服务端用户示例：

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
| 1300–1302 | 私聊（发 / 响应 / 推送） |
| 1400–1431 | 好友 |
| 1412 | 好友事件推送 |
| 1500–1555 | 群聊（阶段 1 已实现创建 / 列表 / 详情，后续继续补消息与邀请） |
| 1502 | 群消息推送（已预留，Handler 待补） |
| 1503 | 群事件推送（建群 / 退群 / 解散，离线可投递） |

**API_CODE（常用）**：0 成功；4 认证失败；9 非好友；10 已在其它设备登录。群聊扩展码 11–15 见 `PROJECT_STATUS.md`。

### 注册 / 登录示例

注册 `1100`：

```json
{"account":"alice","password":"secret123","nickname":"爱丽丝"}
```

登录 `1200`：

```json
{"account":"alice","password":"secret123"}
```

登录成功 `1201`：

```json
{"code":0,"msg":"登录成功","user_id":1,"account":"alice","nickname":"爱丽丝"}
```

客户端将登录态写入 QSettings；下次启动若已登录则调用 `resumeSession()` 向服务端重新 1200 并拉取离线消息。

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

- 监听：**16701**
- 数据库：首次运行自动创建 `server/data/im.db`

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
| Android 真机 | 电脑局域网 IP，可在 QML 中设置 `ClientFacade.serverHost` |

修改 `CMakeLists.txt` 或 Qt 模块后请在 Qt Creator 中 **重新运行 CMake** 再构建。

---

## 客户端页面

| 页面 | 说明 |
|------|------|
| Login / Register | 登录注册 |
| Main（消息 / 联系人 / 我） | 主页 Tab |
| Messages | 会话列表；「+」添加好友 / 创建群聊；群会话可进入群聊天页 |
| Chat | 私聊 / 群聊（同一页面按模式切换） |
| Contacts / AddFriend / FriendRequests | 好友 |
| Profile | 个人信息、退出登录 |

群聊相关页面（CreateGroup、GroupInfo 等）见 `PROJECT_STATUS.md` 第十节；当前已能从消息页进入群聊页，后续继续完善邀请/退群等入口。

---

## 下一步开发

按 [`PROJECT_STATUS.md`](PROJECT_STATUS.md) **第十一节** 分阶段实现群聊：

1. 建群 + 群列表 + 群详情  
2. 群消息收发 + 本地存储  
3. 邀请 / 退群 / 防重复加群  
4. 空群销毁 + 客户端保留聊天记录  

当前进度：阶段 1 已完成基础接入，阶段 2 正在实施中。

---

## 新会话快速上手

打开 `PROJECT_STATUS.md`，从 **第十一节阶段 1** 的 checkbox 清单开始；协议与表结构见该文档第六～九节。
