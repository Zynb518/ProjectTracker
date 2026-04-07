#pragma once

#include <cstdint>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::task::dto::command {
    // 子任务状态动作输入
    struct TaskStatusActionInput {
        std::int64_t subTaskId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
    };
} // namespace project_tracker::modules::task::dto::command
