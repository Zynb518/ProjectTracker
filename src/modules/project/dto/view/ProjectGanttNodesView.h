#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "modules/project/domain/ProjectEnums.h"
#include "modules/project_node/domain/ProjectNodeEnums.h"

namespace project_tracker::modules::project::dto::view {
    struct ProjectGanttProjectView {
        std::int64_t id;
        std::string name;
        std::int64_t ownerUserId;
        std::string ownerRealName;
        domain::ProjectStatus status;
        std::string plannedStartDate;
        std::string plannedEndDate;
        std::optional<std::string> completedAt;
    };

    struct ProjectGanttNodeItemView {
        std::int64_t id;
        std::string name;
        int sequenceNo;
        project_node::domain::ProjectNodeStatus status;
        std::string plannedStartDate;
        std::string plannedEndDate;
        std::optional<std::string> completedAt;
    };

    struct ProjectGanttNodesView {
        ProjectGanttProjectView project;
        std::vector<ProjectGanttNodeItemView> nodes;
    };
} // namespace project_tracker::modules::project::dto::view
