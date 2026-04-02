#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <drogon/utils/coroutine.h>

#include "common/db/SqlExecutor.h"
#include "modules/project/domain/ProjectEnums.h"
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

    // 修改阶段节点基础信息前的校验信息
    struct ProjectNodeBasicInfoUpdateCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        project::domain::ProjectStatus projectStatus;
        domain::ProjectNodeStatus nodeStatus;
    };

    // 创建阶段节点前的项目级校验信息
    struct ProjectNodeCreateCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        project::domain::ProjectStatus projectStatus;
        std::string projectPlannedStartDate;
        std::string projectPlannedEndDate;
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

        // 锁定项目行，供修改阶段节点基础信息写事务区分项目是否存在
        drogon::Task<std::optional<std::int64_t>>
        findProjectIdForUpdate(const common::db::SqlExecutorPtr &executor,
                               std::int64_t projectId) const;

        // 锁定项目行，供创建阶段节点写事务做项目级前置检查
        drogon::Task<std::optional<ProjectNodeCreateCheckResult>>
        findProjectNodeCreateCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            std::int64_t projectId) const;

        // 创建阶段节点
        drogon::Task<dto::view::CreatedProjectNodeView>
        insertProjectNode(const common::db::SqlExecutorPtr &executor,
                          const dto::command::CreateProjectNodeInput &input) const;

        // 锁定阶段节点行，供修改阶段节点基础信息写事务做前置检查
        drogon::Task<std::optional<ProjectNodeBasicInfoUpdateCheckResult>>
        findProjectNodeBasicInfoUpdateCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            std::int64_t projectId,
            std::int64_t nodeId) const;

        // 修改阶段节点基础信息
        drogon::Task<std::optional<dto::view::UpdatedProjectNodeBasicInfoView>>
        updateProjectNodeBasicInfo(const common::db::SqlExecutorPtr &executor,
                                   const dto::command::UpdateProjectNodeBasicInfoInput &input) const;
    };
} // namespace project_tracker::modules::project_node::repository
