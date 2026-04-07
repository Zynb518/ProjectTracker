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
