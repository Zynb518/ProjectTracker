#pragma once

#include <cstdint>
#include <optional>
#include "modules/user/domain/UserEnums.h"

namespace project_tracker::filters {

    // 登录验证逻辑
    inline bool isLoginAllowed(const std::optional<std::int64_t> &userId) {
        return userId.has_value() && *userId > 0;
    }

    // 管理员验证结果类型
    enum class AdminCheckResult {
        Success,
        Unauthorized, // 未登录或登录态失效
        Forbidden     // 无权限访问
    };

    // 管理员验证逻辑
    inline AdminCheckResult checkAdmin(const std::optional<std::int64_t> &userId,
                                       const std::optional<modules::user::domain::SystemRole> &systemRole) {
        if (!userId || *userId <= 0) {
            return AdminCheckResult::Unauthorized;
        }
        if (!systemRole) {
            return AdminCheckResult::Unauthorized;
        }
        if (*systemRole != modules::user::domain::SystemRole::Admin) {
            return AdminCheckResult::Forbidden;
        }
        return AdminCheckResult::Success;
    }

} // namespace project_tracker::filters
