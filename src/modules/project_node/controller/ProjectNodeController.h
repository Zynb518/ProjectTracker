#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include "filters/LoginRequiredFilter.h"
#include "modules/project_node/repository/ProjectNodeRepository.h"
#include "modules/project_node/service/ProjectNodeService.h"

namespace project_tracker::modules::project_node::controller {
    class ProjectNodeController : public drogon::HttpController<ProjectNodeController> {
    public:
        METHOD_LIST_BEGIN
            ADD_METHOD_TO(ProjectNodeController::createProjectNode,
                          "/api/projects/{project_id}/nodes",
                          drogon::Post,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectNodeController::listProjectNodes,
                          "/api/projects/{project_id}/nodes",
                          drogon::Get,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectNodeController::getProjectNodeDetail,
                          "/api/projects/{project_id}/nodes/{node_id}",
                          drogon::Get,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectNodeController::updateProjectNodeBasicInfo,
                          "/api/projects/{project_id}/nodes/{node_id}",
                          drogon::Patch,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectNodeController::startProjectNode,
                          "/api/projects/{project_id}/nodes/{node_id}/start",
                          drogon::Post,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectNodeController::completeProjectNode,
                          "/api/projects/{project_id}/nodes/{node_id}/complete",
                          drogon::Post,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectNodeController::deleteProjectNode,
                          "/api/projects/{project_id}/nodes/{node_id}",
                          drogon::Delete,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectNodeController::reorderProjectNodes,
                          "/api/projects/{project_id}/nodes/reorder",
                          drogon::Put,
                          filters::LoginRequiredFilter::classTypeName());
        METHOD_LIST_END

        drogon::Task<drogon::HttpResponsePtr>
        createProjectNode(drogon::HttpRequestPtr request,
                          std::int64_t projectId);

        drogon::Task<drogon::HttpResponsePtr>
        listProjectNodes(drogon::HttpRequestPtr request,
                         std::int64_t projectId);

        drogon::Task<drogon::HttpResponsePtr>
        getProjectNodeDetail(drogon::HttpRequestPtr request,
                             std::int64_t projectId,
                             std::int64_t nodeId);

        drogon::Task<drogon::HttpResponsePtr>
        updateProjectNodeBasicInfo(drogon::HttpRequestPtr request,
                                   std::int64_t projectId,
                                   std::int64_t nodeId);

        drogon::Task<drogon::HttpResponsePtr>
        startProjectNode(drogon::HttpRequestPtr request,
                         std::int64_t projectId,
                         std::int64_t nodeId);

        drogon::Task<drogon::HttpResponsePtr>
        completeProjectNode(drogon::HttpRequestPtr request,
                            std::int64_t projectId,
                            std::int64_t nodeId);

        drogon::Task<drogon::HttpResponsePtr>
        deleteProjectNode(drogon::HttpRequestPtr request,
                          std::int64_t projectId,
                          std::int64_t nodeId);

        drogon::Task<drogon::HttpResponsePtr>
        reorderProjectNodes(drogon::HttpRequestPtr request,
                            std::int64_t projectId);

    private:
        // 简单读接口直接访问 repository，写接口走 service。
        repository::ProjectNodeRepository projectNodeRepository_;
        service::ProjectNodeService projectNodeService_;
    };
} // namespace project_tracker::modules::project_node::controller
