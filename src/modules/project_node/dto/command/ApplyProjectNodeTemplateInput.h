#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project_node::dto::command {
    // 套用模板时，单个模板节点的日期映射输入
    struct ApplyProjectNodeTemplateItem {
        std::int64_t templateNodeId;
        std::string plannedStartDate;
        std::string plannedEndDate;
    };

    // 基于模板批量生成阶段节点输入
    struct ApplyProjectNodeTemplateInput {
        std::int64_t projectId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
        std::int64_t templateId;
        std::vector<ApplyProjectNodeTemplateItem> nodes;
    };
} // namespace project_tracker::modules::project_node::dto::command
