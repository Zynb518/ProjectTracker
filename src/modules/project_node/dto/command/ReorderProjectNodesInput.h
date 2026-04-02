#pragma once

#include <cstdint>
#include <vector>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project_node::dto::command {
    // 调整阶段节点顺序时，单个节点的新顺序输入
    struct ReorderProjectNodeItem {
        std::int64_t nodeId;
        int sequenceNo;
    };

    // 调整阶段节点顺序输入
    struct ReorderProjectNodesInput {
        std::int64_t projectId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
        std::vector<ReorderProjectNodeItem> nodes;
    };
} // namespace project_tracker::modules::project_node::dto::command
