#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "modules/task/domain/TaskEnums.h"
#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::task::dto::command {
    // 修改子任务基础信息输入
    struct UpdateTaskBasicInfoInput {
        std::int64_t subTaskId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
        std::optional<std::string> name;
        std::optional<std::string> description;
        std::optional<std::int64_t> responsibleUserId;
        std::optional<domain::TaskPriority> priority;
        std::optional<std::string> plannedStartDate;
        std::optional<std::string> plannedEndDate;
    };
} // namespace project_tracker::modules::task::dto::command
