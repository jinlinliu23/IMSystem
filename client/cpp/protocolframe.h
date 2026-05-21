#pragma once

/**
 * @file protocolframe.h
 * @brief 与服务端 network/networkconst.h 保持一致的 TCP 帧头长度定义
 *
 * 完整一帧 = 帧头(6) + JSON 消息体
 * 组包/拆包实现见 TcpClient（客户端）与 TcpConnection（服务端）
 */

namespace Protocol {

constexpr int HEAD_TOTAL_LEN = 6; ///< 帧头总长度
constexpr int HEAD_ID_LEN = 2;    ///< msg_id 占用字节（大端 uint16）
constexpr int HEAD_DATA_LEN = 4;  ///< body 长度占用字节（大端 uint32）

} // namespace Protocol
