#include "modules/auth/service/AuthService.h"

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"
#include "common/util/PasswordUtil.h"

namespace project_tracker::modules::auth::service {
    namespace error = project_tracker::common::error;
    namespace util = project_tracker::common::util;
    namespace user_domain = modules::user::domain;

    namespace {
        user_view::SysUserView buildUserView(const repository::AuthUserRecord &record) {
            return {
                .id = record.id,
                .username = record.username,
                .realName = record.realName,
                .systemRole = record.systemRole,
                .email = record.email,
                .phone = record.phone,
                .status = record.status,
                .createdAt = record.createdAt,
                .updatedAt = record.updatedAt
            };
        }
    } // namespace

    drogon::Task<user_view::SysUserView>
    AuthService::login(const std::string &username,
                       const std::string &password) const {
        const auto user = co_await authRepository_.findUserByUsername(username);
        if (!user) {
            error::throwUnauthorized(
                error::ErrorCode::LoginFailed,
                "用户名或密码错误");
        }

        const auto verifyResult = util::verifyPassword(password, user->passwordHash);
        if (verifyResult == util::PasswordVerifyResult::Mismatched) {
            error::throwUnauthorized(
                error::ErrorCode::LoginFailed,
                "用户名或密码错误");
        }

        if (verifyResult == util::PasswordVerifyResult::Failed) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "密码哈希校验失败");
        }

        if (user->status == user_domain::UserStatus::Disabled) {
            error::throwConflict(
                error::ErrorCode::UserDisabled,
                "用户已被禁用");
        }

        co_return buildUserView(*user);
    }
} // namespace project_tracker::modules::auth::service
