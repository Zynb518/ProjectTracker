#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include "filters/LoginRequiredFilter.h"
#include "modules/auth/repository/AuthRepository.h"
#include "modules/auth/service/AuthService.h"

namespace project_tracker::modules::auth::controller {
    namespace repository = auth::repository;

    class AuthController : public drogon::HttpController<AuthController> {
    public:
        METHOD_LIST_BEGIN
            ADD_METHOD_TO(AuthController::login, "/api/auth/login", drogon::Post);
            ADD_METHOD_TO(AuthController::logout, "/api/auth/logout", drogon::Post);
            ADD_METHOD_TO(AuthController::me,
                          "/api/auth/me",
                          drogon::Get,
                          filters::LoginRequiredFilter::classTypeName());

        METHOD_LIST_END

        drogon::Task<drogon::HttpResponsePtr> login(drogon::HttpRequestPtr request);

        drogon::Task<drogon::HttpResponsePtr> logout(drogon::HttpRequestPtr request);

        drogon::Task<drogon::HttpResponsePtr> me(drogon::HttpRequestPtr request);

    private:
        service::AuthService authService_;
        repository::AuthRepository authRepository_;
    };
} // namespace project_tracker::modules::auth::controller
