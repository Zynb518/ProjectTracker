#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include "filters/LoginRequiredFilter.h"
#include "modules/project_ai/service/ProjectAiService.h"

namespace project_tracker::modules::project_ai::controller {
    class ProjectAiController : public drogon::HttpController<ProjectAiController> {
    public:
        METHOD_LIST_BEGIN
            ADD_METHOD_TO(ProjectAiController::generateProjectDraft,
                          "/api/ai/project-draft/generate",
                          drogon::Post,
                          filters::LoginRequiredFilter::classTypeName());
        METHOD_LIST_END

        drogon::Task<drogon::HttpResponsePtr>
        generateProjectDraft(drogon::HttpRequestPtr request);

    private:
        service::ProjectAiService projectAiService_;
    };
} // namespace project_tracker::modules::project_ai::controller
