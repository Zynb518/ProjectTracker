#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "modules/task/domain/TaskEnums.h"

namespace project_tracker::modules::task::dto::view {
    // 修改子任务状态成功后的展示对象
    struct UpdatedTaskStatusView {
        std::int64_t id;
        domain::TaskStatus status;
        int progressPercent;
        std::optional<std::string> completedAt;
        std::string updatedAt;
    };
} // namespace project_tracker::modules::task::dto::view
