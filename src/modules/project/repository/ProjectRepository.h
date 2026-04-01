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

    // 项目负责人转交候选列表查询条件
    struct ProjectOwnerCandidateQuery {
        std::int64_t projectId;
        std::int64_t operatorUserId;
        bool includeAdminCandidates;
        std::string keyword;
        std::int64_t page = 1;
        std::int64_t pageSize = 10;
    };

    // 项目负责人转交候选列表分页结果
    struct ProjectOwnerCandidatePage {
        std::vector<dto::view::ProjectOwnerCandidateView> list;
        std::int64_t total;
        std::int64_t page;
        std::int64_t pageSize;
    };

    // 项目负责人转交候选列表单查询结果
    struct ProjectOwnerCandidateListResult {
        bool projectExists;
        bool hasPermission;
        bool ownerTransferAllowed;
        ProjectOwnerCandidatePage page;
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

    // 撤销项目完成前的校验信息
    struct ProjectReopenCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        domain::ProjectStatus status;
    };

    // 修改项目基础信息前的校验信息
    struct ProjectBasicInfoUpdateCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        domain::ProjectStatus status;
    };

    // 转交项目负责人前的项目级校验信息
    struct ProjectOwnerTransferProjectCheckResult {
        std::int64_t previousOwnerUserId;
        user::domain::SystemRole creatorUserRole;
    };

    // 转交项目负责人前的目标用户校验信息
    struct TransferTargetUserCheckResult {
        user::domain::SystemRole targetUserRole;
        user::domain::UserStatus targetUserStatus;
        bool targetIsProjectMember;
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

        drogon::Task<void>
        ensureProjectMemberExists(const common::db::SqlExecutorPtr &executor,
                                  std::int64_t projectId,
                                  std::int64_t userId,
                                  std::int64_t addedBy) const;

        // 修改项目基础信息
        drogon::Task<std::optional<dto::view::UpdatedProjectBasicInfoView>>
        updateProjectBasicInfo(const common::db::SqlExecutorPtr &executor,
                               const dto::command::UpdateProjectBasicInfoInput &input) const;

        // 锁定项目行，供修改项目基础信息写事务做前置检查
        drogon::Task<std::optional<ProjectBasicInfoUpdateCheckResult>>
        findProjectBasicInfoUpdateCheckResultForUpdate(const common::db::SqlExecutorPtr &executor,
                                                       std::int64_t projectId) const;

        // 删除项目
        drogon::Task<bool>
        deleteProject(const common::db::SqlExecutorPtr &executor,
                      std::int64_t projectId) const;

        // 锁定项目行，供删除项目写事务做前置检查
        drogon::Task<std::optional<std::int64_t>>
        findProjectDeleteCheckResultForUpdate(const common::db::SqlExecutorPtr &executor,
                                              std::int64_t projectId) const;

        // 锁定项目行，供手动开始项目写事务做前置检查
        drogon::Task<std::optional<ProjectStartCheckResult>>
        findProjectStartCheckResultForUpdate(const common::db::SqlExecutorPtr &executor,
                                             std::int64_t projectId) const;

        // 按手动开始动作更新项目状态
        drogon::Task<std::optional<dto::view::UpdatedProjectStatusView>>
        updateProjectStatusForStart(const common::db::SqlExecutorPtr &executor,
                                    std::int64_t projectId) const;

        // 锁定项目行，供手动完成项目写事务做前置检查
        drogon::Task<std::optional<ProjectCompleteCheckResult>>
        findProjectCompleteCheckResultForUpdate(const common::db::SqlExecutorPtr &executor,
                                                std::int64_t projectId) const;

        // 按手动完成动作更新项目状态
        drogon::Task<std::optional<dto::view::UpdatedProjectStatusView>>
        updateProjectStatusForComplete(const common::db::SqlExecutorPtr &executor,
                                       std::int64_t projectId) const;

        // 锁定项目行，供撤销项目完成写事务做前置检查
        drogon::Task<std::optional<ProjectReopenCheckResult>>
        findProjectReopenCheckResultForUpdate(const common::db::SqlExecutorPtr &executor,
                                              std::int64_t projectId) const;

        // 锁定项目行，供转交项目负责人写事务做前置检查
        drogon::Task<std::optional<ProjectOwnerTransferProjectCheckResult>>
        findProjectOwnerTransferProjectCheckResultForUpdate(const common::db::SqlExecutorPtr &executor,
                                                            std::int64_t projectId) const;

        // 锁定目标用户行，供转交项目负责人写事务做前置检查
        drogon::Task<std::optional<TransferTargetUserCheckResult>>
        findTransferTargetUserCheckResultForUpdate(const common::db::SqlExecutorPtr &executor,
                                                   std::int64_t projectId,
                                                   std::int64_t targetUserId) const;

        // 按撤销完成动作更新项目状态
        drogon::Task<std::optional<dto::view::UpdatedProjectStatusView>>
        updateProjectStatusForReopen(const common::db::SqlExecutorPtr &executor,
                                     std::int64_t projectId) const;

        // 更新项目负责人
        drogon::Task<std::optional<dto::view::TransferredProjectOwnerView>>
        updateProjectOwner(const common::db::SqlExecutorPtr &executor,
                           const dto::command::TransferProjectOwnerInput &input) const;

        // 查询项目分页列表
        drogon::Task<ProjectListPage>
        listProjects(const common::db::SqlExecutorPtr &executor,
                     const ProjectListQuery &query) const;

        // 查询项目负责人转交候选列表
        drogon::Task<ProjectOwnerCandidateListResult>
        listProjectOwnerCandidates(const common::db::SqlExecutorPtr &executor,
                                   const ProjectOwnerCandidateQuery &query) const;

        // 查询项目详情
        drogon::Task<std::optional<dto::view::ProjectDetailView>>
        findProjectDetail(const common::db::SqlExecutorPtr &executor,
                          std::int64_t projectId,
                          std::int64_t currentUserId,
                          user::domain::SystemRole currentUserRole) const;
    };
} // namespace project_tracker::modules::project::repository
