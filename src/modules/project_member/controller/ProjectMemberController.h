#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include "filters/LoginRequiredFilter.h"
#include "modules/project_member/repository/ProjectMemberRepository.h"
#include "modules/project_member/service/ProjectMemberService.h"

namespace project_tracker::modules::project_member::controller {
    class ProjectMemberController : public drogon::HttpController<ProjectMemberController> {
    public:
        METHOD_LIST_BEGIN
            ADD_METHOD_TO(ProjectMemberController::listProjectMembers,
                          "/api/projects/{project_id}/members",
                          drogon::Get,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectMemberController::addProjectMember,
                          "/api/projects/{project_id}/members",
                          drogon::Post,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(ProjectMemberController::listProjectMemberCandidates,
                          "/api/projects/{project_id}/members/candidates",
                          drogon::Get,
                          filters::LoginRequiredFilter::classTypeName());
        METHOD_LIST_END

        drogon::Task<drogon::HttpResponsePtr>
        listProjectMembers(drogon::HttpRequestPtr request,
                           std::int64_t projectId);

        drogon::Task<drogon::HttpResponsePtr>
        addProjectMember(drogon::HttpRequestPtr request,
                         std::int64_t projectId);

        drogon::Task<drogon::HttpResponsePtr>
        listProjectMemberCandidates(drogon::HttpRequestPtr request,
                                    std::int64_t projectId);

    private:
        // 简单读接口直接访问 repository。
        repository::ProjectMemberRepository projectMemberRepository_;
        service::ProjectMemberService projectMemberService_;
    };
} // namespace project_tracker::modules::project_member::controller
