#pragma once

/**
 * TCP 帧头格式（与客户端 client/cpp/protocolframe.h 一致）：
 *   [msg_id: 2 字节大端][body_len: 4 字节大端][body: JSON]
 */
enum MSG_FORMAT {
    HEAD_TOTAL_LEN = 6,
    HEAD_ID_LEN = 2,
    HEAD_DATA_LEN = 4
};