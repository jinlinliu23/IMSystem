#pragma once

/**
 * @file protocolframe.h
 * @brief 与服务端 network/networkconst.h 保持一致的 TCP 帧头长度定义
 */

namespace Protocol {

constexpr int HEAD_TOTAL_LEN = 6;
constexpr int HEAD_ID_LEN = 2;
constexpr int HEAD_DATA_LEN = 4;

} // namespace Protocol
