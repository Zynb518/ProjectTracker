#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include "filters/LoginRequiredFilter.h"
#include "modules/project_template/repository/ProjectTemplateRepository.h"

namespace project_tracker::modules::project_template::controller {
    class ProjectTemplateController : public drogon::HttpController<ProjectTemplateController> {
    public:
        METHOD_LIST_BEGIN
            ADD_METHOD_TO(ProjectTemplateController::listTemplates,
                          "/api/project-templates",
                          drogon::Get,
                          filters::LoginRequiredFilter::classTypeName());
        METHOD_LIST_END

        drogon::Task<drogon::HttpResponsePtr>
        listTemplates(drogon::HttpRequestPtr request);

    private:
        repository::ProjectTemplateRepository projectTemplateRepository_;
    };
} // namespace project_tracker::modules::project_template::controller
