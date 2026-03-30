#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "modules/project/domain/ProjectEnums.h"

namespace project_tracker::modules::project::dto::view {
    // 项目详情权限展示对象
    struct ProjectDetailPermissionsView {
        bool canEditBasic;
        bool canManageMembers;
        bool canManageNodes;
        bool canTransferOwner;
        bool canDelete;
    };

    // 项目详情展示对象
    struct ProjectDetailView {
        std::int64_t id;
        std::string name;
        std::string description;
        std::int64_t ownerUserId;
        std::string ownerRealName;
        domain::ProjectStatus status;
        std::string plannedStartDate;
        std::string plannedEndDate;
        std::optional<std::string> completedAt;
        std::int64_t createdBy;
        std::string createdByRealName;
        std::string createdAt;
        std::string updatedAt;
        std::int64_t memberCount;
        std::int64_t nodeCount;
        std::int64_t completedNodeCount;
        std::int64_t subTaskCount;
        std::int64_t completedSubTaskCount;
        ProjectDetailPermissionsView permissions;
    };
} // namespace project_tracker::modules::project::dto::view
