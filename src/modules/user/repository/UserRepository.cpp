#include "modules/user/repository/UserRepository.h"

#include <algorithm>

#include <drogon/orm/Exception.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::user::repository {
    namespace error = project_tracker::common::error;

    drogon::Task<user_view::SysUserView>
    UserRepository::insertUser(const common::db::SqlExecutorPtr &executor,
                               const CreateUserRecord &record) const {
        static const std::string insertUserSql = R"SQL(
            INSERT INTO sys_user (
                username,
                password_hash,
                real_name,
                system_role,
                email,
                phone
            ) VALUES (
                $1,
                $2,
                $3,
                $4,
                $5,
                $6
            )
            ON CONFLICT (username) DO NOTHING
            RETURNING
                id,
                username,
                real_name,
                system_role,
                email,
                phone,
                status,
                to_char(created_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS created_at,
                to_char(updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                insertUserSql,
                record.username,
                record.passwordHash,
                record.realName,
                record.systemRole,
                record.email,
                record.phone);

            if (result.empty()) {
                error::throwConflict(
                    error::ErrorCode::UsernameAlreadyExists,
                    "用户名已存在");
            }

            const auto &row = result.front();
            co_return user_view::SysUserView{
                .id = row["id"].as<std::int64_t>(),
                .username = row["username"].as<std::string>(),
                .realName = row["real_name"].as<std::string>(),
                .systemRole = static_cast<domain::SystemRole>(row["system_role"].as<int>()),
                .email = row["email"].as<std::string>(),
                .phone = row["phone"].as<std::string>(),
                .status = static_cast<domain::UserStatus>(row["status"].as<int>()),
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
    UserRepository::findUserById(const common::db::SqlExecutorPtr &executor,
                                 std::int64_t userId) const {
        static const std::string selectUserSql = R"SQL(
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
            const auto result = co_await executor->execSqlCoro(
                selectUserSql,
                userId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return user_view::SysUserView{
                .id = row["id"].as<std::int64_t>(),
                .username = row["username"].as<std::string>(),
                .realName = row["real_name"].as<std::string>(),
                .systemRole = static_cast<domain::SystemRole>(row["system_role"].as<int>()),
                .email = row["email"].as<std::string>(),
                .phone = row["phone"].as<std::string>(),
                .status = static_cast<domain::UserStatus>(row["status"].as<int>()),
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
    UserRepository::updateUserBasicInfo(const common::db::SqlExecutorPtr &executor,
                                        const dto::command::UpdateUserBasicInfoInput &input) const {
        static const std::string updateUserSql = R"SQL(
            UPDATE sys_user
            SET
                real_name = COALESCE($2, real_name),
                system_role = COALESCE($3, system_role),
                email = COALESCE($4, email),
                phone = COALESCE($5, phone),
                updated_at = NOW()
            WHERE id = $1
            RETURNING
                id,
                username,
                real_name,
                system_role,
                email,
                phone,
                status,
                to_char(created_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS created_at,
                to_char(updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                updateUserSql,
                input.userId,
                input.realName,
                input.systemRole,
                input.email,
                input.phone);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return user_view::SysUserView{
                .id = row["id"].as<std::int64_t>(),
                .username = row["username"].as<std::string>(),
                .realName = row["real_name"].as<std::string>(),
                .systemRole = static_cast<domain::SystemRole>(row["system_role"].as<int>()),
                .email = row["email"].as<std::string>(),
                .phone = row["phone"].as<std::string>(),
                .status = static_cast<domain::UserStatus>(row["status"].as<int>()),
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
    UserRepository::updateUserStatus(const common::db::SqlExecutorPtr &executor,
                                     const dto::command::UpdateUserStatusInput &input) const {
        static const std::string updateUserStatusSql = R"SQL(
            UPDATE sys_user
            SET
                status = $2,
                updated_at = NOW()
            WHERE id = $1
            RETURNING
                id,
                username,
                real_name,
                system_role,
                email,
                phone,
                status,
                to_char(created_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS created_at,
                to_char(updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                updateUserStatusSql,
                input.userId,
                domain::toInt(input.status));

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return user_view::SysUserView{
                .id = row["id"].as<std::int64_t>(),
                .username = row["username"].as<std::string>(),
                .realName = row["real_name"].as<std::string>(),
                .systemRole = static_cast<domain::SystemRole>(row["system_role"].as<int>()),
                .email = row["email"].as<std::string>(),
                .phone = row["phone"].as<std::string>(),
                .status = static_cast<domain::UserStatus>(row["status"].as<int>()),
                .createdAt = row["created_at"].as<std::string>(),
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<UserListPage>
    UserRepository::listUsers(const common::db::SqlExecutorPtr &executor,
                              const UserListQuery &query) const {

        // 合并成一条查询
        // 并且 filter 只做筛选，供 total 和 paged 使用
        static const std::string listUsersSql = R"SQL(
            WITH filtered AS (
                SELECT
                    id,
                    username,
                    real_name,
                    system_role,
                    email,
                    phone,
                    status,
                    created_at,
                    updated_at
                FROM sys_user
                WHERE ($1 = '' OR username ILIKE $1 OR real_name ILIKE $1)
                  AND ($2 = 0 OR system_role = $2)
                  AND ($3 = 0 OR status = $3)
            ),
            total AS (
                SELECT COUNT(*) AS total
                FROM filtered
            ),
            paged AS (
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
                FROM filtered
                ORDER BY id DESC
                LIMIT $4 OFFSET $5
            )
            SELECT
                t.total,
                p.id,
                p.username,
                p.real_name,
                p.system_role,
                p.email,
                p.phone,
                p.status,
                p.created_at,
                p.updated_at
            FROM total t
            LEFT JOIN paged p ON TRUE
        )SQL";

        // 先把分页和筛选条件整理成 SQL 可直接使用的参数。
        const std::int64_t page = std::max(query.page, std::int64_t{1});
        const std::int64_t pageSizeForSql = std::max(query.pageSize, std::int64_t{1});
        const std::int64_t offsetForSql = (page - 1) * pageSizeForSql;
        const std::string keyword = query.keyword.empty() ? "" : "%" + query.keyword + "%";
        const int systemRole = query.systemRole.value_or(0);
        const int status = query.status.value_or(0);

        try {
            const auto listResult = co_await executor->execSqlCoro(
                listUsersSql,
                keyword,
                systemRole,
                status,
                pageSizeForSql,
                offsetForSql);

            UserListPage result{
                .list = {},
                .total = listResult.front()["total"].as<std::int64_t>(),
                .page = page,
                .pageSize = pageSizeForSql
            };

            result.list.reserve(listResult.size());

            for (const auto &row : listResult) {
                if (row["id"].isNull()) {
                    continue;
                }

                result.list.push_back(user_view::SysUserView{
                    .id = row["id"].as<std::int64_t>(),
                    .username = row["username"].as<std::string>(),
                    .realName = row["real_name"].as<std::string>(),
                    .systemRole = static_cast<domain::SystemRole>(row["system_role"].as<int>()),
                    .email = row["email"].as<std::string>(),
                    .phone = row["phone"].as<std::string>(),
                    .status = static_cast<domain::UserStatus>(row["status"].as<int>()),
                    .createdAt = row["created_at"].as<std::string>(),
                    .updatedAt = row["updated_at"].as<std::string>()
                });
            }

            co_return result;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }
} // namespace project_tracker::modules::user::repository
