#include "modules/project_node/service/ProjectNodeService.h"

#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::project_node::service {
    namespace error = project_tracker::common::error;
    namespace project_domain = modules::project::domain;
    namespace user_domain = modules::user::domain;

    drogon::Task<dto::view::CreatedProjectNodeView>
    ProjectNodeService::createProjectNode(const dto::command::CreateProjectNodeInput &input) const {
        if (input.plannedStartDate > input.plannedEndDate) {
            error::throwConflict(
                error::ErrorCode::InvalidDateRange,
                "planned_start_date 不能晚于 planned_end_date");
        }

        try {
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();

            const auto projectCheckResult = co_await
                projectNodeRepository_.findProjectNodeCreateCheckResultForUpdate(
                    transaction,
                    input.projectId);
            if (!projectCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            const bool isAdmin = input.creatorUserRole == user_domain::SystemRole::Admin;
            if (projectCheckResult->creatorUserRole == user_domain::SystemRole::Employee && isAdmin) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "普通员工创建的个人自用项目不允许管理员创建阶段节点");
            }

            if (!isAdmin && projectCheckResult->ownerUserId != input.creatorUserId) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目负责人");
            }

            if (projectCheckResult->projectStatus == project_domain::ProjectStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompletedReadonly,
                    "项目已完成，不允许创建阶段节点");
            }

            if (input.plannedStartDate < projectCheckResult->projectPlannedStartDate ||
                input.plannedEndDate > projectCheckResult->projectPlannedEndDate) {
                error::throwConflict(
                    error::ErrorCode::InvalidDateRange,
                    "阶段节点时间区间不合法或不在所属项目时间范围内");
            }

            const auto node = co_await projectNodeRepository_.insertProjectNode(
                transaction,
                input);
            co_return node;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }

    drogon::Task<dto::view::UpdatedProjectNodeBasicInfoView>
    ProjectNodeService::updateProjectNodeBasicInfo(
        const dto::command::UpdateProjectNodeBasicInfoInput &input) const {
        if (!input.name && !input.description &&
            !input.plannedStartDate && !input.plannedEndDate) {
            error::throwBadRequest(
                error::ErrorCode::InvalidParameter,
                "至少需要提供一个可修改字段");
        }

        if (input.plannedStartDate && input.plannedEndDate &&
            *input.plannedStartDate > *input.plannedEndDate) {
            error::throwConflict(
                error::ErrorCode::InvalidDateRange,
                "planned_start_date 不能晚于 planned_end_date");
        }

        try {
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();

            const auto projectId = co_await projectNodeRepository_.findProjectIdForUpdate(
                transaction,
                input.projectId);
            if (!projectId) {
                error::throwNotFound(
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            const auto nodeCheckResult = co_await
                projectNodeRepository_.findProjectNodeBasicInfoUpdateCheckResultForUpdate(
                    transaction,
                    input.projectId,
                    input.nodeId);
            if (!nodeCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::PhaseNotFound,
                    "阶段节点不存在");
            }

            const bool isAdmin = input.operatorUserRole == user_domain::SystemRole::Admin;
            if (nodeCheckResult->creatorUserRole == user_domain::SystemRole::Employee && isAdmin) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "普通员工创建的个人自用项目不允许管理员修改阶段节点");
            }

            if (!isAdmin && nodeCheckResult->ownerUserId != input.operatorUserId) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目负责人");
            }

            if (nodeCheckResult->projectStatus == project_domain::ProjectStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompletedReadonly,
                    "项目已完成，不允许修改阶段节点");
            }

            if (nodeCheckResult->nodeStatus == domain::ProjectNodeStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::PhaseCompletedReadonly,
                    "阶段节点已完成，不允许修改基础信息");
            }

            const auto node = co_await projectNodeRepository_.updateProjectNodeBasicInfo(
                transaction,
                input);
            if (!node) {
                error::throwConflict(
                    error::ErrorCode::InvalidDateRange,
                    "阶段节点时间区间不合法或不能覆盖全部子任务");
            }

            co_return *node;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }

    drogon::Task<dto::view::ReorderedProjectNodesView>
    ProjectNodeService::reorderProjectNodes(
        const dto::command::ReorderProjectNodesInput &input) const {
        if (input.nodes.empty()) {
            error::throwBadRequest(
                error::ErrorCode::InvalidParameter,
                "nodes 至少需要包含一个阶段节点");
        }

        try {
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();

            const auto checkResult = co_await
                projectNodeRepository_.findProjectNodeReorderCheckResultForUpdate(
                    transaction,
                    input);
            if (!checkResult) {
                error::throwNotFound(
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            const bool isAdmin = input.operatorUserRole == user_domain::SystemRole::Admin;
            if (checkResult->creatorUserRole == user_domain::SystemRole::Employee && isAdmin) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "普通员工创建的个人自用项目不允许管理员调整阶段节点顺序");
            }

            if (!isAdmin && checkResult->ownerUserId != input.operatorUserId) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目负责人");
            }

            if (checkResult->projectStatus == project_domain::ProjectStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompletedReadonly,
                    "项目已完成，不允许调整阶段节点顺序");
            }

            const auto inputNodeCount = static_cast<std::int64_t>(input.nodes.size());
            if (checkResult->matchedNodeCount != inputNodeCount) {
                error::throwNotFound(
                    error::ErrorCode::PhaseNotFound,
                    "阶段节点不存在");
            }

            if (checkResult->totalNodeCount != inputNodeCount) {
                error::throwConflict(
                    error::ErrorCode::PhaseSequenceConflict,
                    "提交的阶段节点集合与当前项目节点集合不一致");
            }

            if (checkResult->completedNodeCount > 0) {
                error::throwConflict(
                    error::ErrorCode::CompletedPhaseReorderForbidden,
                    "已完成阶段节点不允许调整顺序");
            }

            const auto result = co_await projectNodeRepository_.reorderProjectNodes(
                transaction,
                input);
            if (!result) {
                error::throwConflict(
                    error::ErrorCode::PhaseSequenceConflict,
                    "调整阶段节点顺序失败");
            }

            co_return *result;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }

    drogon::Task<std::int64_t>
    ProjectNodeService::deleteProjectNode(const dto::command::DeleteProjectNodeInput &input) const {
        try {
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();

            const auto projectId = co_await projectNodeRepository_.findProjectIdForUpdate(
                transaction,
                input.projectId);
            if (!projectId) {
                error::throwNotFound(
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            const auto nodeCheckResult = co_await
                projectNodeRepository_.findProjectNodeDeleteCheckResultForUpdate(
                    transaction,
                    input.projectId,
                    input.nodeId);
            if (!nodeCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::PhaseNotFound,
                    "阶段节点不存在");
            }

            const bool isAdmin = input.operatorUserRole == user_domain::SystemRole::Admin;
            if (nodeCheckResult->creatorUserRole == user_domain::SystemRole::Employee && isAdmin) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "普通员工创建的个人自用项目不允许管理员删除阶段节点");
            }

            if (!isAdmin && nodeCheckResult->ownerUserId != input.operatorUserId) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目负责人");
            }

            if (nodeCheckResult->projectStatus == project_domain::ProjectStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompletedReadonly,
                    "项目已完成，不允许删除阶段节点");
            }

            if (nodeCheckResult->nodeStatus == domain::ProjectNodeStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::PhaseCompletedReadonly,
                    "阶段节点已完成，必须先撤销完成再删除");
            }

            const bool deleted = co_await projectNodeRepository_.deleteProjectNode(
                transaction,
                input);
            if (!deleted) {
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    "删除阶段节点失败");
            }

            co_return input.nodeId;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }
} // namespace project_tracker::modules::project_node::service
