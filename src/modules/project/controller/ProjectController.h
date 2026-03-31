#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include "filters/LoginRequiredFilter.h"
#include "modules/project/repository/ProjectRepository.h"
#include "modules/project/service/ProjectService.h"

namespace project_tracker::modules::project::controller {
    class ProjectController : public drogon::HttpController<ProjectController> {
    public:
        METHOD_LIST_BEGIN
            ADD_METHOD_TO(ProjectController::createProject,
                          "/api/projects",
                          drogon::Post,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectController::listProjects,
                          "/api/projects",
                          drogon::Get,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectController::getProjectDetail,
                          "/api/projects/{project_id}",
                          drogon::Get,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectController::updateProjectBasicInfo,
                          "/api/projects/{project_id}",
                          drogon::Patch,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectController::deleteProject,
                          "/api/projects/{project_id}",
                          drogon::Delete,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectController::startProject,
                          "/api/projects/{project_id}/start",
                          drogon::Post,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectController::completeProject,
                          "/api/projects/{project_id}/complete",
                          drogon::Post,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectController::reopenProject,
                          "/api/projects/{project_id}/reopen",
                          drogon::Post,
                          filters::LoginRequiredFilter::classTypeName());
        METHOD_LIST_END

        drogon::Task<drogon::HttpResponsePtr>
        listProjects(drogon::HttpRequestPtr request);

        drogon::Task<drogon::HttpResponsePtr>
        createProject(drogon::HttpRequestPtr request);

        drogon::Task<drogon::HttpResponsePtr>
        getProjectDetail(drogon::HttpRequestPtr request, std::int64_t projectId);

        drogon::Task<drogon::HttpResponsePtr>
        updateProjectBasicInfo(drogon::HttpRequestPtr request, std::int64_t projectId);

        drogon::Task<drogon::HttpResponsePtr>
        deleteProject(drogon::HttpRequestPtr request, std::int64_t projectId);

        drogon::Task<drogon::HttpResponsePtr>
        startProject(drogon::HttpRequestPtr request, std::int64_t projectId);

        drogon::Task<drogon::HttpResponsePtr>
        completeProject(drogon::HttpRequestPtr request, std::int64_t projectId);

        drogon::Task<drogon::HttpResponsePtr>
        reopenProject(drogon::HttpRequestPtr request, std::int64_t projectId);

    private:
        // 简单读接口直接访问 repository。
        repository::ProjectRepository projectRepository_;
        service::ProjectService projectService_;
    };
} // namespace project_tracker::modules::project::controller
