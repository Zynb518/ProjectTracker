#include "modules/project_member/repository/ProjectMemberRepository.h"

#include <algorithm>
#include <string>

#include <drogon/orm/Exception.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::project_member::repository {
    namespace error = project_tracker::common::error;
    namespace project_domain = modules::project::domain;
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

    drogon::Task<std::optional<ProjectMemberCandidateListResult>>
    ProjectMemberRepository::listProjectMemberCandidates(
        const common::db::SqlExecutorPtr &executor,
        const ProjectMemberCandidateQuery &query) const {
        // 单条 SQL 内同时给出项目存在性、成员管理权限、个人项目限制和候选列表分页结果，
        // 避免把“先判权限再查列表”拆成两条 statement 后出现读快照漂移。
        // CTE 语义：
        // 1. project_context：当前 project_id 的单行项目上下文，负责判断操作者是否有成员管理权限，
        //    以及该项目是否允许执行成员管理（排除普通员工个人自用项目）。
        // 2. filtered：在允许继续的前提下，产出完整候选用户集合；只保留启用状态用户，
        //    按关键字筛选，并排除已经属于当前项目的成员。
        // 3. total：对 filtered 这份完整候选集合计数，给分页返回 total。
        // 4. paged：从 filtered 中按固定排序切出当前页数据，给最终返回 list。

        static const std::string listProjectMemberCandidatesSql = R"SQL(
            WITH project_context AS (
                SELECT
                    p.id AS project_id,
                    CASE
                        WHEN $2 = TRUE THEN TRUE
                        WHEN p.owner_user_id = $3 AND $4 = TRUE THEN TRUE
                        ELSE FALSE
                    END AS has_permission,
                    CASE
                        WHEN creator_user.system_role = 3 THEN FALSE
                        ELSE TRUE
                    END AS member_manage_allowed
                FROM project p
                JOIN sys_user creator_user ON creator_user.id = p.created_by
                WHERE p.id = $1
            ),
            filtered AS (
                SELECT
                    u.id,
                    u.username,
                    u.real_name,
                    u.system_role,
                    u.status
                FROM project_context pc
                JOIN sys_user u ON pc.has_permission AND pc.member_manage_allowed
                WHERE u.status = 1 AND
                    ($5 = '' OR u.username ILIKE $5 OR u.real_name ILIKE $5) AND
                    NOT EXISTS (
                        SELECT 1
                        FROM project_member pm
                        WHERE pm.project_id = pc.project_id AND
                            pm.user_id = u.id
                    )
            ),
            total AS (
                SELECT COUNT(*) AS total
                FROM filtered
            ),
            paged AS (
                SELECT
                    f.id,
                    f.username,
                    f.real_name,
                    f.system_role,
                    f.status
                FROM filtered f
                ORDER BY f.system_role ASC, f.id DESC
                LIMIT $6 OFFSET $7
            )
            SELECT
                pc.has_permission,
                pc.member_manage_allowed,
                t.total,
                p.id,
                p.username,
                p.real_name,
                p.system_role,
                p.status
            FROM project_context pc
            LEFT JOIN total t ON TRUE
            LEFT JOIN paged p ON TRUE
        )SQL";

        const std::int64_t page = std::max(query.page, std::int64_t{1});
        const std::int64_t pageSizeForSql = std::max(query.pageSize, std::int64_t{1});
        const std::int64_t offsetForSql = (page - 1) * pageSizeForSql;
        const std::string keyword = query.keyword.empty() ? "" : "%" + query.keyword + "%";
        const bool isAdmin = query.operatorUserRole == user_domain::SystemRole::Admin;
        const bool isProjectManager = query.operatorUserRole == user_domain::SystemRole::ProjectManager;

        try {
            const auto listResult = co_await executor->execSqlCoro(
                listProjectMemberCandidatesSql,
                query.projectId,
                isAdmin,
                query.operatorUserId,
                isProjectManager,
                keyword,
                pageSizeForSql,
                offsetForSql);

            if (listResult.empty()) {
                co_return std::nullopt;
            }

            ProjectMemberCandidateListResult result{
                .hasPermission = listResult.front()["has_permission"].as<bool>(),
                .memberManageAllowed = listResult.front()["member_manage_allowed"].as<bool>(),
                .page = ProjectMemberCandidatePage{
                    .list = {},
                    .total = listResult.front()["total"].as<std::int64_t>(),
                    .page = page,
                    .pageSize = pageSizeForSql
                }
            };

            if (!result.hasPermission || !result.memberManageAllowed) {
                co_return result;
            }

            result.page.list.reserve(listResult.size());

            for (const auto &row : listResult) {
                if (row["id"].isNull()) {
                    continue;
                }

                result.page.list.push_back(dto::view::ProjectMemberCandidateView{
                    .id = row["id"].as<std::int64_t>(),
                    .username = row["username"].as<std::string>(),
                    .realName = row["real_name"].as<std::string>(),
                    .systemRole = static_cast<user_domain::SystemRole>(row["system_role"].as<int>()),
                    .status = static_cast<user_domain::UserStatus>(row["status"].as<int>()),
                });
            }

            co_return result;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<AddProjectMemberProjectCheckResult>>
    ProjectMemberRepository::findAddProjectMemberProjectCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId) const {
        static const std::string findAddProjectMemberProjectCheckResultForUpdateSql = R"SQL(
            SELECT
                p.owner_user_id,
                creator_user.system_role AS creator_user_role,
                p.status
            FROM project p
            JOIN sys_user creator_user ON creator_user.id = p.created_by
            WHERE p.id = $1
            FOR UPDATE OF p
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findAddProjectMemberProjectCheckResultForUpdateSql,
                projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return AddProjectMemberProjectCheckResult{
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .creatorUserRole = static_cast<user_domain::SystemRole>(
                    row["creator_user_role"].as<int>()),
                .status = static_cast<project_domain::ProjectStatus>(row["status"].as<int>())
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<user::domain::UserStatus>>
    ProjectMemberRepository::findAddProjectMemberTargetUserCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t userId) const {
        static const std::string findAddProjectMemberTargetUserCheckResultForUpdateSql = R"SQL(
            SELECT
                u.status
            FROM sys_user u
            WHERE u.id = $1
            FOR UPDATE OF u
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findAddProjectMemberTargetUserCheckResultForUpdateSql,
                userId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            co_return static_cast<user_domain::UserStatus>(result.front()["status"].as<int>());
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::AddedProjectMemberView>>
    ProjectMemberRepository::insertProjectMember(const common::db::SqlExecutorPtr &executor,
                                                 const dto::command::AddProjectMemberInput &input) const {
        static const std::string insertProjectMemberSql = R"SQL(
            INSERT INTO project_member (
                project_id,
                user_id,
                added_by
            ) VALUES (
                $1,
                $2,
                $3
            )
            ON CONFLICT (project_id, user_id) DO NOTHING
            RETURNING
                project_id,
                user_id,
                to_char(joined_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS joined_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                insertProjectMemberSql,
                input.projectId,
                input.userId,
                input.operatorUserId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return dto::view::AddedProjectMemberView{
                .projectId = row["project_id"].as<std::int64_t>(),
                .userId = row["user_id"].as<std::int64_t>(),
                .joinedAt = row["joined_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }
} // namespace project_tracker::modules::project_member::repository
