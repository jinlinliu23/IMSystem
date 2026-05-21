#pragma once

#include <string>

/**
 * @brief 密码安全存储：随机盐 + SHA256(salt + password) 十六进制字符串
 *
 * 注册时：generateSalt() → hashPassword() → 写入 users 表的 salt、password_hash 列
 * 登录时（待实现）：用同一算法校验客户端提交的密码
 */
namespace PasswordHash {

std::string hashPassword(const std::string &password, const std::string &salt);
std::string generateSalt();

} // namespace PasswordHash
