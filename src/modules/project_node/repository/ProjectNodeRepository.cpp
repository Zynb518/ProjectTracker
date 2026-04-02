#include "modules/project_node/repository/ProjectNodeRepository.h"

#include <string>

#include <drogon/orm/Exception.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::project_node::repository {
    namespace error = project_tracker::common::error;
    namespace user_domain = modules::user::domain;

    drogon::Task<std::optional<ProjectNodeListResult>>
    ProjectNodeRepository::listProjectNodes(const common::db::SqlExecutorPtr &executor,
                                            const ProjectNodeListQuery &query) const {
        // 查询思路：
        // 1. project_context 先确定项目是否存在，以及当前用户是否可见。
        // 2. task_stats 单独按 node_id 聚合子任务统计，避免主查询堆一大串 GROUP BY。
        // 3. 主查询在有权限时返回节点列表，再把统计结果左连接回每个节点。
        static const std::string listProjectNodesSql = R"SQL(
            WITH project_context AS (
                SELECT
                    p.id AS project_id,
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
            ),
            task_stats AS (
                SELECT
                    pn.id AS node_id,
                    COUNT(st.id) AS sub_task_count,
                    COUNT(st.id) FILTER (WHERE st.status = 3) AS completed_sub_task_count
                FROM project_context pc
                JOIN project_node pn ON pn.project_id = pc.project_id
                LEFT JOIN sub_task st ON st.node_id = pn.id
                WHERE pc.has_permission
                GROUP BY pn.id
            )
            SELECT
                pc.has_permission,
                pn.id,
                pn.project_id,
                pn.name,
                pn.description,
                pn.sequence_no,
                pn.status,
                to_char(pn.planned_start_date, 'YYYY-MM-DD') AS planned_start_date,
                to_char(pn.planned_end_date, 'YYYY-MM-DD') AS planned_end_date,
                to_char(pn.completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS completed_at,
                pn.created_by,
                to_char(pn.created_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS created_at,
                to_char(pn.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at,
                COALESCE(ts.sub_task_count, 0) AS sub_task_count,
                COALESCE(ts.completed_sub_task_count, 0) AS completed_sub_task_count
            FROM project_context pc
            LEFT JOIN project_node pn ON pn.project_id = pc.project_id AND
                pc.has_permission
            LEFT JOIN task_stats ts ON ts.node_id = pn.id
            ORDER BY pn.sequence_no ASC NULLS LAST, pn.id ASC NULLS LAST
        )SQL";

        const bool isAdmin = query.currentUserRole == user_domain::SystemRole::Admin;

        try {
            const auto listResult = co_await executor->execSqlCoro(
                listProjectNodesSql,
                query.projectId,
                isAdmin,
                query.currentUserId);

            if (listResult.empty()) {
                co_return std::nullopt;
            }

            ProjectNodeListResult result{
                .hasPermission = listResult.front()["has_permission"].as<bool>(),
                .list = {}
            };

            if (!result.hasPermission) {
                co_return result;
            }

            result.list.reserve(listResult.size());

            for (const auto &row : listResult) {
                if (row["id"].isNull()) {
                    continue;
                }

                std::optional<std::string> completedAt;
                if (!row["completed_at"].isNull()) {
                    completedAt = row["completed_at"].as<std::string>();
                }

                result.list.push_back(dto::view::ProjectNodeListItemView{
                    .id = row["id"].as<std::int64_t>(),
                    .projectId = row["project_id"].as<std::int64_t>(),
                    .name = row["name"].as<std::string>(),
                    .description = row["description"].as<std::string>(),
                    .sequenceNo = row["sequence_no"].as<int>(),
                    .status = static_cast<domain::ProjectNodeStatus>(row["status"].as<int>()),
                    .plannedStartDate = row["planned_start_date"].as<std::string>(),
                    .plannedEndDate = row["planned_end_date"].as<std::string>(),
                    .completedAt = completedAt,
                    .createdBy = row["created_by"].as<std::int64_t>(),
                    .createdAt = row["created_at"].as<std::string>(),
                    .updatedAt = row["updated_at"].as<std::string>(),
                    .subTaskCount = row["sub_task_count"].as<std::int64_t>(),
                    .completedSubTaskCount = row["completed_sub_task_count"].as<std::int64_t>()
                });
            }

            co_return result;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }
} // namespace project_tracker::modules::project_node::repository
