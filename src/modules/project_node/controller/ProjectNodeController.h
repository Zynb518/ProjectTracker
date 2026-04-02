#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include "filters/LoginRequiredFilter.h"
#include "modules/project_node/repository/ProjectNodeRepository.h"

namespace project_tracker::modules::project_node::controller {
    class ProjectNodeController : public drogon::HttpController<ProjectNodeController> {
    public:
        METHOD_LIST_BEGIN
            ADD_METHOD_TO(ProjectNodeController::listProjectNodes,
                          "/api/projects/{project_id}/nodes",
                          drogon::Get,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectNodeController::getProjectNodeDetail,
                          "/api/projects/{project_id}/nodes/{node_id}",
                          drogon::Get,
                          filters::LoginRequiredFilter::classTypeName());
        METHOD_LIST_END

        drogon::Task<drogon::HttpResponsePtr>
        listProjectNodes(drogon::HttpRequestPtr request,
                         std::int64_t projectId);

        drogon::Task<drogon::HttpResponsePtr>
        getProjectNodeDetail(drogon::HttpRequestPtr request,
                             std::int64_t projectId,
                             std::int64_t nodeId);

    private:
        // 12.1 是简单读接口，先直接访问 repository。
        repository::ProjectNodeRepository projectNodeRepository_;
    };
} // namespace project_tracker::modules::project_node::controller
