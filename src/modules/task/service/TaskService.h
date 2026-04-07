#pragma once

#include <drogon/utils/coroutine.h>

#include "modules/project/repository/ProjectRepository.h"
#include "modules/project_node/repository/ProjectNodeRepository.h"
#include "modules/task/dto/TaskDto.h"
#include "modules/task/repository/TaskRepository.h"

namespace project_tracker::modules::task::service {
    class TaskService {
    public:
        // 创建子任务
        drogon::Task<dto::view::CreatedTaskView>
        createTask(const dto::command::CreateTaskInput &input) const;

        // 修改子任务基础信息
        drogon::Task<dto::view::UpdatedTaskBasicInfoView>
        updateTaskBasicInfo(const dto::command::UpdateTaskBasicInfoInput &input) const;

        // 手动开始子任务
        drogon::Task<dto::view::StartedTaskView>
        startTask(const dto::command::StartTaskInput &input) const;

        // 删除子任务
        drogon::Task<std::int64_t>
        deleteTask(const dto::command::DeleteTaskInput &input) const;

    private:
        project::repository::ProjectRepository projectRepository_;
        project_node::repository::ProjectNodeRepository projectNodeRepository_;
        repository::TaskRepository taskRepository_;
    };
} // namespace project_tracker::modules::task::service
