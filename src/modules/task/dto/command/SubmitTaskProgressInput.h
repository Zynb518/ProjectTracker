#pragma once

#include <cstdint>
#include <string>

#include "modules/task/domain/TaskEnums.h"
#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::task::dto::command {
    // 提交子任务进度输入
    struct SubmitTaskProgressInput {
        std::int64_t subTaskId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
        domain::TaskStatus status;
        int progressPercent;
        std::string progressNote;
    };
} // namespace project_tracker::modules::task::dto::command
