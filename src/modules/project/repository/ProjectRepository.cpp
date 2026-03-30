#include "modules/project/repository/ProjectRepository.h"

#include <algorithm>
#include <string>

#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::project::repository {
    namespace error = project_tracker::common::error;
    namespace user_domain = modules::user::domain;

    drogon::Task<dto::view::CreatedProjectView>
    ProjectRepository::createProject(const dto::command::CreateProjectInput &input) const {
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
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();

            const auto projectResult = co_await transaction->execSqlCoro(
                insertProjectSql,
                input.name,
                input.description,
                input.creatorUserId,
                input.plannedStartDate,
                input.plannedEndDate,
                input.creatorUserId);

            const auto &row = projectResult.front();

            co_await transaction->execSqlCoro(
                insertProjectMemberSql,
                row["id"].as<std::int64_t>(),
                input.creatorUserId,
                input.creatorUserId);

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

    drogon::Task<ProjectListPage>
    ProjectRepository::listProjects(const ProjectListQuery &query) const {

        // 先进行筛选（项目名，状态，拥有者）
        // 再根据是否是管理员，拥有者，项目成员
        static const std::string countProjectsSql = R"SQL(
            SELECT COUNT(*) AS total
            FROM project p
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
        )SQL";

        static const std::string selectProjectsSql = R"SQL(
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
                    FROM sub_task st
                    JOIN project_node pn ON pn.id = st.node_id
                    WHERE pn.project_id = p.id
                ) AS sub_task_count,
                to_char(p.created_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS created_at,
                to_char(p.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at,
                (p.owner_user_id = $5) AS is_owner
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
            ORDER BY p.updated_at DESC, p.id DESC
            LIMIT $6 OFFSET $7
        )SQL";

        const std::int64_t page = std::max(query.page, std::int64_t{1});
        const std::int64_t pageSizeForSql = std::max(query.pageSize, std::int64_t{1});
        const std::int64_t offsetForSql = (page - 1) * pageSizeForSql;
        const std::string keyword = query.keyword.empty() ? "" : "%" + query.keyword + "%";
        const int status = query.status ? domain::toInt(*query.status) : 0;
        const std::int64_t ownerUserId = query.ownerUserId.value_or(0);
        const bool isAdmin = query.currentUserRole == user_domain::SystemRole::Admin;

        try {
            const auto dbClient = drogon::app().getDbClient();

            const auto totalResult = co_await dbClient->execSqlCoro(
                countProjectsSql,
                keyword,
                status,
                ownerUserId,
                isAdmin,
                query.currentUserId);

            const auto listResult = co_await dbClient->execSqlCoro(
                selectProjectsSql,
                keyword,
                status,
                ownerUserId,
                isAdmin,
                query.currentUserId,
                pageSizeForSql,
                offsetForSql);

            ProjectListPage result{
                .list = {},
                .total = totalResult.front()["total"].as<std::int64_t>(),
                .page = page,
                .pageSize = pageSizeForSql
            };

            result.list.reserve(listResult.size());

            for (const auto &row : listResult) {
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

    drogon::Task<std::optional<dto::view::ProjectDetailView>>
    ProjectRepository::findProjectDetail(std::int64_t projectId,
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
            const auto dbClient = drogon::app().getDbClient();
            const auto result = co_await dbClient->execSqlCoro(
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
} // namespace project_tracker::modules::project::repository
