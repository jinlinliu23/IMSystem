# IMSystem

即时通讯课程项目：注册、登录、单聊、群聊。

## 数据库说明

- **类型**：服务端内嵌 **SQLite**（不是单独安装的 MySQL/PostgreSQL）
- **文件**：`server/data/im.db`（首次运行 `server` 时自动创建目录和表）
- **表**：`users`（id, **username=账号**（唯一）, **nickname=昵称**, password_hash, salt, created_at）
- **查看数据**：`sqlite3 server/data/im.db "SELECT id, username, nickname FROM users;"`

客户端 `QSettings` 只保存服务器 IP/端口，**不存用户数据库**。

## 消息协议（当前仅注册）

**传输**：TCP 端口 `16701`，每帧 = 6 字节头 + JSON 体。

| 字段 | 长度 | 说明 |
|------|------|------|
| msg_id | 2 | 大端 uint16，见 `common/msg_ids.h` |
| body_len | 4 | 大端 uint32 |
| body | N | UTF-8 JSON |

**注册请求** `msg_id = 1100`：

```json
{"username":"alice","password":"secret123","nickname":"爱丽丝"}
```
（`username` 即用户填写的**账号**，全站唯一且**区分大小写**；`nickname` 为**昵称**展示名）

**注册响应** `msg_id = 1101`：

```json
{"code":0,"msg":"注册成功","user_id":1}
```

`code`：0 成功，1 账号已被注册，2 参数错误，3 数据库错误，4 账号或密码错误。

**登录请求** `msg_id = 1200`：

```json
{"username":"alice","password":"secret123"}
```

**登录响应** `msg_id = 1201`：

```json
{"code":0,"msg":"登录成功","user_id":1,"username":"alice","nickname":"爱丽丝"}
```

登录成功后客户端将 `user_id` / `username` / `nickname` 写入 `QSettings`，下次启动直达主页。

单聊/群聊消息 ID 尚未实现，可在 `common/msg_ids.h` 中扩展。

## 服务端

依赖：jsoncpp、sqlite3、openssl（pkg-config）。

```bash
cd server
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/server
```

- 监听端口：**16701**
- SQLite 数据库：`server/data/im.db`（首次运行自动建表）

### 注册协议

| 方向 | msg_id | JSON |
|------|--------|------|
| 客户端 → 服务端 | 1100 | `{"username","password","nickname"}` |
| 服务端 → 客户端 | 1101 | `{"code":0,"msg":"...","user_id":1}` |

`code`: 0 成功，1 用户名已存在，2 参数错误，3 数据库/内部错误。

## 客户端

```bash
cd client
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/appclient
```

- 桌面默认连接 `127.0.0.1:16701`
- Android 模拟器默认 `10.0.2.2:16701`；真机请改为电脑局域网 IP：

```qml
ClientFacade.serverHost = "192.168.x.x"
```

真机调试需在 Android 清单中允许网络（Qt 工程一般已包含 `INTERNET` 权限）。
