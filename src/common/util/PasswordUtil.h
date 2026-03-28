#pragma once

#include <array>
#include <optional>
#include <string>

#include <argon2.h>
#include <drogon/utils/Utilities.h>

namespace project_tracker::common::util {
    enum class PasswordVerifyResult {
        Matched,
        Mismatched,
        Failed
    };

    // 生成 Argon2 编码密码哈希
    inline std::optional<std::string> hashPassword(const std::string &password) {
        constexpr std::uint32_t timeCost = 2;
        constexpr std::uint32_t memoryCost = 65536;
        constexpr std::uint32_t parallelism = 1;
        constexpr std::size_t saltLength = 16;
        constexpr std::size_t hashLength = 32;

        std::array<unsigned char, saltLength> salt{};
        if (!drogon::utils::secureRandomBytes(salt.data(), salt.size())) {
            return std::nullopt;
        }

        const auto encodedLength = argon2_encodedlen(
            timeCost,
            memoryCost,
            parallelism,
            saltLength,
            hashLength,
            Argon2_id);

        std::string encodedHash(encodedLength, '\0');
        const int result = argon2_hash(
            timeCost,
            memoryCost,
            parallelism,
            password.data(),
            password.size(),
            salt.data(),
            salt.size(),
            nullptr,
            hashLength,
            encodedHash.data(),
            encodedHash.size(),
            Argon2_id,
            ARGON2_VERSION_13);

        if (result != ARGON2_OK) {
            return std::nullopt;
        }

        if (!encodedHash.empty() && encodedHash.back() == '\0') {
            encodedHash.pop_back();
        }

        return encodedHash;
    }

    // 校验明文密码和 Argon2 编码哈希是否匹配
    // 说明：
    // 1. Matched 表示匹配成功
    // 2. Mismatched 表示密码不匹配
    // 3. Failed 表示哈希格式非法或底层校验失败
    inline PasswordVerifyResult verifyPassword(const std::string &password,
                                               const std::string &passwordHash) {
        const int result = argon2_verify(
            passwordHash.c_str(),
            password.c_str(),
            password.size(),
            Argon2_id);

        if (result == ARGON2_OK) {
            return PasswordVerifyResult::Matched;
        }

        if (result == ARGON2_VERIFY_MISMATCH) {
            return PasswordVerifyResult::Mismatched;
        }

        return PasswordVerifyResult::Failed;
    }
} // namespace project_tracker::common::util
