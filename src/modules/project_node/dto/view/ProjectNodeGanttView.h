#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "modules/project/domain/ProjectEnums.h"
#include "modules/project_node/domain/ProjectNodeEnums.h"
#include "modules/task/domain/TaskEnums.h"

namespace project_tracker::modules::project_node::dto::view {
    struct ProjectNodeGanttProjectView {
        std::int64_t id;
        std::string name;
        std::int64_t ownerUserId;
        std::string ownerRealName;
        project::domain::ProjectStatus status;
        std::string plannedStartDate;
        std::string plannedEndDate;
        std::optional<std::string> completedAt;
    };

    struct ProjectNodeGanttNodeView {
        std::int64_t id;
        std::string name;
        int sequenceNo;
        domain::ProjectNodeStatus status;
        std::string plannedStartDate;
        std::string plannedEndDate;
        std::optional<std::string> completedAt;
    };

    struct ProjectNodeGanttTaskItemView {
        std::int64_t id;
        std::int64_t nodeId;
        std::string nodeName;
        std::string name;
        std::int64_t responsibleUserId;
        std::string responsibleRealName;
        task::domain::TaskStatus status;
        int progressPercent;
        task::domain::TaskPriority priority;
        std::string plannedStartDate;
        std::string plannedEndDate;
        std::optional<std::string> completedAt;
    };

    struct ProjectNodeGanttView {
        ProjectNodeGanttProjectView project;
        ProjectNodeGanttNodeView node;
        std::vector<ProjectNodeGanttTaskItemView> subtasks;
    };
} // namespace project_tracker::modules::project_node::dto::view
