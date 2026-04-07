#pragma once

#include <drogon/utils/coroutine.h>

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

    private:
        repository::TaskRepository taskRepository_;
    };
} // namespace project_tracker::modules::task::service
