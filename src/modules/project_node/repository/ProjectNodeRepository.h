#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <drogon/utils/coroutine.h>

#include "common/db/SqlExecutor.h"
#include "modules/project_node/dto/ProjectNodeDto.h"
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

    // 阶段节点详情查询条件
    struct ProjectNodeDetailQuery {
        std::int64_t projectId;
        std::int64_t nodeId;
        std::int64_t currentUserId;
        user::domain::SystemRole currentUserRole;
    };

    // 阶段节点详情查询结果
    struct ProjectNodeDetailResult {
        bool hasPermission;
        std::optional<dto::view::ProjectNodeDetailView> detail;
    };

    class ProjectNodeRepository {
    public:
        // 查询项目阶段节点列表
        drogon::Task<std::optional<ProjectNodeListResult>>
        listProjectNodes(const common::db::SqlExecutorPtr &executor,
                         const ProjectNodeListQuery &query) const;

        // 查询阶段节点详情
        drogon::Task<std::optional<ProjectNodeDetailResult>>
        findProjectNodeDetail(const common::db::SqlExecutorPtr &executor,
                              const ProjectNodeDetailQuery &query) const;
    };
} // namespace project_tracker::modules::project_node::repository
