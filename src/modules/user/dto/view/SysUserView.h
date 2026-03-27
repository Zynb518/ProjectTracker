#pragma once

#include <cstdint>
#include <string>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::user::dto::view {
    // 系统用户展示对象
    // 说明：
    // 1. 字段对齐 sys_user 的业务可见部分
    // 2. 不包含 password / password_hash，这类字段不应该在业务返回对象中传播
    struct SysUserView {
        std::int64_t id;
        std::string username;
        std::string realName;
        domain::SystemRole systemRole;
        std::string email;
        std::string phone;
        domain::UserStatus status;
        std::string createdAt;
        std::string updatedAt;
    };
} // namespace project_tracker::modules::user::dto::view
