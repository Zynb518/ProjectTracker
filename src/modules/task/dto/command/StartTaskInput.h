#pragma once

#include <cstdint>
#include <string>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::task::dto::command {
    // 手动开始子任务输入
    struct StartTaskInput {
        std::int64_t subTaskId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
        std::string progressNote = "开始处理该任务";
    };
} // namespace project_tracker::modules::task::dto::command
