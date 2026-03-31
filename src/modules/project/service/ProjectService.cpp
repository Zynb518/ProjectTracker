#include "modules/project/service/ProjectService.h"

#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::project::service {
    namespace error = project_tracker::common::error;
    namespace user_domain = modules::user::domain;

    drogon::Task<dto::view::CreatedProjectView>
    ProjectService::createProject(const dto::command::CreateProjectInput &input) const {
        if (input.plannedStartDate > input.plannedEndDate) {
            error::throwConflict(
                error::ErrorCode::InvalidDateRange,
                "planned_start_date 不能晚于 planned_end_date");
        }

        try {
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();
            const auto project = co_await projectRepository_.insertProject(transaction, input);
            co_await projectRepository_.insertProjectMember(
                transaction,
                project.id,
                input.creatorUserId,
                input.creatorUserId);
            co_return project;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }

    drogon::Task<dto::view::UpdatedProjectBasicInfoView>
    ProjectService::updateProjectBasicInfo(
        const dto::command::UpdateProjectBasicInfoInput &input) const {
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
            const auto project = co_await projectRepository_.updateProjectBasicInfo(transaction, input);
            if (!project) {

                // 当前空结果统一包含三种情况：
                // 1. 项目不存在
                // 2. 项目已完成，被更新 SQL 的 status 条件拦截
                // 3. 当前操作者不是管理员且不是项目负责人

                error::throwNotFound(
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在/项目已完成/当前操作者不是管理员且不是项目负责人");
            }

            co_return *project;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }

    drogon::Task<std::int64_t>
    ProjectService::deleteProject(const dto::command::DeleteProjectInput &input) const {
        try {
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();
            const auto ownerUserId = co_await projectRepository_.findProjectDeleteCheckResult(
                transaction,
                input.projectId);
            if (!ownerUserId) {
                error::throwNotFound(
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            const bool isAdmin = input.operatorUserRole == user_domain::SystemRole::Admin;
            if (!isAdmin && *ownerUserId != input.operatorUserId) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目负责人");
            }

            const bool deleted = co_await projectRepository_.deleteProject(transaction, input);
            if (!deleted) {
                error::throwNotFound(
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            co_return input.projectId;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }

    drogon::Task<dto::view::UpdatedProjectStatusView>
    ProjectService::startProject(const dto::command::StartProjectInput &input) const {
        try {
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();
            const auto project = co_await projectRepository_.findProjectStartCheckResult(
                transaction,
                input.projectId);
            if (!project) {
                error::throwNotFound(
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            const bool isAdmin = input.operatorUserRole == user_domain::SystemRole::Admin;
            if (project->creatorUserRole == user_domain::SystemRole::Employee && isAdmin) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "普通员工创建的个人自用项目不允许管理员执行开始操作");
            }

            if (!isAdmin && project->ownerUserId != input.operatorUserId) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目负责人");
            }

            if (project->status == domain::ProjectStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompletedReadonly,
                    "项目已完成，必须先撤销完成");
            }

            if (project->hasNodes) {
                error::throwConflict(
                    error::ErrorCode::StartConditionNotMet,
                    "项目下存在阶段节点，不允许手动开始");
            }

            const auto updatedProject = co_await projectRepository_.updateProjectStatusForStart(
                transaction,
                input);

            if (!updatedProject) {
                error::throwConflict(
                    error::ErrorCode::StartConditionNotMet,
                    "项目当前不满足手动开始条件");
            }

            co_return *updatedProject;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }

    drogon::Task<dto::view::UpdatedProjectStatusView>
    ProjectService::completeProject(const dto::command::CompleteProjectInput &input) const {
        try {
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();
            const auto project = co_await projectRepository_.findProjectCompleteCheckResult(
                transaction,
                input.projectId);
            if (!project) {
                error::throwNotFound(
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            const bool isAdmin = input.operatorUserRole == user_domain::SystemRole::Admin;
            if (project->creatorUserRole == user_domain::SystemRole::Employee && isAdmin) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "普通员工创建的个人自用项目不允许管理员执行完成操作");
            }

            if (!isAdmin && project->ownerUserId != input.operatorUserId) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目负责人");
            }

            if (project->status == domain::ProjectStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompletedReadonly,
                    "项目已完成，不能重复完成");
            }

            if (project->nodeCount > 0 && project->completedNodeCount != project->nodeCount) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompleteConditionNotMet,
                    "项目下存在未完成阶段节点，不能手动完成");
            }

            const auto updatedProject = co_await projectRepository_.updateProjectStatusForComplete(
                transaction,
                input);
            if (!updatedProject) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompleteConditionNotMet,
                    "项目当前不满足手动完成条件");
            }

            co_return *updatedProject;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }
} // namespace project_tracker::modules::project::service
