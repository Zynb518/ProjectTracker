#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "modules/project_node/domain/ProjectNodeEnums.h"

namespace project_tracker::modules::project_node::dto::view {
    // 修改阶段节点状态成功后的展示对象
    struct UpdatedProjectNodeStatusView {
        std::int64_t id;
        domain::ProjectNodeStatus status;
        std::optional<std::string> completedAt;
        std::string updatedAt;
    };
} // namespace project_tracker::modules::project_node::dto::view
