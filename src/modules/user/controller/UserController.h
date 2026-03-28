#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include "filters/AdminRequiredFilter.h"
#include "modules/user/repository/UserRepository.h"

namespace project_tracker::modules::user::controller {
    class UserController : public drogon::HttpController<UserController> {
    public:
        METHOD_LIST_BEGIN
            ADD_METHOD_TO(UserController::listUsers,
                          "/api/users",
                          drogon::Get,
                          filters::AdminRequiredFilter::classTypeName());
            ADD_METHOD_TO(UserController::getUserDetail,
                          "/api/users/{user_id}",
                          drogon::Get,
                          filters::AdminRequiredFilter::classTypeName());
        METHOD_LIST_END

        drogon::Task<drogon::HttpResponsePtr>
        listUsers(drogon::HttpRequestPtr request);

        drogon::Task<drogon::HttpResponsePtr>
        getUserDetail(drogon::HttpRequestPtr request, std::int64_t userId);

    private:
        // 简单读接口直接访问 repository。
        repository::UserRepository userRepository_;
    };
} // namespace project_tracker::modules::user::controller
