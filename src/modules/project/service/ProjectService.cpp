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
            const auto projectCheckResult = co_await
                projectRepository_.findProjectBasicInfoUpdateCheckResultForUpdate(
                    transaction,
                    input.projectId);
            if (!projectCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            const bool isAdmin = input.operatorUserRole == user_domain::SystemRole::Admin;
            if (projectCheckResult->creatorUserRole == user_domain::SystemRole::Employee && isAdmin) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "普通员工创建的个人自用项目不允许管理员修改基础信息");
            }

            if (!isAdmin && projectCheckResult->ownerUserId != input.operatorUserId) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目负责人");
            }

            if (projectCheckResult->status == domain::ProjectStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompletedReadonly,
                    "项目已完成，不允许修改基础信息");
            }

            const auto project = co_await projectRepository_.updateProjectBasicInfo(transaction, input);
            if (!project) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompletedReadonly,
                    "修改项目基础信息失败");
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
            const auto ownerUserId = co_await projectRepository_.findProjectDeleteCheckResultForUpdate(
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

            const bool deleted = co_await projectRepository_.deleteProject(
                transaction,
                input.projectId);
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

    drogon::Task<repository::ProjectOwnerCandidatePage>
    ProjectService::listProjectOwnerCandidates(
        const dto::command::ListProjectOwnerCandidatesInput &input) const {
        try {
            const auto dbClient = drogon::app().getDbClient();
            const bool isAdmin = input.operatorUserRole == user_domain::SystemRole::Admin;

            const auto queryResult = co_await projectRepository_.listProjectOwnerCandidates(
                dbClient,
                repository::ProjectOwnerCandidateQuery{
                    .projectId = input.projectId,
                    .operatorUserId = input.operatorUserId,
                    .includeAdminCandidates = isAdmin,
                    .keyword = input.keyword,
                    .page = input.page,
                    .pageSize = input.pageSize
                });

            if (!queryResult.projectExists) {
                error::throwNotFound(
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            if (!queryResult.hasPermission) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目负责人");
            }

            if (!queryResult.ownerTransferAllowed) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "普通员工创建的个人自用项目不允许转交负责人");
            }

            co_return queryResult.page;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }

    drogon::Task<dto::view::TransferredProjectOwnerView>
    ProjectService::transferProjectOwner(const dto::command::TransferProjectOwnerInput &input) const {
        std::shared_ptr<drogon::orm::Transaction> transaction;
        bool hasWrittenBeforeFinish = false;

        try {
            const auto dbClient = drogon::app().getDbClient();
            transaction = co_await dbClient->newTransactionCoro();

            const auto projectCheckResult = co_await projectRepository_.findProjectOwnerTransferProjectCheckResultForUpdate(
                transaction,
                input.projectId);
            if (!projectCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            const auto targetUserCheckResult = co_await projectRepository_.findTransferTargetUserCheckResultForUpdate(
                transaction,
                input.projectId,
                input.targetUserId);
            if (!targetUserCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::UserNotFound,
                    "目标用户不存在");
            }

            const bool isAdmin = input.operatorUserRole == user_domain::SystemRole::Admin;
            if (!isAdmin && projectCheckResult->previousOwnerUserId != input.operatorUserId) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目负责人");
            }

            if (projectCheckResult->creatorUserRole == user_domain::SystemRole::Employee) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "普通员工创建的个人自用项目不允许转交负责人");
            }

            if (input.targetUserId == projectCheckResult->previousOwnerUserId) {
                error::throwConflict(
                    error::ErrorCode::OwnerTransferTargetInvalid,
                    "target_user_id 不能与当前负责人相同");
            }

            if (targetUserCheckResult->targetUserStatus != user_domain::UserStatus::Enabled) {
                error::throwConflict(
                    error::ErrorCode::OwnerTransferTargetInvalid,
                    "目标用户必须是启用状态");
            }

            if (isAdmin) {
                if (targetUserCheckResult->targetUserRole != user_domain::SystemRole::Admin &&
                    targetUserCheckResult->targetUserRole != user_domain::SystemRole::ProjectManager) {
                    error::throwConflict(
                        error::ErrorCode::OwnerTransferTargetInvalid,
                        "管理员转交时，目标用户必须是管理员或项目经理");
                }
            } else if (targetUserCheckResult->targetUserRole != user_domain::SystemRole::ProjectManager) {
                error::throwConflict(
                    error::ErrorCode::OwnerTransferTargetInvalid,
                    "项目负责人转交时，目标用户必须是项目经理");
            }

            // 先补成员，再转交负责人，保证转交成功后的负责人一定属于当前项目成员。
            const bool autoAddedAsMember = !targetUserCheckResult->targetIsProjectMember;
            if (autoAddedAsMember) {
                co_await projectRepository_.ensureProjectMemberExists(
                    transaction,
                    input.projectId,
                    input.targetUserId,
                    input.operatorUserId);
                hasWrittenBeforeFinish = true;
            }

            const auto project = co_await projectRepository_.updateProjectOwner(transaction, input);
            if (!project) {
                if (hasWrittenBeforeFinish) {
                    transaction->rollback();
                    hasWrittenBeforeFinish = false;
                }
                error::throwNotFound(
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            auto result = *project;
            result.autoAddedAsMember = autoAddedAsMember;
            co_return result;
        } catch (const error::BusinessException &) {
            if (hasWrittenBeforeFinish && transaction) {
                transaction->rollback();
            }
            throw;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }

    drogon::Task<dto::view::UpdatedProjectStatusView>
    ProjectService::startProject(const dto::command::ProjectStatusActionInput &input) const {
        try {
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();
            const auto project = co_await projectRepository_.findProjectStartCheckResultForUpdate(
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

            const auto updatedProject = co_await projectRepository_.updateProjectStatusForStart(
                transaction,
                input.projectId);

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
    ProjectService::completeProject(const dto::command::ProjectStatusActionInput &input) const {
        try {
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();
            const auto project = co_await projectRepository_.findProjectCompleteCheckResultForUpdate(
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

            const auto updatedProject = co_await projectRepository_.updateProjectStatusForComplete(
                transaction,
                input.projectId);
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

    drogon::Task<dto::view::UpdatedProjectStatusView>
    ProjectService::reopenProject(const dto::command::ProjectStatusActionInput &input) const {
        try {
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();
            const auto project = co_await projectRepository_.findProjectReopenCheckResultForUpdate(
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
                    "普通员工创建的个人自用项目不允许管理员执行撤销完成操作");
            }

            if (!isAdmin && project->ownerUserId != input.operatorUserId) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目负责人");
            }

            if (project->status != domain::ProjectStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::ReopenFailed,
                    "项目当前不是已完成状态，不能撤销完成");
            }

            const auto updatedProject = co_await projectRepository_.updateProjectStatusForReopen(
                transaction,
                input.projectId);
            if (!updatedProject) {
                error::throwConflict(
                    error::ErrorCode::ReopenFailed,
                    "撤销项目完成失败");
            }

            co_return *updatedProject;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }
} // namespace project_tracker::modules::project::service
