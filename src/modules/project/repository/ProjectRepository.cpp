#include "modules/project/repository/ProjectRepository.h"

#include <algorithm>
#include <string>

#include <drogon/orm/Exception.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::project::repository {
    namespace error = project_tracker::common::error;
    namespace project_node_domain = modules::project_node::domain;
    namespace task_domain = modules::task::domain;
    namespace user_domain = modules::user::domain;

    drogon::Task<dto::view::CreatedProjectView>
    ProjectRepository::insertProject(const common::db::SqlExecutorPtr &executor,
                                     const dto::command::CreateProjectInput &input) const {
        static const std::string insertProjectSql = R"SQL(
            INSERT INTO project (
                name,
                description,
                owner_user_id,
                status,
                planned_start_date,
                planned_end_date,
                created_by
            ) VALUES (
                $1,
                $2,
                $3,
                1,
                $4,
                $5,
                $6
            )
            RETURNING
                id,
                name,
                description,
                owner_user_id,
                status,
                to_char(planned_start_date, 'YYYY-MM-DD') AS planned_start_date,
                to_char(planned_end_date, 'YYYY-MM-DD') AS planned_end_date,
                to_char(completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS completed_at,
                created_by,
                to_char(created_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS created_at,
                to_char(updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                insertProjectSql,
                input.name,
                input.description,
                input.creatorUserId,
                input.plannedStartDate,
                input.plannedEndDate,
                input.creatorUserId);

            const auto &row = result.front();

            std::optional<std::string> completedAt;
            if (!row["completed_at"].isNull()) {
                completedAt = row["completed_at"].as<std::string>();
            }

            co_return dto::view::CreatedProjectView{
                .id = row["id"].as<std::int64_t>(),
                .name = row["name"].as<std::string>(),
                .description = row["description"].as<std::string>(),
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .status = static_cast<domain::ProjectStatus>(row["status"].as<int>()),
                .plannedStartDate = row["planned_start_date"].as<std::string>(),
                .plannedEndDate = row["planned_end_date"].as<std::string>(),
                .completedAt = completedAt,
                .createdBy = row["created_by"].as<std::int64_t>(),
                .createdAt = row["created_at"].as<std::string>(),
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<void>
    ProjectRepository::insertProjectMember(const common::db::SqlExecutorPtr &executor,
                                           std::int64_t projectId,
                                           std::int64_t userId,
                                           std::int64_t addedBy) const {
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
        )SQL";

        try {
            co_await executor->execSqlCoro(
                insertProjectMemberSql,
                projectId,
                userId,
                addedBy);
            co_return;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<void>
    ProjectRepository::ensureProjectMemberExists(const common::db::SqlExecutorPtr &executor,
                                                 std::int64_t projectId,
                                                 std::int64_t userId,
                                                 std::int64_t addedBy) const {
        static const std::string ensureProjectMemberExistsSql = R"SQL(
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
        )SQL";

        try {
            co_await executor->execSqlCoro(
                ensureProjectMemberExistsSql,
                projectId,
                userId,
                addedBy);
            co_return;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::UpdatedProjectBasicInfoView> >
    ProjectRepository::updateProjectBasicInfo(
        const common::db::SqlExecutorPtr &executor,
        const dto::command::UpdateProjectBasicInfoInput &input) const {
        // 查询思路：
        // 1. target_context 先算出项目本次更新后的有效名称、说明、起止日期。
        // 2. node_range 单独聚合当前项目下全部节点的最小开始日期和最大结束日期。
        // 3. 最后只在“项目日期合法，且仍能覆盖全部阶段节点”时执行更新；
        //    同时按最终 planned_end_date 重算未完成项目状态：未开始保持未开始，已开始项目只在进行中/已延期之间切换。
        static const std::string updateProjectBasicInfoSql = R"SQL(
            WITH target_context AS (
                SELECT
                    p.id,
                    COALESCE($2, p.name) AS next_name,
                    COALESCE($3, p.description) AS next_description,
                    COALESCE($4::date, p.planned_start_date) AS next_planned_start_date,
                    COALESCE($5::date, p.planned_end_date) AS next_planned_end_date
                FROM project p
                WHERE p.id = $1
            ),
            node_range AS (
                SELECT
                    pn.project_id,
                    MIN(pn.planned_start_date) AS min_planned_start_date,
                    MAX(pn.planned_end_date) AS max_planned_end_date
                FROM project_node pn
                WHERE pn.project_id = $1
                GROUP BY pn.project_id
            )
            UPDATE project p
            SET
                name = tc.next_name,
                description = tc.next_description,
                planned_start_date = tc.next_planned_start_date,
                planned_end_date = tc.next_planned_end_date,
                status = CASE
                    WHEN p.status = 1 THEN 1
                    WHEN p.status = 3 THEN 3
                    WHEN (NOW() AT TIME ZONE 'Asia/Shanghai')::date >
                         tc.next_planned_end_date THEN 4
                    ELSE 2
                END,
                updated_at = NOW()
            FROM target_context tc
            LEFT JOIN node_range nr ON nr.project_id = tc.id
            WHERE p.id = tc.id AND
                tc.next_planned_start_date <= tc.next_planned_end_date AND
                (
                    nr.project_id IS NULL OR
                    (
                        tc.next_planned_start_date <= nr.min_planned_start_date AND
                        tc.next_planned_end_date >= nr.max_planned_end_date
                    )
                )
            RETURNING
                p.id,
                p.name,
                p.description,
                p.status,
                to_char(p.planned_start_date, 'YYYY-MM-DD') AS planned_start_date,
                to_char(p.planned_end_date, 'YYYY-MM-DD') AS planned_end_date,
                to_char(p.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                updateProjectBasicInfoSql,
                input.projectId,
                input.name,
                input.description,
                input.plannedStartDate,
                input.plannedEndDate);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return dto::view::UpdatedProjectBasicInfoView{
                .id = row["id"].as<std::int64_t>(),
                .name = row["name"].as<std::string>(),
                .description = row["description"].as<std::string>(),
                .status = static_cast<domain::ProjectStatus>(row["status"].as<int>()),
                .plannedStartDate = row["planned_start_date"].as<std::string>(),
                .plannedEndDate = row["planned_end_date"].as<std::string>(),
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<ProjectBasicInfoUpdateCheckResult>>
    ProjectRepository::findProjectBasicInfoUpdateCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId) const {
        static const std::string findProjectBasicInfoUpdateCheckResultForUpdateSql = R"SQL(
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
                findProjectBasicInfoUpdateCheckResultForUpdateSql,
                projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return ProjectBasicInfoUpdateCheckResult{
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .creatorUserRole = static_cast<user_domain::SystemRole>(
                    row["creator_user_role"].as<int>()),
                .status = static_cast<domain::ProjectStatus>(row["status"].as<int>())
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<std::int64_t>>
    ProjectRepository::findProjectDeleteCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId) const {
        static const std::string findProjectDeleteCheckResultForUpdateSql = R"SQL(
            SELECT
                p.owner_user_id
            FROM project p
            WHERE p.id = $1
            FOR UPDATE OF p
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findProjectDeleteCheckResultForUpdateSql,
                projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            co_return result.front()["owner_user_id"].as<std::int64_t>();
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<ProjectStartCheckResult>>
    ProjectRepository::findProjectStartCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId) const {
        static const std::string findProjectStartCheckResultForUpdateSql = R"SQL(
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
                findProjectStartCheckResultForUpdateSql,
                projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return ProjectStartCheckResult{
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .creatorUserRole = static_cast<user_domain::SystemRole>(
                    row["creator_user_role"].as<int>()),
                .status = static_cast<domain::ProjectStatus>(row["status"].as<int>())
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::UpdatedProjectStatusView> >
    ProjectRepository::updateProjectStatusForStart(const common::db::SqlExecutorPtr &executor,
                                                   std::int64_t projectId) const {
        // -- 按北京时间取“今天”，再和 DATE 类型的 planned_end_date 比较是否已延期。
        static const std::string updateProjectStatusForStartSql = R"SQL(
            UPDATE project
            SET
                status = CASE
                    WHEN (NOW() AT TIME ZONE 'Asia/Shanghai')::date > planned_end_date THEN 4
                    ELSE 2
                END,
                completed_at = NULL,
                updated_at = NOW()
            WHERE id = $1 AND
                status = 1 AND
                NOT EXISTS (
                    SELECT 1
                    FROM project_node pn
                    WHERE pn.project_id = project.id
                )
            RETURNING
                id,
                status,
                to_char(completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS completed_at,
                to_char(updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                updateProjectStatusForStartSql,
                projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();

            std::optional<std::string> completedAt;
            if (!row["completed_at"].isNull()) {
                completedAt = row["completed_at"].as<std::string>();
            }

            co_return dto::view::UpdatedProjectStatusView{
                .id = row["id"].as<std::int64_t>(),
                .status = static_cast<domain::ProjectStatus>(row["status"].as<int>()),
                .completedAt = completedAt,
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::UpdatedProjectStatusView>>
    ProjectRepository::updateProjectStatusForNodeSignal(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId) const {
        // -- 项目行已在调用方事务中锁定；这里只把仍为未开始的项目推进到项目自己的已开始态。
        static const std::string updateProjectStatusForNodeSignalSql = R"SQL(
            UPDATE project
            SET
                status = CASE
                    WHEN (NOW() AT TIME ZONE 'Asia/Shanghai')::date > planned_end_date THEN 4
                    ELSE 2
                END,
                completed_at = NULL,
                updated_at = NOW()
            WHERE id = $1 AND
                status = 1
            RETURNING
                id,
                status,
                to_char(completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS completed_at,
                to_char(updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                updateProjectStatusForNodeSignalSql,
                projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();

            std::optional<std::string> completedAt;
            if (!row["completed_at"].isNull()) {
                completedAt = row["completed_at"].as<std::string>();
            }

            co_return dto::view::UpdatedProjectStatusView{
                .id = row["id"].as<std::int64_t>(),
                .status = static_cast<domain::ProjectStatus>(row["status"].as<int>()),
                .completedAt = completedAt,
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<ProjectCompleteCheckResult>>
    ProjectRepository::findProjectCompleteCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId) const {
        static const std::string findProjectCompleteCheckResultForUpdateSql = R"SQL(
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
                findProjectCompleteCheckResultForUpdateSql,
                projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return ProjectCompleteCheckResult{
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .creatorUserRole = static_cast<user_domain::SystemRole>(row["creator_user_role"].as<int>()),
                .status = static_cast<domain::ProjectStatus>(row["status"].as<int>())
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::UpdatedProjectStatusView> >
    ProjectRepository::updateProjectStatusForComplete(const common::db::SqlExecutorPtr &executor,
                                                      std::int64_t projectId) const {
        static const std::string updateProjectStatusForCompleteSql = R"SQL(
            UPDATE project
            SET
                status = 3,
                completed_at = NOW(),
                updated_at = NOW()
            WHERE id = $1 AND
                status <> 3 AND
                (
                    NOT EXISTS (
                        SELECT 1
                        FROM project_node pn
                        WHERE pn.project_id = project.id
                    ) OR
                    NOT EXISTS (
                        SELECT 1
                        FROM project_node pn
                        WHERE pn.project_id = project.id AND
                            pn.status <> 3
                    )
                )
            RETURNING
                id,
                status,
                to_char(completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS completed_at,
                to_char(updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                updateProjectStatusForCompleteSql,
                projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();

            std::optional<std::string> completedAt;
            if (!row["completed_at"].isNull()) {
                completedAt = row["completed_at"].as<std::string>();
            }

            co_return dto::view::UpdatedProjectStatusView{
                .id = row["id"].as<std::int64_t>(),
                .status = static_cast<domain::ProjectStatus>(row["status"].as<int>()),
                .completedAt = completedAt,
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<ProjectReopenCheckResult>>
    ProjectRepository::findProjectReopenCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId) const {
        static const std::string findProjectReopenCheckResultForUpdateSql = R"SQL(
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
                findProjectReopenCheckResultForUpdateSql,
                projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return ProjectReopenCheckResult{
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .creatorUserRole = static_cast<user_domain::SystemRole>(
                    row["creator_user_role"].as<int>()),
                .status = static_cast<domain::ProjectStatus>(row["status"].as<int>())
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<ProjectOwnerTransferProjectCheckResult>>
    ProjectRepository::findProjectOwnerTransferProjectCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId) const {
        static const std::string findProjectOwnerTransferProjectCheckResultForUpdateSql = R"SQL(
            SELECT
                p.owner_user_id AS previous_owner_user_id,
                creator_user.system_role AS creator_user_role
            FROM project p
            JOIN sys_user creator_user ON creator_user.id = p.created_by
            WHERE p.id = $1
            FOR UPDATE OF p
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findProjectOwnerTransferProjectCheckResultForUpdateSql,
                projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return ProjectOwnerTransferProjectCheckResult{
                .previousOwnerUserId = row["previous_owner_user_id"].as<std::int64_t>(),
                .creatorUserRole = static_cast<user_domain::SystemRole>(row["creator_user_role"].as<int>())
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<TransferTargetUserCheckResult>>
    ProjectRepository::findTransferTargetUserCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId,
        std::int64_t targetUserId) const {
        static const std::string findTransferTargetUserCheckResultForUpdateSql = R"SQL(
            SELECT
                u.system_role AS target_user_role,
                u.status AS target_user_status,
                EXISTS (
                    SELECT 1
                    FROM project_member pm
                    WHERE pm.project_id = $1 AND
                        pm.user_id = u.id
                ) AS target_is_project_member
            FROM sys_user u
            WHERE u.id = $2
            FOR UPDATE OF u
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findTransferTargetUserCheckResultForUpdateSql,
                projectId,
                targetUserId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return TransferTargetUserCheckResult{
                .targetUserRole = static_cast<user_domain::SystemRole>(row["target_user_role"].as<int>()),
                .targetUserStatus = static_cast<user_domain::UserStatus>(row["target_user_status"].as<int>()),
                .targetIsProjectMember = row["target_is_project_member"].as<bool>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::UpdatedProjectStatusView>>
    ProjectRepository::updateProjectStatusForReopen(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId) const {

        // 按北京时间先判断是否已延期；当前 schema 没有项目级开始信号历史，
        // 这里对已完成项目按“已存在开始信号”回退到进行中/已延期。
        static const std::string updateProjectStatusForReopenSql = R"SQL(
            UPDATE project
            SET
                status = CASE
                    WHEN (NOW() AT TIME ZONE 'Asia/Shanghai')::date > planned_end_date THEN 4
                    ELSE 2
                END,
                completed_at = NULL,
                updated_at = NOW()
            WHERE id = $1 AND
                status = 3
            RETURNING
                id,
                status,
                to_char(completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS completed_at,
                to_char(updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                updateProjectStatusForReopenSql,
                projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();

            std::optional<std::string> completedAt;
            if (!row["completed_at"].isNull()) {
                completedAt = row["completed_at"].as<std::string>();
            }

            co_return dto::view::UpdatedProjectStatusView{
                .id = row["id"].as<std::int64_t>(),
                .status = static_cast<domain::ProjectStatus>(row["status"].as<int>()),
                .completedAt = completedAt,
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::TransferredProjectOwnerView>>
    ProjectRepository::updateProjectOwner(const common::db::SqlExecutorPtr &executor,
                                          const dto::command::TransferProjectOwnerInput &input) const {
        static const std::string updateProjectOwnerSql = R"SQL(
            WITH locked_project AS (
                SELECT
                    p.id,
                    p.owner_user_id
                FROM project p
                WHERE p.id = $1
                FOR UPDATE
            ),
            updated AS (
                UPDATE project p
                SET
                    owner_user_id = $2,
                    updated_at = NOW()
                FROM locked_project lp
                WHERE p.id = lp.id
                RETURNING
                    p.id AS project_id,
                    lp.owner_user_id AS previous_owner_user_id,
                    p.owner_user_id,
                    p.updated_at
            )
            SELECT
                u.project_id,
                u.previous_owner_user_id,
                u.owner_user_id,
                target_user.real_name AS owner_real_name,
                to_char(u.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD\"T\"HH24:MI:SS') || '+08:00' AS updated_at
            FROM updated u
            JOIN sys_user target_user ON target_user.id = u.owner_user_id
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                updateProjectOwnerSql,
                input.projectId,
                input.targetUserId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return dto::view::TransferredProjectOwnerView{
                .projectId = row["project_id"].as<std::int64_t>(),
                .previousOwnerUserId = row["previous_owner_user_id"].as<std::int64_t>(),
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .ownerRealName = row["owner_real_name"].as<std::string>(),
                .autoAddedAsMember = false,
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<bool>
    ProjectRepository::deleteProject(const common::db::SqlExecutorPtr &executor,
                                     std::int64_t projectId) const {
        static const std::string deleteProjectSql = R"SQL(
            DELETE FROM project
            WHERE id = $1
            RETURNING id
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                deleteProjectSql,
                projectId);

            co_return !result.empty();
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<ProjectListPage>
    ProjectRepository::listProjects(const common::db::SqlExecutorPtr &executor,
                                    const ProjectListQuery &query) const {
        // 1.先进行筛选（项目名，状态，拥有者）
        //   再根据是否是管理员，拥有者，项目成员
        // 2.再根据filter中间结果，求 total, paged
        static const std::string listProjectsSql = R"SQL(
            WITH filtered AS (
                SELECT
                    p.id,
                    p.name,
                    p.description,
                    p.owner_user_id,
                    owner_user.real_name AS owner_real_name,
                    p.status,
                    p.planned_start_date,
                    p.planned_end_date,
                    p.completed_at,
                    p.created_at,
                    p.updated_at
                FROM project p
                JOIN sys_user owner_user ON owner_user.id = p.owner_user_id
                WHERE ($1 = '' OR p.name ILIKE $1) AND
                    ($2 = 0 OR p.status = $2) AND
                    ($3 = 0::bigint OR p.owner_user_id = $3) AND
                    (
                        $4 = TRUE OR
                        p.owner_user_id = $5 OR
                        EXISTS (
                            SELECT 1
                            FROM project_member pm
                            WHERE pm.project_id = p.id AND
                                pm.user_id = $5
                        )
                    )
            ),
            total AS (
                SELECT COUNT(*) AS total
                FROM filtered
            ),
            paged AS (
                SELECT
                    f.id,
                    f.name,
                    f.description,
                    f.owner_user_id,
                    f.owner_real_name,
                    f.status,
                    to_char(f.planned_start_date, 'YYYY-MM-DD') AS planned_start_date,
                    to_char(f.planned_end_date, 'YYYY-MM-DD') AS planned_end_date,
                    to_char(f.completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS completed_at,
                    (
                        SELECT COUNT(*)
                        FROM project_member pm
                        WHERE pm.project_id = f.id
                    ) AS member_count,
                    (
                        SELECT COUNT(*)
                        FROM project_node pn
                        WHERE pn.project_id = f.id
                    ) AS node_count,
                    (
                        SELECT COUNT(*)
                        FROM sub_task st
                        JOIN project_node pn ON pn.id = st.node_id
                        WHERE pn.project_id = f.id
                    ) AS sub_task_count,
                    to_char(f.created_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS created_at,
                    to_char(f.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at,
                    (f.owner_user_id = $5) AS is_owner
                FROM filtered f
                ORDER BY f.updated_at DESC, f.id DESC
                LIMIT $6 OFFSET $7
            )
            SELECT
                t.total,
                p.id,
                p.name,
                p.description,
                p.owner_user_id,
                p.owner_real_name,
                p.status,
                p.planned_start_date,
                p.planned_end_date,
                p.completed_at,
                p.member_count,
                p.node_count,
                p.sub_task_count,
                p.created_at,
                p.updated_at,
                p.is_owner
            FROM total t
            LEFT JOIN paged p ON TRUE
        )SQL";

        /*
        *   - 有分页数据时：
              返回多行，每行都带同一个 total
            - 没分页数据时：
              返回一行，total 有值，但项目列全是 NULL
         */

        const std::int64_t page = std::max(query.page, std::int64_t{1});
        const std::int64_t pageSizeForSql = std::max(query.pageSize, std::int64_t{1});
        const std::int64_t offsetForSql = (page - 1) * pageSizeForSql;
        const std::string keyword = query.keyword.empty() ? "" : "%" + query.keyword + "%";
        const int status = query.status ? domain::toInt(*query.status) : 0;
        const std::int64_t ownerUserId = query.ownerUserId.value_or(0);
        const bool isAdmin = query.currentUserRole == user_domain::SystemRole::Admin;

        try {
            const auto listResult = co_await executor->execSqlCoro(
                listProjectsSql,
                keyword,
                status,
                ownerUserId,
                isAdmin,
                query.currentUserId,
                pageSizeForSql,
                offsetForSql);

            ProjectListPage result{
                .list = {},
                .total = listResult.front()["total"].as<std::int64_t>(),
                .page = page,
                .pageSize = pageSizeForSql
            };

            result.list.reserve(listResult.size());

            for (const auto &row: listResult) {
                if (row["id"].isNull()) {
                    continue;
                }

                std::optional<std::string> completedAt;
                if (!row["completed_at"].isNull()) {
                    completedAt = row["completed_at"].as<std::string>();
                }

                result.list.push_back(dto::view::ProjectListItemView{
                    .id = row["id"].as<std::int64_t>(),
                    .name = row["name"].as<std::string>(),
                    .description = row["description"].as<std::string>(),
                    .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                    .ownerRealName = row["owner_real_name"].as<std::string>(),
                    .status = static_cast<domain::ProjectStatus>(row["status"].as<int>()),
                    .plannedStartDate = row["planned_start_date"].as<std::string>(),
                    .plannedEndDate = row["planned_end_date"].as<std::string>(),
                    .completedAt = completedAt,
                    .memberCount = row["member_count"].as<std::int64_t>(),
                    .nodeCount = row["node_count"].as<std::int64_t>(),
                    .subTaskCount = row["sub_task_count"].as<std::int64_t>(),
                    .createdAt = row["created_at"].as<std::string>(),
                    .updatedAt = row["updated_at"].as<std::string>(),
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

    drogon::Task<ProjectOwnerCandidateListResult>
    ProjectRepository::listProjectOwnerCandidates(const common::db::SqlExecutorPtr &executor,
                                                  const ProjectOwnerCandidateQuery &query) const {
        static const std::string listProjectOwnerCandidatesSql = R"SQL(
            WITH project_context AS (
                SELECT
                    (p.id IS NOT NULL) AS project_exists,
                    COALESCE(p.owner_user_id, 0::bigint) AS owner_user_id,
                    CASE
                        WHEN p.id IS NULL THEN FALSE
                        WHEN creator_user.system_role = 3 THEN FALSE
                        ELSE TRUE
                    END AS owner_transfer_allowed,
                    CASE
                        WHEN p.id IS NULL THEN FALSE
                        WHEN $2 = TRUE THEN TRUE
                        ELSE p.owner_user_id = $3
                    END AS has_permission
                FROM (SELECT 1) anchor
                LEFT JOIN project p ON p.id = $1
                LEFT JOIN sys_user creator_user ON creator_user.id = p.created_by
            ),
            filtered AS (
                SELECT
                    u.id,
                    u.username,
                    u.real_name,
                    u.system_role,
                    u.status,
                    EXISTS (
                        SELECT 1
                        FROM project_member pm
                        WHERE pm.project_id = $1 AND
                            pm.user_id = u.id
                    ) AS is_project_member
                FROM project_context pc
                JOIN sys_user u ON pc.project_exists AND pc.has_permission AND pc.owner_transfer_allowed
                WHERE u.status = 1 AND
                    (
                        ($2 = TRUE AND u.system_role IN (1, 2)) OR
                        ($2 = FALSE AND u.system_role = 2)
                    ) AND
                    ($4 = '' OR u.username ILIKE $4 OR u.real_name ILIKE $4) AND
                    u.id <> pc.owner_user_id
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
                    f.status,
                    f.is_project_member
                FROM filtered f
                ORDER BY f.system_role ASC, f.id DESC
                LIMIT $5 OFFSET $6
            )
            SELECT
                pc.project_exists,
                pc.has_permission,
                pc.owner_transfer_allowed,
                t.total,
                p.id,
                p.username,
                p.real_name,
                p.system_role,
                p.status,
                p.is_project_member
            FROM project_context pc
            LEFT JOIN total t ON TRUE
            LEFT JOIN paged p ON TRUE
        )SQL";

        const std::int64_t page = std::max(query.page, std::int64_t{1});
        const std::int64_t pageSizeForSql = std::max(query.pageSize, std::int64_t{1});
        const std::int64_t offsetForSql = (page - 1) * pageSizeForSql;
        const std::string keyword = query.keyword.empty() ? "" : "%" + query.keyword + "%";

        try {
            const auto listResult = co_await executor->execSqlCoro(
                listProjectOwnerCandidatesSql,
                query.projectId,
                query.includeAdminCandidates,
                query.operatorUserId,
                keyword,
                pageSizeForSql,
                offsetForSql);

            ProjectOwnerCandidateListResult result{
                .projectExists = listResult.front()["project_exists"].as<bool>(),
                .hasPermission = listResult.front()["has_permission"].as<bool>(),
                .ownerTransferAllowed = listResult.front()["owner_transfer_allowed"].as<bool>(),
                .page = ProjectOwnerCandidatePage{
                    .list = {},
                    .total = listResult.front()["total"].as<std::int64_t>(),
                    .page = page,
                    .pageSize = pageSizeForSql
                }
            };

            result.page.list.reserve(listResult.size());

            for (const auto &row : listResult) {
                if (row["id"].isNull()) {
                    continue;
                }

                result.page.list.push_back(dto::view::ProjectOwnerCandidateView{
                    .id = row["id"].as<std::int64_t>(),
                    .username = row["username"].as<std::string>(),
                    .realName = row["real_name"].as<std::string>(),
                    .systemRole = static_cast<user_domain::SystemRole>(row["system_role"].as<int>()),
                    .status = static_cast<user_domain::UserStatus>(row["status"].as<int>()),
                    .isProjectMember = row["is_project_member"].as<bool>()
                });
            }

            co_return result;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::ProjectDetailView> >
    ProjectRepository::findProjectDetail(const common::db::SqlExecutorPtr &executor,
                                         std::int64_t projectId,
                                         std::int64_t currentUserId,
                                         user::domain::SystemRole currentUserRole) const {
        static const std::string findProjectDetailSql = R"SQL(
            SELECT
                p.id,
                p.name,
                p.description,
                p.owner_user_id,
                owner_user.real_name AS owner_real_name,
                p.status,
                to_char(p.planned_start_date, 'YYYY-MM-DD') AS planned_start_date,
                to_char(p.planned_end_date, 'YYYY-MM-DD') AS planned_end_date,
                to_char(p.completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS completed_at,
                p.created_by,
                created_by_user.real_name AS created_by_real_name,
                to_char(p.created_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS created_at,
                to_char(p.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at,
                (
                    SELECT COUNT(*)
                    FROM project_member pm
                    WHERE pm.project_id = p.id
                ) AS member_count,
                (
                    SELECT COUNT(*)
                    FROM project_node pn
                    WHERE pn.project_id = p.id
                ) AS node_count,
                (
                    SELECT COUNT(*)
                    FROM project_node pn
                    WHERE pn.project_id = p.id AND
                        pn.status = 3
                ) AS completed_node_count,
                (
                    SELECT COUNT(*)
                    FROM sub_task st
                    JOIN project_node pn ON pn.id = st.node_id
                    WHERE pn.project_id = p.id
                ) AS sub_task_count,
                (
                    SELECT COUNT(*)
                    FROM sub_task st
                    JOIN project_node pn ON pn.id = st.node_id
                    WHERE pn.project_id = p.id AND
                        st.status = 3
                ) AS completed_sub_task_count
            FROM project p
            JOIN sys_user owner_user ON owner_user.id = p.owner_user_id
            JOIN sys_user created_by_user ON created_by_user.id = p.created_by
            WHERE p.id = $1 AND
                (
                    $2 = TRUE OR
                    p.owner_user_id = $3 OR
                    EXISTS (
                        SELECT 1
                        FROM project_member pm
                        WHERE pm.project_id = p.id AND
                            pm.user_id = $3
                    )
                )
            LIMIT 1
        )SQL";

        const bool isAdmin = currentUserRole == user_domain::SystemRole::Admin;

        try {
            const auto result = co_await executor->execSqlCoro(
                findProjectDetailSql,
                projectId,
                isAdmin,
                currentUserId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();

            std::optional<std::string> completedAt;
            if (!row["completed_at"].isNull()) {
                completedAt = row["completed_at"].as<std::string>();
            }

            const auto status = static_cast<domain::ProjectStatus>(row["status"].as<int>());
            const bool isOwner = row["owner_user_id"].as<std::int64_t>() == currentUserId;
            const bool canManage = isAdmin || isOwner;
            const bool isCompleted = status == domain::ProjectStatus::Completed;

            // 管理员和拥有者可以管理（前提没完成）
            // 完成了也可以转交/删除
            co_return dto::view::ProjectDetailView{
                .id = row["id"].as<std::int64_t>(),
                .name = row["name"].as<std::string>(),
                .description = row["description"].as<std::string>(),
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .ownerRealName = row["owner_real_name"].as<std::string>(),
                .status = status,
                .plannedStartDate = row["planned_start_date"].as<std::string>(),
                .plannedEndDate = row["planned_end_date"].as<std::string>(),
                .completedAt = completedAt,
                .createdBy = row["created_by"].as<std::int64_t>(),
                .createdByRealName = row["created_by_real_name"].as<std::string>(),
                .createdAt = row["created_at"].as<std::string>(),
                .updatedAt = row["updated_at"].as<std::string>(),
                .memberCount = row["member_count"].as<std::int64_t>(),
                .nodeCount = row["node_count"].as<std::int64_t>(),
                .completedNodeCount = row["completed_node_count"].as<std::int64_t>(),
                .subTaskCount = row["sub_task_count"].as<std::int64_t>(),
                .completedSubTaskCount = row["completed_sub_task_count"].as<std::int64_t>(),
                .permissions = dto::view::ProjectDetailPermissionsView{
                    .canEditBasic = canManage && !isCompleted,
                    .canManageMembers = canManage && !isCompleted,
                    .canManageNodes = canManage && !isCompleted,
                    .canTransferOwner = canManage,
                    .canDelete = canManage
                }
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<ProjectGanttNodesResult>>
    ProjectRepository::findProjectGanttNodes(const common::db::SqlExecutorPtr &executor,
                                             const ProjectGanttNodesQuery &query) const {
        static const std::string findProjectGanttNodesSql = R"SQL(
            WITH project_context AS (
                SELECT
                    p.id,
                    p.name,
                    p.owner_user_id,
                    owner_user.real_name AS owner_real_name,
                    p.status,
                    to_char(p.planned_start_date, 'YYYY-MM-DD') AS planned_start_date,
                    to_char(p.planned_end_date, 'YYYY-MM-DD') AS planned_end_date,
                    to_char(p.completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS completed_at,
                    CASE
                        WHEN $2 = TRUE THEN TRUE
                        WHEN p.owner_user_id = $3 THEN TRUE
                        WHEN EXISTS (
                            SELECT 1
                            FROM project_member pm
                            WHERE pm.project_id = p.id AND
                                pm.user_id = $3
                        ) THEN TRUE
                        ELSE FALSE
                    END AS has_permission
                FROM project p
                JOIN sys_user owner_user ON owner_user.id = p.owner_user_id
                WHERE p.id = $1
            )
            SELECT
                pc.has_permission,
                pc.id AS project_id,
                pc.name AS project_name,
                pc.owner_user_id,
                pc.owner_real_name,
                pc.status AS project_status,
                pc.planned_start_date AS project_planned_start_date,
                pc.planned_end_date AS project_planned_end_date,
                pc.completed_at AS project_completed_at,
                pn.id AS node_id,
                pn.name AS node_name,
                pn.sequence_no,
                pn.status AS node_status,
                to_char(pn.planned_start_date, 'YYYY-MM-DD') AS node_planned_start_date,
                to_char(pn.planned_end_date, 'YYYY-MM-DD') AS node_planned_end_date,
                to_char(pn.completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS node_completed_at
            FROM project_context pc
            LEFT JOIN project_node pn ON pn.project_id = pc.id AND
                pc.has_permission
            ORDER BY pn.sequence_no ASC NULLS LAST, pn.id ASC NULLS LAST
        )SQL";

        const bool isAdmin = query.currentUserRole == user_domain::SystemRole::Admin;

        try {
            const auto result = co_await executor->execSqlCoro(
                findProjectGanttNodesSql,
                query.projectId,
                isAdmin,
                query.currentUserId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            ProjectGanttNodesResult ganttResult{
                .hasPermission = result.front()["has_permission"].as<bool>(),
                .detail = std::nullopt
            };

            if (!ganttResult.hasPermission) {
                co_return ganttResult;
            }

            const auto &firstRow = result.front();

            std::optional<std::string> projectCompletedAt;
            if (!firstRow["project_completed_at"].isNull()) {
                projectCompletedAt = firstRow["project_completed_at"].as<std::string>();
            }

            dto::view::ProjectGanttNodesView view{
                .project = dto::view::ProjectGanttProjectView{
                    .id = firstRow["project_id"].as<std::int64_t>(),
                    .name = firstRow["project_name"].as<std::string>(),
                    .ownerUserId = firstRow["owner_user_id"].as<std::int64_t>(),
                    .ownerRealName = firstRow["owner_real_name"].as<std::string>(),
                    .status = static_cast<domain::ProjectStatus>(
                        firstRow["project_status"].as<int>()),
                    .plannedStartDate = firstRow["project_planned_start_date"].as<std::string>(),
                    .plannedEndDate = firstRow["project_planned_end_date"].as<std::string>(),
                    .completedAt = projectCompletedAt
                },
                .nodes = {}
            };

            view.nodes.reserve(result.size());

            for (const auto &row : result) {
                if (row["node_id"].isNull()) {
                    continue;
                }

                std::optional<std::string> nodeCompletedAt;
                if (!row["node_completed_at"].isNull()) {
                    nodeCompletedAt = row["node_completed_at"].as<std::string>();
                }

                view.nodes.push_back(dto::view::ProjectGanttNodeItemView{
                    .id = row["node_id"].as<std::int64_t>(),
                    .name = row["node_name"].as<std::string>(),
                    .sequenceNo = row["sequence_no"].as<int>(),
                    .status = static_cast<project_node_domain::ProjectNodeStatus>(
                        row["node_status"].as<int>()),
                    .plannedStartDate = row["node_planned_start_date"].as<std::string>(),
                    .plannedEndDate = row["node_planned_end_date"].as<std::string>(),
                    .completedAt = nodeCompletedAt
                });
            }

            ganttResult.detail = std::move(view);
            co_return ganttResult;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<ProjectGanttMembersResult>>
    ProjectRepository::findProjectGanttMembers(const common::db::SqlExecutorPtr &executor,
                                               const ProjectGanttMembersQuery &query) const {
        static const std::string findProjectGanttMembersSql = R"SQL(
            WITH project_context AS (
                SELECT
                    p.id,
                    p.name,
                    p.owner_user_id,
                    owner_user.real_name AS owner_real_name,
                    p.status,
                    to_char(p.planned_start_date, 'YYYY-MM-DD') AS planned_start_date,
                    to_char(p.planned_end_date, 'YYYY-MM-DD') AS planned_end_date,
                    to_char(p.completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS completed_at,
                    CASE
                        WHEN $2 = TRUE THEN TRUE
                        WHEN p.owner_user_id = $3 THEN TRUE
                        WHEN EXISTS (
                            SELECT 1
                            FROM project_member pm
                            WHERE pm.project_id = p.id AND
                                pm.user_id = $3
                        ) THEN TRUE
                        ELSE FALSE
                    END AS has_permission
                FROM project p
                JOIN sys_user owner_user ON owner_user.id = p.owner_user_id
                WHERE p.id = $1
            ),
            member_candidates AS (
                SELECT DISTINCT
                    candidate.member_user_id,
                    candidate.member_real_name
                FROM (
                    SELECT
                        pm.user_id AS member_user_id,
                        member_user.real_name AS member_real_name
                    FROM project_context pc
                    JOIN project_member pm ON pm.project_id = pc.id AND
                        pc.has_permission
                    JOIN sys_user member_user ON member_user.id = pm.user_id

                    UNION ALL

                    SELECT
                        pc.owner_user_id AS member_user_id,
                        pc.owner_real_name AS member_real_name
                    FROM project_context pc
                    WHERE pc.has_permission
                ) candidate
            ),
            member_tasks AS (
                SELECT
                    st.responsible_user_id AS member_user_id,
                    responsible_user.real_name AS member_real_name,
                    st.id AS subtask_id,
                    st.name AS subtask_name,
                    pn.id AS node_id,
                    pn.name AS node_name,
                    st.status AS subtask_status,
                    st.progress_percent,
                    to_char(st.planned_start_date, 'YYYY-MM-DD') AS subtask_planned_start_date,
                    to_char(st.planned_end_date, 'YYYY-MM-DD') AS subtask_planned_end_date,
                    to_char(st.completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS subtask_completed_at,
                    st.priority,
                    st.planned_end_date AS subtask_planned_end_date_order
                FROM project_context pc
                JOIN project_node pn ON pn.project_id = pc.id AND
                    pc.has_permission
                JOIN sub_task st ON st.node_id = pn.id
                JOIN sys_user responsible_user ON responsible_user.id = st.responsible_user_id
            )
            SELECT
                pc.has_permission,
                pc.id AS project_id,
                pc.name AS project_name,
                pc.owner_user_id,
                pc.owner_real_name,
                pc.status AS project_status,
                pc.planned_start_date AS project_planned_start_date,
                pc.planned_end_date AS project_planned_end_date,
                pc.completed_at AS project_completed_at,
                mc.member_user_id,
                mc.member_real_name,
                mt.subtask_id,
                mt.subtask_name,
                mt.node_id,
                mt.node_name,
                mt.subtask_status,
                mt.progress_percent,
                mt.subtask_planned_start_date,
                mt.subtask_planned_end_date,
                mt.subtask_completed_at
            FROM project_context pc
            LEFT JOIN member_candidates mc ON pc.has_permission
            LEFT JOIN member_tasks mt ON mt.member_user_id = mc.member_user_id
            ORDER BY
                CASE
                    WHEN mt.subtask_id IS NULL THEN 1
                    ELSE 0
                END ASC,
                mc.member_real_name ASC NULLS LAST,
                mc.member_user_id ASC NULLS LAST,
                mt.priority DESC NULLS LAST,
                mt.subtask_planned_end_date_order ASC NULLS LAST,
                mt.subtask_id DESC NULLS LAST
        )SQL";

        const bool isAdmin = query.currentUserRole == user_domain::SystemRole::Admin;

        try {
            const auto result = co_await executor->execSqlCoro(
                findProjectGanttMembersSql,
                query.projectId,
                isAdmin,
                query.currentUserId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            ProjectGanttMembersResult ganttResult{
                .hasPermission = result.front()["has_permission"].as<bool>(),
                .detail = std::nullopt
            };

            if (!ganttResult.hasPermission) {
                co_return ganttResult;
            }

            const auto &firstRow = result.front();

            std::optional<std::string> projectCompletedAt;
            if (!firstRow["project_completed_at"].isNull()) {
                projectCompletedAt = firstRow["project_completed_at"].as<std::string>();
            }

            dto::view::ProjectGanttMembersView view{
                .project = dto::view::ProjectGanttProjectView{
                    .id = firstRow["project_id"].as<std::int64_t>(),
                    .name = firstRow["project_name"].as<std::string>(),
                    .ownerUserId = firstRow["owner_user_id"].as<std::int64_t>(),
                    .ownerRealName = firstRow["owner_real_name"].as<std::string>(),
                    .status = static_cast<domain::ProjectStatus>(
                        firstRow["project_status"].as<int>()),
                    .plannedStartDate = firstRow["project_planned_start_date"].as<std::string>(),
                    .plannedEndDate = firstRow["project_planned_end_date"].as<std::string>(),
                    .completedAt = projectCompletedAt
                },
                .memberRows = {}
            };

            view.memberRows.reserve(result.size());

            dto::view::ProjectGanttMemberRowView *currentMemberRow = nullptr;

            for (const auto &row : result) {
                if (row["member_user_id"].isNull()) {
                    continue;
                }

                const auto memberUserId = row["member_user_id"].as<std::int64_t>();
                if (!currentMemberRow || currentMemberRow->userId != memberUserId) {
                    view.memberRows.push_back(dto::view::ProjectGanttMemberRowView{
                        .userId = memberUserId,
                        .realName = row["member_real_name"].as<std::string>(),
                        .subtasks = {}
                    });
                    currentMemberRow = &view.memberRows.back();
                }

                if (row["subtask_id"].isNull()) {
                    continue;
                }

                std::optional<std::string> subtaskCompletedAt;
                if (!row["subtask_completed_at"].isNull()) {
                    subtaskCompletedAt = row["subtask_completed_at"].as<std::string>();
                }

                currentMemberRow->subtasks.push_back(dto::view::ProjectGanttMemberTaskItemView{
                    .subtaskId = row["subtask_id"].as<std::int64_t>(),
                    .name = row["subtask_name"].as<std::string>(),
                    .nodeId = row["node_id"].as<std::int64_t>(),
                    .nodeName = row["node_name"].as<std::string>(),
                    .status = static_cast<task_domain::TaskStatus>(
                        row["subtask_status"].as<int>()),
                    .progressPercent = row["progress_percent"].as<int>(),
                    .plannedStartDate = row["subtask_planned_start_date"].as<std::string>(),
                    .plannedEndDate = row["subtask_planned_end_date"].as<std::string>(),
                    .completedAt = subtaskCompletedAt
                });
            }

            ganttResult.detail = std::move(view);
            co_return ganttResult;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }
} // namespace project_tracker::modules::project::repository
