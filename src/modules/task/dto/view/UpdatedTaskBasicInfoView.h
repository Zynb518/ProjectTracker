#pragma once

#include <cstdint>
#include <string>

#include "modules/task/domain/TaskEnums.h"

namespace project_tracker::modules::task::dto::view {
    // 修改子任务基础信息成功后的展示对象
    struct UpdatedTaskBasicInfoView {
        std::int64_t id;
        std::string name;
        std::string description;
        std::int64_t responsibleUserId;
        domain::TaskPriority priority;
        std::string plannedStartDate;
        std::string plannedEndDate;
        std::string updatedAt;
    };
} // namespace project_tracker::modules::task::dto::view
