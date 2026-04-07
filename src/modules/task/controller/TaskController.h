#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include "filters/LoginRequiredFilter.h"
#include "modules/task/repository/TaskRepository.h"

namespace project_tracker::modules::task::controller {
    class TaskController : public drogon::HttpController<TaskController> {
    public:
        METHOD_LIST_BEGIN
            ADD_METHOD_TO(TaskController::listNodeTasks,
                          "/api/projects/{project_id}/nodes/{node_id}/subtasks",
                          drogon::Get,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(TaskController::listMyTasks,
                          "/api/my/subtasks",
                          drogon::Get,
                          filters::LoginRequiredFilter::classTypeName());
        METHOD_LIST_END

        drogon::Task<drogon::HttpResponsePtr>
        listNodeTasks(drogon::HttpRequestPtr request,
                      std::int64_t projectId,
                      std::int64_t nodeId);

        drogon::Task<drogon::HttpResponsePtr>
        listMyTasks(drogon::HttpRequestPtr request);

    private:
        // 简单读接口直接访问 repository。
        repository::TaskRepository taskRepository_;
    };
} // namespace project_tracker::modules::task::controller
