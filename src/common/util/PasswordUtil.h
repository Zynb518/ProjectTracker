#pragma once

#include <string>

#include <argon2.h>

#include "common/error/Throw.h"

namespace project_tracker::common::util {
    namespace error = common::error;

    // 校验明文密码和 Argon2 编码哈希是否匹配
    // 说明：
    // 1. 返回 true 表示匹配成功
    // 2. 返回 false 表示密码不匹配
    // 3. 如果哈希格式非法或底层校验异常，则抛出内部错误
    inline bool verifyPassword(const std::string &password,
                               const std::string &passwordHash) {
        const int result = argon2_verify(
            passwordHash.c_str(),
            password.c_str(),
            password.size(),
            Argon2_id);

        if (result == ARGON2_OK) {
            return true;
        }

        if (result == ARGON2_VERIFY_MISMATCH) {
            return false;
        }

        error::throwInternalError(
            error::ErrorCode::InternalError,
            "密码哈希校验失败");
    }
} // namespace project_tracker::common::util
