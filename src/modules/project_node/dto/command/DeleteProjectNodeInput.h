#pragma once

#include <cstdint>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project_node::dto::command {
    // 删除阶段节点输入
    struct DeleteProjectNodeInput {
        std::int64_t projectId;
        std::int64_t nodeId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
    };
} // namespace project_tracker::modules::project_node::dto::command
