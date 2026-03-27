#include "modules/auth/repository/AuthRepository.h"

#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>
#include <drogon/orm/Row.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::auth::repository {
    namespace error = project_tracker::common::error;

    drogon::Task<std::optional<AuthUserRecord>>
    AuthRepository::findUserByUsername(const std::string &username) const {
        constexpr std::string_view selectUserSql = R"SQL(
            SELECT
                id,
                username,
                password_hash,
                real_name,
                system_role,
                email,
                phone,
                status,
                to_char(created_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS created_at,
                to_char(updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
            FROM sys_user
            WHERE username = $1
            LIMIT 1
        )SQL";

        try {
            const auto dbClient = drogon::app().getDbClient();
            const auto result = co_await dbClient->execSqlCoro(
                std::string(selectUserSql),
                username);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();

            co_return AuthUserRecord{
                .id = row["id"].as<long long>(),
                .username = row["username"].as<std::string>(),
                .passwordHash = row["password_hash"].as<std::string>(),
                .realName = row["real_name"].as<std::string>(),
                .systemRole = static_cast<user_domain::SystemRole>(row["system_role"].as<int>()),
                .email = row["email"].as<std::string>(),
                .phone = row["phone"].as<std::string>(),
                .status = static_cast<user_domain::UserStatus>(row["status"].as<int>()),
                .createdAt = row["created_at"].as<std::string>(),
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<user_view::SysUserView>>
    AuthRepository::findUserById(std::int64_t userId) const {
        constexpr std::string_view selectUserSql = R"SQL(
            SELECT
                id,
                username,
                real_name,
                system_role,
                email,
                phone,
                status,
                to_char(created_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS created_at,
                to_char(updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
            FROM sys_user
            WHERE id = $1
            LIMIT 1
        )SQL";

        try {
            const auto dbClient = drogon::app().getDbClient();
            const auto result = co_await dbClient->execSqlCoro(
                std::string(selectUserSql),
                userId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();

            co_return user_view::SysUserView{
                .id = row["id"].as<long long>(),
                .username = row["username"].as<std::string>(),
                .realName = row["real_name"].as<std::string>(),
                .systemRole = static_cast<user_domain::SystemRole>(row["system_role"].as<int>()),
                .email = row["email"].as<std::string>(),
                .phone = row["phone"].as<std::string>(),
                .status = static_cast<user_domain::UserStatus>(row["status"].as<int>()),
                .createdAt = row["created_at"].as<std::string>(),
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }
} // namespace project_tracker::modules::auth::repository
