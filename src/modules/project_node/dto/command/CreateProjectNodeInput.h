#pragma once

#include <cstdint>
#include <string>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project_node::dto::command {
    // 创建阶段节点输入
    struct CreateProjectNodeInput {
        std::int64_t projectId;
        std::int64_t creatorUserId;
        user::domain::SystemRole creatorUserRole;
        std::string name;
        std::string description;
        std::string plannedStartDate;
        std::string plannedEndDate;
    };
} // namespace project_tracker::modules::project_node::dto::command
