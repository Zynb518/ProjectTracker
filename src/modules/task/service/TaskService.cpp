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
} // namespace project_tracker::modules::task::service
