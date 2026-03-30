#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "modules/project/domain/ProjectEnums.h"

namespace project_tracker::modules::project::dto::view {
    // 项目列表项展示对象
    struct ProjectListItemView {
        std::int64_t id;
        std::string name;
        std::string description;
        std::int64_t ownerUserId;
        std::string ownerRealName;
        domain::ProjectStatus status;
        std::string plannedStartDate;
        std::string plannedEndDate;
        std::optional<std::string> completedAt;
        std::int64_t memberCount;
        std::int64_t nodeCount;
        std::int64_t subTaskCount;
        std::string createdAt;
        std::string updatedAt;
        bool isOwner;
    };
} // namespace project_tracker::modules::project::dto::view
