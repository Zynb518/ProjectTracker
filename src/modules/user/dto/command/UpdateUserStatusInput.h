#pragma once

#include <cstdint>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::user::dto::command {
    // 修改用户状态输入
    struct UpdateUserStatusInput {
        std::int64_t userId;
        domain::UserStatus status;
    };
} // namespace project_tracker::modules::user::dto::command
