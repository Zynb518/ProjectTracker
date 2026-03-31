#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <drogon/utils/coroutine.h>

#include "common/db/SqlExecutor.h"
#include "modules/project/dto/ProjectDto.h"
#include "modules/project/domain/ProjectEnums.h"
#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project::repository {
    // 项目列表查询条件
    struct ProjectListQuery {
        std::string keyword;
        std::optional<domain::ProjectStatus> status;
        std::optional<std::int64_t> ownerUserId;
        std::int64_t currentUserId;
        user::domain::SystemRole currentUserRole;
        std::int64_t page = 1;
        std::int64_t pageSize = 10;
    };

    // 项目列表分页结果
    struct ProjectListPage {
        std::vector<dto::view::ProjectListItemView> list;
        std::int64_t total;
        std::int64_t page;
        std::int64_t pageSize;
    };

    // 手动开始项目前的校验信息
    struct ProjectStartCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        domain::ProjectStatus status;
        bool hasNodes;
    };

    // 手动完成项目前的校验信息
    struct ProjectCompleteCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        domain::ProjectStatus status;
        std::int64_t nodeCount;
        std::int64_t completedNodeCount;
    };

    class ProjectRepository {
    public:
        drogon::Task<dto::view::CreatedProjectView>
        insertProject(const common::db::SqlExecutorPtr &executor,
                      const dto::command::CreateProjectInput &input) const;

        drogon::Task<void>
        insertProjectMember(const common::db::SqlExecutorPtr &executor,
                            std::int64_t projectId,
                            std::int64_t userId,
                            std::int64_t addedBy) const;

        // 修改项目基础信息
        drogon::Task<std::optional<dto::view::UpdatedProjectBasicInfoView>>
        updateProjectBasicInfo(const common::db::SqlExecutorPtr &executor,
                               const dto::command::UpdateProjectBasicInfoInput &input) const;

        // 删除项目
        drogon::Task<bool>
        deleteProject(const common::db::SqlExecutorPtr &executor,
                      const dto::command::DeleteProjectInput &input) const;

        // 查询删除项目时的负责人用户 ID
        drogon::Task<std::optional<std::int64_t>>
        findProjectDeleteCheckResult(const common::db::SqlExecutorPtr &executor,
                                     std::int64_t projectId) const;

        // 查询手动开始项目前的校验信息
        drogon::Task<std::optional<ProjectStartCheckResult>>
        findProjectStartCheckResult(const common::db::SqlExecutorPtr &executor,
                                    std::int64_t projectId) const;

        // 按手动开始动作更新项目状态
        drogon::Task<std::optional<dto::view::UpdatedProjectStatusView>>
        updateProjectStatusForStart(const common::db::SqlExecutorPtr &executor,
                                    const dto::command::StartProjectInput &input) const;

        // 查询手动完成项目前的校验信息
        drogon::Task<std::optional<ProjectCompleteCheckResult>>
        findProjectCompleteCheckResult(const common::db::SqlExecutorPtr &executor,
                                       std::int64_t projectId) const;

        // 按手动完成动作更新项目状态
        drogon::Task<std::optional<dto::view::UpdatedProjectStatusView>>
        updateProjectStatusForComplete(const common::db::SqlExecutorPtr &executor,
                                       const dto::command::CompleteProjectInput &input) const;

        // 查询项目分页列表
        drogon::Task<ProjectListPage>
        listProjects(const common::db::SqlExecutorPtr &executor,
                     const ProjectListQuery &query) const;

        // 查询项目详情
        drogon::Task<std::optional<dto::view::ProjectDetailView>>
        findProjectDetail(const common::db::SqlExecutorPtr &executor,
                          std::int64_t projectId,
                          std::int64_t currentUserId,
                          user::domain::SystemRole currentUserRole) const;
    };
} // namespace project_tracker::modules::project::repository
