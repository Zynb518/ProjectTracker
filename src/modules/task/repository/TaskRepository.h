#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <drogon/utils/coroutine.h>

#include "common/db/SqlExecutor.h"
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
    };
} // namespace project_tracker::modules::task::repository
