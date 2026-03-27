#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include "modules/auth/service/AuthService.h"

namespace project_tracker::modules::auth::controller {
    class AuthController : public drogon::HttpController<AuthController> {
    public:
        METHOD_LIST_BEGIN
            ADD_METHOD_TO(AuthController::login, "/api/auth/login", drogon::Post);
        METHOD_LIST_END

        drogon::Task<drogon::HttpResponsePtr> login(drogon::HttpRequestPtr request);

    private:
        service::AuthService authService_;
    };
} // namespace project_tracker::modules::auth::controller
