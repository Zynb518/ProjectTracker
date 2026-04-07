#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "modules/task/domain/TaskEnums.h"

namespace project_tracker::modules::task::dto::view {
    // 当前用户负责的子任务列表项展示对象
    struct MyTaskListItemView {
        std::int64_t id;
        std::int64_t projectId;
        std::string projectName;
        std::int64_t nodeId;
        std::string nodeName;
        std::string name;
        domain::TaskStatus status;
        int progressPercent;
        domain::TaskPriority priority;
        std::string plannedStartDate;
        std::string plannedEndDate;
        std::optional<std::string> completedAt;
        std::string updatedAt;
    };
} // namespace project_tracker::modules::task::dto::view
