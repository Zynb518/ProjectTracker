#pragma once

#include <string>

#include <drogon/utils/coroutine.h>

#include "modules/auth/repository/AuthRepository.h"
#include "modules/user/dto/view/SysUserView.h"

namespace project_tracker::modules::auth::service {
    namespace user_view = modules::user::dto::view;

    class AuthService {
    public:
        drogon::Task<user_view::SysUserView>
        login(const std::string &username,
              const std::string &password) const;

    private:
        repository::AuthRepository authRepository_;
    };
} // namespace project_tracker::modules::auth::service
