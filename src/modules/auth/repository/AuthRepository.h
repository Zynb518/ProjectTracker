#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <drogon/utils/coroutine.h>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::auth::repository {
    namespace user_domain = modules::user::domain;

    // 登录查询使用的用户记录
    // 说明：
    // 1. 包含 password_hash，供 AuthService 做密码校验
    // 2. 不直接作为接口返回对象使用
    struct AuthUserRecord {
        std::int64_t id;
        std::string username;
        std::string passwordHash;
        std::string realName;
        user_domain::SystemRole systemRole;
        std::string email;
        std::string phone;
        user_domain::UserStatus status;
        std::string createdAt;
        std::string updatedAt;
    };

    class AuthRepository {
    public:
        drogon::Task<std::optional<AuthUserRecord>>
        findUserByUsername(const std::string &username) const;
    };
} // namespace project_tracker::modules::auth::repository
