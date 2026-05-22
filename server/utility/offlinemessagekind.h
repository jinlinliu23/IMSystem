#pragma once

#include <cstdint>

/** 离线消息业务类型（写入 offline_messages.kind） */
enum class OfflineMessageKind : int {
    PrivateChat = 1,
    GroupChat = 2,
    FriendNotify = 3,
    GroupNotify = 4,
};
