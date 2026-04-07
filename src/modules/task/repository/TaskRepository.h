#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <drogon/utils/coroutine.h>

#include "common/db/SqlExecutor.h"
#include "modules/project/domain/ProjectEnums.h"
#include "modules/project_node/domain/ProjectNodeEnums.h"
#include "modules/task/dto/TaskDto.h"
#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::task::repository {
    // 节点下子任务列表查询条件
    struct TaskListQuery {
        std::int64_t projectId;
        std::int64_t nodeId;
        std::int64_t currentUserId;
        user::domain::SystemRole currentUserRole;
        std::optional<domain::TaskStatus> status;
        std::optional<std::int64_t> responsibleUserId;
    };

    // 节点下子任务列表查询结果
    struct TaskListResult {
        bool projectExists;
        bool hasPermission;
        bool nodeExists;
        std::vector<dto::view::TaskListItemView> list;
    };

    // 当前用户负责的子任务列表查询条件
    struct MyTaskListQuery {
        std::int64_t currentUserId;
        std::optional<domain::TaskStatus> status;
        std::optional<std::int64_t> projectId;
    };

    // 子任务详情查询条件
    struct TaskDetailQuery {
        std::int64_t subTaskId;
        std::int64_t currentUserId;
        user::domain::SystemRole currentUserRole;
    };

    // 子任务详情查询结果
    struct TaskDetailResult {
        bool hasPermission;
        std::optional<dto::view::TaskDetailView> detail;
    };

    // 创建子任务前的项目级校验信息
    struct TaskCreateProjectCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        project::domain::ProjectStatus projectStatus;
    };

    // 创建子任务前的节点级校验信息
    struct TaskCreateNodeCheckResult {
        project_node::domain::ProjectNodeStatus nodeStatus;
        std::string nodePlannedStartDate;
        std::string nodePlannedEndDate;
    };

    // 修改子任务基础信息前的项目级校验信息
    struct TaskBasicInfoUpdateProjectCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        project::domain::ProjectStatus projectStatus;
    };

    // 修改子任务基础信息前的子任务级校验信息
    struct TaskBasicInfoUpdateTaskCheckResult {
        project_node::domain::ProjectNodeStatus nodeStatus;
        domain::TaskStatus taskStatus;
    };

    enum class TaskBasicInfoUpdateFailureReason {
        InvalidDateRange,
        ResponsibleUserInvalid
    };

    // 修改子任务基础信息结果
    struct TaskBasicInfoUpdateResult {
        std::optional<dto::view::UpdatedTaskBasicInfoView> task;
        std::optional<TaskBasicInfoUpdateFailureReason> failureReason;
    };

    enum class TaskDeleteFailureReason {
        NotFound,
        Forbidden,
        ProjectCompleted,
        NodeCompleted,
        TaskCompleted
    };

    // 删除子任务结果
    struct TaskDeleteResult {
        std::optional<std::int64_t> deletedTaskId;
        std::optional<TaskDeleteFailureReason> failureReason;
    };

    // 手动开始子任务前的项目级校验信息
    struct TaskStartProjectCheckResult {
        std::int64_t projectId;
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        project::domain::ProjectStatus projectStatus;
    };

    // 手动开始子任务前的子任务级校验信息
    struct TaskStartTaskCheckResult {
        std::int64_t nodeId;
        std::int64_t responsibleUserId;
        project_node::domain::ProjectNodeStatus nodeStatus;
        domain::TaskStatus taskStatus;
    };

    class TaskRepository {
    public:
        // 查询节点下子任务列表
        drogon::Task<TaskListResult>
        listNodeTasks(const common::db::SqlExecutorPtr &executor,
                      const TaskListQuery &query) const;

        // 查询当前用户负责的子任务列表
        drogon::Task<std::vector<dto::view::MyTaskListItemView>>
        listMyTasks(const common::db::SqlExecutorPtr &executor,
                    const MyTaskListQuery &query) const;

        // 查询子任务详情
        drogon::Task<std::optional<TaskDetailResult>>
        findTaskDetail(const common::db::SqlExecutorPtr &executor,
                       const TaskDetailQuery &query) const;

        // 锁定所属项目行，供修改子任务基础信息写事务做项目级前置检查
        drogon::Task<std::optional<TaskBasicInfoUpdateProjectCheckResult>>
        findTaskBasicInfoUpdateProjectCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            std::int64_t subTaskId) const;

        // 锁定目标子任务行，供修改子任务基础信息写事务做子任务级前置检查
        drogon::Task<std::optional<TaskBasicInfoUpdateTaskCheckResult>>
        findTaskBasicInfoUpdateTaskCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            std::int64_t subTaskId) const;

        // 修改子任务基础信息
        drogon::Task<TaskBasicInfoUpdateResult>
        updateTaskBasicInfo(const common::db::SqlExecutorPtr &executor,
                            const dto::command::UpdateTaskBasicInfoInput &input) const;

        // 原子删除子任务，并返回失败原因
        drogon::Task<TaskDeleteResult>
        deleteTask(const common::db::SqlExecutorPtr &executor,
                   const dto::command::DeleteTaskInput &input) const;

        // 锁定所属项目行，供手动开始子任务写事务做项目级前置检查
        drogon::Task<std::optional<TaskStartProjectCheckResult>>
        findTaskStartProjectCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            std::int64_t subTaskId) const;

        // 锁定目标子任务行，供手动开始子任务写事务做子任务级前置检查
        drogon::Task<std::optional<TaskStartTaskCheckResult>>
        findTaskStartTaskCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            std::int64_t subTaskId) const;

        // 按手动开始动作更新子任务当前值
        drogon::Task<std::optional<dto::view::UpdatedTaskStatusView>>
        updateTaskStatusForStart(const common::db::SqlExecutorPtr &executor,
                                 std::int64_t subTaskId) const;

        // 追加一条手动开始子任务的进度记录
        drogon::Task<dto::view::TaskProgressRecordView>
        insertTaskProgressRecordForStart(const common::db::SqlExecutorPtr &executor,
                                         std::int64_t subTaskId,
                                         std::int64_t operatorUserId,
                                         const std::string &progressNote,
                                         domain::TaskStatus status) const;

        // 锁定项目行，供创建子任务写事务做项目级前置检查
        drogon::Task<std::optional<TaskCreateProjectCheckResult>>
        findTaskCreateProjectCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            std::int64_t projectId) const;

        // 锁定目标节点，供创建子任务写事务做节点级前置检查
        drogon::Task<std::optional<TaskCreateNodeCheckResult>>
        findTaskCreateNodeCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            std::int64_t projectId,
            std::int64_t nodeId) const;

        // 创建子任务；返回空表示负责人已不属于当前项目成员
        drogon::Task<std::optional<dto::view::CreatedTaskView>>
        insertTask(const common::db::SqlExecutorPtr &executor,
                   const dto::command::CreateTaskInput &input,
                   std::int64_t responsibleUserId,
                   domain::TaskPriority priority) const;
    };
} // namespace project_tracker::modules::task::repository
