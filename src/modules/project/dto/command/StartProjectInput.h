#pragma once

#include <cstdint>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project::dto::command {
    // 手动开始项目输入
    struct StartProjectInput {
        std::int64_t projectId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
    };
} // namespace project_tracker::modules::project::dto::command
