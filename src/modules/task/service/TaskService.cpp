#include "modules/task/service/TaskService.h"

#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::task::service {
    namespace error = project_tracker::common::error;
    namespace project_domain = modules::project::domain;
    namespace project_node_domain = modules::project_node::domain;
    namespace user_domain = modules::user::domain;

    drogon::Task<dto::view::CreatedTaskView>
    TaskService::createTask(const dto::command::CreateTaskInput &input) const {
        if (input.plannedStartDate > input.plannedEndDate) {
            error::throwConflict(
                error::ErrorCode::InvalidDateRange,
                "planned_start_date 不能晚于 planned_end_date");
        }

        try {
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();

            const auto projectCheckResult = co_await
                taskRepository_.findTaskCreateProjectCheckResultForUpdate(
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
                    "普通员工创建的个人自用项目不允许管理员创建子任务");
            }

            if (!isAdmin && projectCheckResult->ownerUserId != input.creatorUserId) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目负责人");
            }

            if (projectCheckResult->projectStatus == project_domain::ProjectStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompletedReadonly,
                    "项目已完成，不允许创建子任务");
            }

            const auto nodeCheckResult = co_await
                taskRepository_.findTaskCreateNodeCheckResultForUpdate(
                    transaction,
                    input.projectId,
                    input.nodeId);
            if (!nodeCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::PhaseNotFound,
                    "阶段节点不存在");
            }

            if (nodeCheckResult->nodeStatus == project_node_domain::ProjectNodeStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::PhaseCompletedReadonly,
                    "阶段节点已完成，不允许创建子任务");
            }

            if (input.plannedStartDate < nodeCheckResult->nodePlannedStartDate ||
                input.plannedEndDate > nodeCheckResult->nodePlannedEndDate) {
                error::throwConflict(
                    error::ErrorCode::InvalidDateRange,
                    "子任务时间区间不合法或不在所属节点时间范围内");
            }

            const std::int64_t responsibleUserId = input.responsibleUserId.value_or(
                projectCheckResult->ownerUserId);
            const auto priority = input.priority.value_or(domain::TaskPriority::Medium);

            const auto task = co_await taskRepository_.insertTask(
                transaction,
                input,
                responsibleUserId,
                priority);
            if (!task) {
                error::throwConflict(
                    error::ErrorCode::SubTaskResponsibleUserInvalid,
                    "子任务负责人不属于当前项目成员");
            }

            co_return *task;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }

    drogon::Task<dto::view::UpdatedTaskBasicInfoView>
    TaskService::updateTaskBasicInfo(
        const dto::command::UpdateTaskBasicInfoInput &input) const {
        if (!input.name && !input.description &&
            !input.responsibleUserId && !input.priority &&
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
                taskRepository_.findTaskBasicInfoUpdateProjectCheckResultForUpdate(
                    transaction,
                    input.subTaskId);
            if (!projectCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::SubTaskNotFound,
                    "子任务不存在");
            }

            const bool isAdmin = input.operatorUserRole == user_domain::SystemRole::Admin;
            if (projectCheckResult->creatorUserRole == user_domain::SystemRole::Employee && isAdmin) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "普通员工创建的个人自用项目不允许管理员修改子任务");
            }

            if (!isAdmin && projectCheckResult->ownerUserId != input.operatorUserId) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目负责人");
            }

            if (projectCheckResult->projectStatus == project_domain::ProjectStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompletedReadonly,
                    "项目已完成，不允许修改子任务");
            }

            const auto taskCheckResult = co_await
                taskRepository_.findTaskBasicInfoUpdateTaskCheckResultForUpdate(
                    transaction,
                    input.subTaskId);
            if (!taskCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::SubTaskNotFound,
                    "子任务不存在");
            }

            if (taskCheckResult->nodeStatus == project_node_domain::ProjectNodeStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::PhaseCompletedReadonly,
                    "阶段节点已完成，不允许修改子任务");
            }

            if (taskCheckResult->taskStatus == domain::TaskStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::SubTaskCompletedReadonly,
                    "子任务已完成，不允许修改基础信息");
            }

            const auto updateResult = co_await taskRepository_.updateTaskBasicInfo(
                transaction,
                input);
            if (!updateResult.task) {
                if (updateResult.failureReason ==
                    repository::TaskBasicInfoUpdateFailureReason::ResponsibleUserInvalid) {
                    error::throwConflict(
                        error::ErrorCode::SubTaskResponsibleUserInvalid,
                        "子任务负责人不属于当前项目成员");
                }

                if (updateResult.failureReason ==
                    repository::TaskBasicInfoUpdateFailureReason::InvalidDateRange) {
                    error::throwConflict(
                        error::ErrorCode::InvalidDateRange,
                        "子任务时间区间不合法或不在所属节点时间范围内");
                }

                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    "修改子任务基础信息失败");
            }

            co_return *updateResult.task;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }

    drogon::Task<dto::view::StartedTaskView>
    TaskService::startTask(const dto::command::StartTaskInput &input) const {
        std::shared_ptr<drogon::orm::Transaction> transaction;
        bool hasWrittenBeforeFinish = false;

        try {
            const auto dbClient = drogon::app().getDbClient();
            transaction = co_await dbClient->newTransactionCoro();

            const auto projectCheckResult = co_await
                taskRepository_.findTaskStartProjectCheckResultForUpdate(
                    transaction,
                    input.subTaskId);
            if (!projectCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::SubTaskNotFound,
                    "子任务不存在");
            }

            const auto taskCheckResult = co_await
                taskRepository_.findTaskStartTaskCheckResultForUpdate(
                    transaction,
                    input.subTaskId);
            if (!taskCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::SubTaskNotFound,
                    "子任务不存在");
            }

            const bool isAdmin = input.operatorUserRole == user_domain::SystemRole::Admin;
            if (projectCheckResult->creatorUserRole == user_domain::SystemRole::Employee && isAdmin) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "普通员工创建的个人自用项目不允许管理员开始子任务");
            }

            if (!isAdmin &&
                projectCheckResult->ownerUserId != input.operatorUserId &&
                taskCheckResult->responsibleUserId != input.operatorUserId) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者既不是项目负责人也不是子任务负责人");
            }

            if (projectCheckResult->projectStatus == project_domain::ProjectStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompletedReadonly,
                    "项目已完成，必须先撤销完成");
            }

            if (taskCheckResult->nodeStatus == project_node_domain::ProjectNodeStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::PhaseCompletedReadonly,
                    "阶段节点已完成，必须先撤销完成");
            }

            if (taskCheckResult->taskStatus == domain::TaskStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::SubTaskCompletedReadonly,
                    "子任务已完成，必须先撤销完成");
            }

            const auto task = co_await taskRepository_.updateTaskStatusForStart(
                transaction,
                input.subTaskId);
            if (!task) {
                error::throwConflict(
                    error::ErrorCode::StartConditionNotMet,
                    "子任务当前不满足手动开始条件");
            }
            hasWrittenBeforeFinish = true;

            const std::string progressNote = input.progressNote.empty()
                                                 ? "开始处理该任务"
                                                 : input.progressNote;
            const auto progressRecord = co_await taskRepository_.insertTaskProgressRecordForStart(
                transaction,
                input.subTaskId,
                input.operatorUserId,
                progressNote,
                task->status);

            if (taskCheckResult->nodeStatus == project_node_domain::ProjectNodeStatus::NotStarted) {
                co_await projectNodeRepository_.updateProjectNodeStatusForTaskSignal(
                    transaction,
                    taskCheckResult->nodeId);
            }

            if (projectCheckResult->projectStatus == project_domain::ProjectStatus::NotStarted) {
                co_await projectRepository_.updateProjectStatusForNodeSignal(
                    transaction,
                    projectCheckResult->projectId);
            }

            co_return dto::view::StartedTaskView{
                .subtask = *task,
                .progressRecord = progressRecord
            };
        } catch (const error::BusinessException &) {
            if (hasWrittenBeforeFinish && transaction) {
                transaction->rollback();
            }
            throw;
        } catch (const drogon::orm::DrogonDbException &) {
            if (hasWrittenBeforeFinish && transaction) {
                transaction->rollback();
            }
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }

    drogon::Task<dto::view::StartedTaskView>
    TaskService::submitTaskProgress(
        const dto::command::SubmitTaskProgressInput &input) const {
        std::shared_ptr<drogon::orm::Transaction> transaction;
        bool hasWrittenBeforeFinish = false;

        try {
            const auto dbClient = drogon::app().getDbClient();
            transaction = co_await dbClient->newTransactionCoro();

            const auto projectCheckResult = co_await
                taskRepository_.findTaskStartProjectCheckResultForUpdate(
                    transaction,
                    input.subTaskId);
            if (!projectCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::SubTaskNotFound,
                    "子任务不存在");
            }

            const auto taskCheckResult = co_await
                taskRepository_.findTaskSubmitProgressTaskCheckResultForUpdate(
                    transaction,
                    input.subTaskId);
            if (!taskCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::SubTaskNotFound,
                    "子任务不存在");
            }

            const bool isAdmin = input.operatorUserRole == user_domain::SystemRole::Admin;
            if (projectCheckResult->creatorUserRole == user_domain::SystemRole::Employee && isAdmin) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "普通员工创建的个人自用项目不允许管理员提交子任务进度");
            }

            if (taskCheckResult->responsibleUserId != input.operatorUserId) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者不是子任务负责人");
            }

            if (projectCheckResult->projectStatus == project_domain::ProjectStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompletedReadonly,
                    "项目已完成，必须先撤销完成");
            }

            if (taskCheckResult->nodeStatus == project_node_domain::ProjectNodeStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::PhaseCompletedReadonly,
                    "阶段节点已完成，必须先撤销完成");
            }

            if (taskCheckResult->taskStatus == domain::TaskStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::SubTaskCompletedReadonly,
                    "子任务已完成，必须先撤销完成");
            }

            const auto task = co_await taskRepository_.updateTaskStatusForSubmitProgress(
                transaction,
                input);
            if (!task) {
                error::throwInternalError(
                    error::ErrorCode::InternalError,
                    "提交子任务进度失败");
            }
            hasWrittenBeforeFinish = true;

            const auto progressRecord = co_await
                taskRepository_.insertTaskProgressRecordForSubmitProgress(
                    transaction,
                    input,
                    task->status);

            if (task->status != domain::TaskStatus::NotStarted &&
                taskCheckResult->nodeStatus == project_node_domain::ProjectNodeStatus::NotStarted) {
                co_await projectNodeRepository_.updateProjectNodeStatusForTaskSignal(
                    transaction,
                    taskCheckResult->nodeId);
            }

            if (task->status != domain::TaskStatus::NotStarted &&
                projectCheckResult->projectStatus == project_domain::ProjectStatus::NotStarted) {
                co_await projectRepository_.updateProjectStatusForNodeSignal(
                    transaction,
                    projectCheckResult->projectId);
            }

            co_return dto::view::StartedTaskView{
                .subtask = *task,
                .progressRecord = progressRecord
            };
        } catch (const error::BusinessException &) {
            if (hasWrittenBeforeFinish && transaction) {
                transaction->rollback();
            }
            throw;
        } catch (const drogon::orm::DrogonDbException &) {
            if (hasWrittenBeforeFinish && transaction) {
                transaction->rollback();
            }
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }

    drogon::Task<dto::view::UpdatedTaskStatusView>
    TaskService::reopenTask(const dto::command::TaskStatusActionInput &input) const {
        try {
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();

            const auto projectCheckResult = co_await
                taskRepository_.findTaskStartProjectCheckResultForUpdate(
                    transaction,
                    input.subTaskId);
            if (!projectCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::SubTaskNotFound,
                    "子任务不存在");
            }

            const auto taskCheckResult = co_await
                taskRepository_.findTaskStartTaskCheckResultForUpdate(
                    transaction,
                    input.subTaskId);
            if (!taskCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::SubTaskNotFound,
                    "子任务不存在");
            }

            const bool isAdmin = input.operatorUserRole == user_domain::SystemRole::Admin;
            if (projectCheckResult->creatorUserRole == user_domain::SystemRole::Employee && isAdmin) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "普通员工创建的个人自用项目不允许管理员撤销子任务完成");
            }

            if (!isAdmin &&
                projectCheckResult->ownerUserId != input.operatorUserId &&
                taskCheckResult->responsibleUserId != input.operatorUserId) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者既不是项目负责人也不是子任务负责人");
            }

            if (projectCheckResult->projectStatus == project_domain::ProjectStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompletedReadonly,
                    "项目已完成，必须先撤销项目完成");
            }

            if (taskCheckResult->nodeStatus == project_node_domain::ProjectNodeStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::PhaseCompletedReadonly,
                    "阶段节点已完成，必须先撤销完成");
            }

            if (taskCheckResult->taskStatus != domain::TaskStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::ReopenFailed,
                    "子任务当前不是已完成状态，不能撤销完成");
            }

            const auto restoreSnapshot = co_await taskRepository_.findTaskReopenRestoreSnapshot(
                transaction,
                input.subTaskId);

            const auto restoreStatus = restoreSnapshot.latestUnfinishedStatus.value_or(
                domain::TaskStatus::NotStarted);
            const int restoreProgressPercent =
                restoreSnapshot.latestUnfinishedProgressPercent.value_or(0);

            const auto task = co_await taskRepository_.updateTaskStatusForReopen(
                transaction,
                input.subTaskId,
                restoreStatus,
                restoreProgressPercent);
            if (!task) {
                error::throwConflict(
                    error::ErrorCode::ReopenFailed,
                    "撤销子任务完成失败");
            }

            co_return *task;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }

    drogon::Task<std::int64_t>
    TaskService::deleteTask(const dto::command::DeleteTaskInput &input) const {
        const auto dbClient = drogon::app().getDbClient();
        const auto deleteResult = co_await taskRepository_.deleteTask(dbClient, input);

        if (deleteResult.deletedTaskId) {
            co_return *deleteResult.deletedTaskId;
        }

        if (deleteResult.failureReason == repository::TaskDeleteFailureReason::NotFound) {
            error::throwNotFound(
                error::ErrorCode::SubTaskNotFound,
                "子任务不存在");
        }

        if (deleteResult.failureReason == repository::TaskDeleteFailureReason::Forbidden) {
            error::throwForbidden(
                error::ErrorCode::Forbidden,
                "当前操作者无权限删除子任务");
        }

        if (deleteResult.failureReason == repository::TaskDeleteFailureReason::ProjectCompleted) {
            error::throwConflict(
                error::ErrorCode::ProjectCompletedReadonly,
                "项目已完成，不允许删除子任务");
        }

        if (deleteResult.failureReason == repository::TaskDeleteFailureReason::NodeCompleted) {
            error::throwConflict(
                error::ErrorCode::PhaseCompletedReadonly,
                "阶段节点已完成，不允许删除子任务");
        }

        if (deleteResult.failureReason == repository::TaskDeleteFailureReason::TaskCompleted) {
            error::throwConflict(
                error::ErrorCode::SubTaskCompletedReadonly,
                "子任务已完成，必须先撤销完成再删除");
        }

        error::throwInternalError(
            error::ErrorCode::InternalError,
            "删除子任务失败");
    }
} // namespace project_tracker::modules::task::service
