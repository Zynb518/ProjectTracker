#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "modules/project_node/domain/ProjectNodeEnums.h"

namespace project_tracker::modules::project_node::dto::view {
    // 创建阶段节点成功后的展示对象
    struct CreatedProjectNodeView {
        std::int64_t id;
        std::int64_t projectId;
        std::string name;
        std::string description;
        int sequenceNo;
        domain::ProjectNodeStatus status;
        std::string plannedStartDate;
        std::string plannedEndDate;
        std::optional<std::string> completedAt;
        std::int64_t createdBy;
        std::string createdAt;
        std::string updatedAt;
    };
} // namespace project_tracker::modules::project_node::dto::view
