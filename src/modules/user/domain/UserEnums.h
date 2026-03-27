#pragma once

#include <optional>

namespace project_tracker::modules::user::domain {
    // 系统角色
    // 对齐 sys_user.system_role：
    // 1 = 管理员
    // 2 = 项目经理
    // 3 = 普通员工
    enum class SystemRole : int {
        Admin = 1,
        ProjectManager = 2,
        Employee = 3
    };

    // 用户状态
    // 对齐 sys_user.status：
    // 1 = 启用
    // 2 = 禁用
    enum class UserStatus : int {
        Enabled = 1,
        Disabled = 2
    };

    inline constexpr int toInt(SystemRole role) {
        return static_cast<int>(role);
    }

    inline constexpr int toInt(UserStatus status) {
        return static_cast<int>(status);
    }
} // namespace project_tracker::modules::user::domain
