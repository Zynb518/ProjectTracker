#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "modules/project/dto/view/ProjectGanttNodesView.h"
#include "modules/task/domain/TaskEnums.h"

namespace project_tracker::modules::project::dto::view {
    struct ProjectGanttMemberTaskItemView {
        std::int64_t subtaskId;
        std::string name;
        std::int64_t nodeId;
        std::string nodeName;
        task::domain::TaskStatus status;
        int progressPercent;
        std::string plannedStartDate;
        std::string plannedEndDate;
        std::optional<std::string> completedAt;
    };

    struct ProjectGanttMemberRowView {
        std::int64_t userId;
        std::string realName;
        std::vector<ProjectGanttMemberTaskItemView> subtasks;
    };

    struct ProjectGanttMembersView {
        ProjectGanttProjectView project;
        std::vector<ProjectGanttMemberRowView> memberRows;
    };
} // namespace project_tracker::modules::project::dto::view
