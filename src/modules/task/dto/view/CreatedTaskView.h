#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "modules/task/domain/TaskEnums.h"

namespace project_tracker::modules::task::dto::view {
    // 创建子任务成功后的展示对象
    struct CreatedTaskView {
        std::int64_t id;
        std::int64_t nodeId;
        std::string name;
        std::string description;
        std::int64_t responsibleUserId;
        domain::TaskStatus status;
        int progressPercent;
        domain::TaskPriority priority;
        std::string plannedStartDate;
        std::string plannedEndDate;
        std::optional<std::string> completedAt;
        std::int64_t createdBy;
        std::string createdAt;
        std::string updatedAt;
    };
} // namespace project_tracker::modules::task::dto::view
