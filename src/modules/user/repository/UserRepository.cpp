#include "modules/user/repository/UserRepository.h"

#include <algorithm>
#include <string_view>

#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::user::repository {
    namespace error = project_tracker::common::error;

    drogon::Task<UserListPage>
    UserRepository::listUsers(const UserListQuery &query) const {
        constexpr std::string_view countUsersSql = R"SQL(
            SELECT COUNT(*) AS total
            FROM sys_user
            WHERE ($1 = '' OR username ILIKE $1 OR real_name ILIKE $1)
              AND ($2 = 0 OR system_role = $2)
              AND ($3 = 0 OR status = $3)
        )SQL";

        constexpr std::string_view selectUsersSql = R"SQL(
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
            WHERE ($1 = '' OR username ILIKE $1 OR real_name ILIKE $1)
              AND ($2 = 0 OR system_role = $2)
              AND ($3 = 0 OR status = $3)
            ORDER BY id DESC
            LIMIT $4 OFFSET $5
        )SQL";

        // 先把分页和筛选条件整理成 SQL 可直接使用的参数。
        const std::int64_t page = std::max(query.page, std::int64_t{1});
        const std::int64_t pageSizeForSql = std::max(query.pageSize, std::int64_t{1});
        const std::int64_t offsetForSql = (page - 1) * pageSizeForSql;
        const std::string keyword = query.keyword.empty() ? "" : "%" + query.keyword + "%";
        const int systemRole = query.systemRole.value_or(0);
        const int status = query.status.value_or(0);

        try {
            const auto dbClient = drogon::app().getDbClient();

            // 先查总数，再查当前页数据。
            const auto totalResult = co_await dbClient->execSqlCoro(
                std::string(countUsersSql),
                keyword,
                systemRole,
                status);

            const auto listResult = co_await dbClient->execSqlCoro(
                std::string(selectUsersSql),
                keyword,
                systemRole,
                status,
                pageSizeForSql,
                offsetForSql);

            UserListPage result{
                .list = {},
                .total = totalResult.front()["total"].as<std::int64_t>(),
                .page = page,
                .pageSize = pageSizeForSql
            };

            result.list.reserve(listResult.size());

            for (const auto &row : listResult) {
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
