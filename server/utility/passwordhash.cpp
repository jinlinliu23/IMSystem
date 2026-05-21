#include "passwordhash.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace PasswordHash {

std::string hashPassword(const std::string &password, const std::string &salt)
{
    const std::string input = salt + password;

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLen = 0;

    if (EVP_Digest(input.data(), input.size(), digest, &digestLen, EVP_sha256(), nullptr) != 1) {
        throw std::runtime_error("EVP_Digest failed");
    }

    std::ostringstream oss;
    for (unsigned int i = 0; i < digestLen; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return oss.str();
}

std::string generateSalt()
{
    unsigned char buf[16];
    if (RAND_bytes(buf, sizeof(buf)) != 1) {
        throw std::runtime_error("RAND_bytes failed");
    }

    std::ostringstream oss;
    for (unsigned char b : buf) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return oss.str();
}

} // namespace PasswordHash
