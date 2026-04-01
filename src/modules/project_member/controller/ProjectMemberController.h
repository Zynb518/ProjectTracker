#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include "filters/LoginRequiredFilter.h"
#include "modules/project_member/repository/ProjectMemberRepository.h"

namespace project_tracker::modules::project_member::controller {
    class ProjectMemberController : public drogon::HttpController<ProjectMemberController> {
    public:
        METHOD_LIST_BEGIN
            ADD_METHOD_TO(ProjectMemberController::listProjectMembers,
                          "/api/projects/{project_id}/members",
                          drogon::Get,
                          filters::LoginRequiredFilter::classTypeName());
        METHOD_LIST_END

        drogon::Task<drogon::HttpResponsePtr>
        listProjectMembers(drogon::HttpRequestPtr request,
                           std::int64_t projectId);

    private:
        // 简单读接口直接访问 repository。
        repository::ProjectMemberRepository projectMemberRepository_;
    };
} // namespace project_tracker::modules::project_member::controller
