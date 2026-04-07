#pragma once

#include <cstdint>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::task::dto::command {
    // 删除子任务输入
    struct DeleteTaskInput {
        std::int64_t subTaskId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
    };
} // namespace project_tracker::modules::task::dto::command
