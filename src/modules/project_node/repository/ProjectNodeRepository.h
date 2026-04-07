#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <drogon/utils/coroutine.h>

#include "common/db/SqlExecutor.h"
#include "modules/project/domain/ProjectEnums.h"
#include "modules/project_node/dto/ProjectNodeDto.h"
#include "modules/project_template/domain/ProjectTemplateEnums.h"
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

    // 手动开始阶段节点前的校验信息
    struct ProjectNodeStartCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        project::domain::ProjectStatus projectStatus;
        domain::ProjectNodeStatus nodeStatus;
    };

    // 手动完成阶段节点前的校验信息
    struct ProjectNodeCompleteCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        project::domain::ProjectStatus projectStatus;
        domain::ProjectNodeStatus nodeStatus;
    };

    // 撤销阶段节点完成前的校验信息
    struct ProjectNodeReopenCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        project::domain::ProjectStatus projectStatus;
        domain::ProjectNodeStatus nodeStatus;
    };

    // 删除阶段节点前的校验信息
    struct ProjectNodeDeleteCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        project::domain::ProjectStatus projectStatus;
        domain::ProjectNodeStatus nodeStatus;
        int sequenceNo;
    };

    // 调整阶段节点顺序前的校验信息
    struct ProjectNodeReorderCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        project::domain::ProjectStatus projectStatus;
        std::int64_t totalNodeCount;
        std::int64_t matchedNodeCount;
        std::int64_t completedNodeCount;
    };

    // 创建阶段节点前的项目级校验信息
    struct ProjectNodeCreateCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        project::domain::ProjectStatus projectStatus;
        std::string projectPlannedStartDate;
        std::string projectPlannedEndDate;
    };

    // 基于模板批量生成阶段节点前的校验信息
    struct ProjectNodeApplyTemplateCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        project::domain::ProjectStatus projectStatus;
        std::string projectPlannedStartDate;
        std::string projectPlannedEndDate;
        std::int64_t projectNodeCount;
        std::optional<project_template::domain::ProjectTemplateStatus> templateStatus;
        std::int64_t inputNodeCount;
        std::int64_t distinctInputNodeCount;
        std::int64_t templateNodeCount;
        std::int64_t matchedTemplateNodeCount;
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

        // 锁定项目行并汇总模板映射对账信息，供套用模板写事务做前置检查
        drogon::Task<std::optional<ProjectNodeApplyTemplateCheckResult>>
        findProjectNodeApplyTemplateCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            const dto::command::ApplyProjectNodeTemplateInput &input) const;

        // 基于模板批量生成阶段节点
        drogon::Task<std::optional<dto::view::AppliedProjectNodeTemplateView>>
        insertProjectNodesFromTemplate(
            const common::db::SqlExecutorPtr &executor,
            const dto::command::ApplyProjectNodeTemplateInput &input) const;

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

        // 锁定阶段节点行，供手动开始阶段节点写事务做前置检查
        drogon::Task<std::optional<ProjectNodeStartCheckResult>>
        findProjectNodeStartCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            std::int64_t projectId,
            std::int64_t nodeId) const;

        // 按手动开始动作更新阶段节点状态
        drogon::Task<std::optional<dto::view::UpdatedProjectNodeStatusView>>
        updateProjectNodeStatusForStart(const common::db::SqlExecutorPtr &executor,
                                        std::int64_t projectId,
                                        std::int64_t nodeId) const;

        // 锁定阶段节点行，供手动完成阶段节点写事务做前置检查
        drogon::Task<std::optional<ProjectNodeCompleteCheckResult>>
        findProjectNodeCompleteCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            std::int64_t projectId,
            std::int64_t nodeId) const;

        // 按手动完成动作更新阶段节点状态
        drogon::Task<std::optional<dto::view::UpdatedProjectNodeStatusView>>
        updateProjectNodeStatusForComplete(const common::db::SqlExecutorPtr &executor,
                                           std::int64_t projectId,
                                           std::int64_t nodeId) const;

        // 锁定阶段节点行，供撤销阶段节点完成写事务做前置检查
        drogon::Task<std::optional<ProjectNodeReopenCheckResult>>
        findProjectNodeReopenCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            std::int64_t projectId,
            std::int64_t nodeId) const;

        // 按撤销完成动作更新阶段节点状态
        drogon::Task<std::optional<dto::view::UpdatedProjectNodeStatusView>>
        updateProjectNodeStatusForReopen(const common::db::SqlExecutorPtr &executor,
                                         std::int64_t projectId,
                                         std::int64_t nodeId) const;

        // 锁定阶段节点行，供删除阶段节点写事务做前置检查
        drogon::Task<std::optional<ProjectNodeDeleteCheckResult>>
        findProjectNodeDeleteCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            std::int64_t projectId,
            std::int64_t nodeId) const;

        // 删除阶段节点并补齐后续顺序号
        drogon::Task<bool>
        deleteProjectNode(const common::db::SqlExecutorPtr &executor,
                          const dto::command::DeleteProjectNodeInput &input) const;

        // 锁定输入中的阶段节点，供调整顺序写事务做前置检查
        drogon::Task<std::optional<ProjectNodeReorderCheckResult>>
        findProjectNodeReorderCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            const dto::command::ReorderProjectNodesInput &input) const;

        // 调整阶段节点顺序
        drogon::Task<std::optional<dto::view::ReorderedProjectNodesView>>
        reorderProjectNodes(const common::db::SqlExecutorPtr &executor,
                            const dto::command::ReorderProjectNodesInput &input) const;
    };
} // namespace project_tracker::modules::project_node::repository
