#pragma once

#include <optional>
#include <string>

#include <drogon/utils/coroutine.h>

#include "modules/user/dto/view/SysUserView.h"
#include "modules/user/repository/UserRepository.h"

namespace project_tracker::modules::user::service {
    // 创建用户命令
    struct CreateUserCommand {
        std::string username;
        std::string password;
        std::string realName;
        std::optional<int> systemRole;
        std::string email;
        std::string phone;
    };

    class UserService {
    public:
        // 创建用户
        drogon::Task<dto::view::SysUserView>
        createUser(const CreateUserCommand &command) const;

    private:
        repository::UserRepository userRepository_;
    };
} // namespace project_tracker::modules::user::service
