#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "modules/task/domain/TaskEnums.h"
#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::task::dto::command {
    // 创建子任务输入
    struct CreateTaskInput {
        std::int64_t projectId;
        std::int64_t nodeId;
        std::int64_t creatorUserId;
        user::domain::SystemRole creatorUserRole;
        std::string name;
        std::string description;
        std::optional<std::int64_t> responsibleUserId;
        std::optional<domain::TaskPriority> priority;
        std::string plannedStartDate;
        std::string plannedEndDate;
    };
} // namespace project_tracker::modules::task::dto::command
