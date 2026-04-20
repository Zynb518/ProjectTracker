#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include "modules/health/service/HealthService.h"

namespace project_tracker::modules::health::controller {
    class HealthController : public drogon::HttpController<HealthController> {
    public:
        METHOD_LIST_BEGIN
            ADD_METHOD_TO(HealthController::health, "/api/health", drogon::Get);
            ADD_METHOD_TO(HealthController::ready, "/api/health/ready", drogon::Get);
        METHOD_LIST_END

        drogon::Task<drogon::HttpResponsePtr> health(drogon::HttpRequestPtr request);

        drogon::Task<drogon::HttpResponsePtr> ready(drogon::HttpRequestPtr request);

    private:
        service::HealthService healthService_;
    };
} // namespace project_tracker::modules::health::controller
