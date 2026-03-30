#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "modules/project/domain/ProjectEnums.h"

namespace project_tracker::modules::project::dto::view {
    // 创建项目成功后的展示对象
    struct CreatedProjectView {
        std::int64_t id;
        std::string name;
        std::string description;
        std::int64_t ownerUserId;
        domain::ProjectStatus status;
        std::string plannedStartDate;
        std::string plannedEndDate;
        std::optional<std::string> completedAt;
        std::int64_t createdBy;
        std::string createdAt;
        std::string updatedAt;
    };
} // namespace project_tracker::modules::project::dto::view
