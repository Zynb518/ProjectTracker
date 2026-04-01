#include "modules/project_member/repository/ProjectMemberRepository.h"

#include <string>

#include <drogon/orm/Exception.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::project_member::repository {
    namespace error = project_tracker::common::error;
    namespace user_domain = modules::user::domain;

    drogon::Task<std::optional<ProjectMemberListResult>>
    ProjectMemberRepository::listProjectMembers(const common::db::SqlExecutorPtr &executor,
                                                const ProjectMemberListQuery &query) const {
        // 单条 SQL 内先构造项目上下文，再按同一条 statement 的读快照决定：
        // 1. 查不到 project_context 行：项目不存在，repository 返回 nullopt。
        // 2. 查得到但 has_permission = false：只返回一行权限结果，不泄露成员明细。
        // 3. 查得到且 has_permission = true：返回该项目全部成员，并标记谁是负责人。
        static const std::string listProjectMembersSql = R"SQL(
            WITH project_context AS (
                SELECT
                    p.id AS project_id,
                    p.owner_user_id,
                    CASE
                        WHEN $2 = TRUE THEN TRUE
                        WHEN EXISTS (
                            SELECT 1
                            FROM project_member self_pm
                            WHERE self_pm.project_id = p.id AND
                                self_pm.user_id = $3
                        ) THEN TRUE
                        ELSE FALSE
                    END AS has_permission
                FROM project p
                WHERE p.id = $1
            )
            SELECT
                pc.has_permission,
                pm.project_id,
                pm.user_id,
                u.username,
                u.real_name,
                u.system_role,
                u.status,
                to_char(pm.joined_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS joined_at,
                (pc.owner_user_id = pm.user_id) AS is_owner
            FROM project_context pc
            LEFT JOIN project_member pm ON pm.project_id = pc.project_id AND
                pc.has_permission
            LEFT JOIN sys_user u ON u.id = pm.user_id
            ORDER BY pm.joined_at ASC NULLS LAST, pm.id ASC NULLS LAST
        )SQL";

        const bool isAdmin = query.currentUserRole == user_domain::SystemRole::Admin;

        try {
            const auto listResult = co_await executor->execSqlCoro(
                listProjectMembersSql,
                query.projectId,
                isAdmin,
                query.currentUserId);

            if (listResult.empty()) {
                co_return std::nullopt;
            }

            ProjectMemberListResult result{
                .hasPermission = listResult.front()["has_permission"].as<bool>(),
                .list = {}
            };

            if (!result.hasPermission) {
                co_return result;
            }

            result.list.reserve(listResult.size());

            for (const auto &row : listResult) {
                if (row["user_id"].isNull()) {
                    continue;
                }

                result.list.push_back(dto::view::ProjectMemberListItemView{
                    .projectId = row["project_id"].as<std::int64_t>(),
                    .userId = row["user_id"].as<std::int64_t>(),
                    .username = row["username"].as<std::string>(),
                    .realName = row["real_name"].as<std::string>(),
                    .systemRole = static_cast<user_domain::SystemRole>(row["system_role"].as<int>()),
                    .status = static_cast<user_domain::UserStatus>(row["status"].as<int>()),
                    .joinedAt = row["joined_at"].as<std::string>(),
                    .isOwner = row["is_owner"].as<bool>()
                });
            }

            co_return result;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }
} // namespace project_tracker::modules::project_member::repository
