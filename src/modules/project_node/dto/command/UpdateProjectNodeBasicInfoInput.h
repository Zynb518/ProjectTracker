#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project_node::dto::command {
    // 修改阶段节点基础信息输入
    struct UpdateProjectNodeBasicInfoInput {
        std::int64_t projectId;
        std::int64_t nodeId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
        std::optional<std::string> name;
        std::optional<std::string> description;
        std::optional<std::string> plannedStartDate;
        std::optional<std::string> plannedEndDate;
    };
} // namespace project_tracker::modules::project_node::dto::command
