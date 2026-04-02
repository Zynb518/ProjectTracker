#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <drogon/utils/coroutine.h>

#include "common/db/SqlExecutor.h"
#include "modules/project_node/dto/view/ProjectNodeListItemView.h"
#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project_node::repository {
    // 阶段节点列表查询条件
    struct ProjectNodeListQuery {
        std::int64_t projectId;
        std::int64_t currentUserId;
        user::domain::SystemRole currentUserRole;
    };

    // 阶段节点列表查询结果
    struct ProjectNodeListResult {
        bool hasPermission;
        std::vector<dto::view::ProjectNodeListItemView> list;
    };

    class ProjectNodeRepository {
    public:
        // 查询项目阶段节点列表
        drogon::Task<std::optional<ProjectNodeListResult>>
        listProjectNodes(const common::db::SqlExecutorPtr &executor,
                         const ProjectNodeListQuery &query) const;
    };
} // namespace project_tracker::modules::project_node::repository
