#include "modules/task/repository/TaskRepository.h"

#include <string>

#include <drogon/orm/Exception.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::task::repository {
    namespace error = project_tracker::common::error;
    namespace user_domain = modules::user::domain;

    drogon::Task<TaskListResult>
    TaskRepository::listNodeTasks(const common::db::SqlExecutorPtr &executor,
                                  const TaskListQuery &query) const {
        // 查询思路：
        // 1. project_context 先判断项目是否存在，以及当前用户是否可见。
        // 2. node_context 只在项目存在且可见时确认目标节点是否属于当前项目。
        // 3. task_list 只在节点存在时返回子任务列表，并支持状态和负责人筛选。
        static const std::string listNodeTasksSql = R"SQL(
            WITH project_context AS (
                SELECT
                    (p.id IS NOT NULL) AS project_exists,
                    CASE
                        WHEN p.id IS NULL THEN FALSE
                        WHEN $3 = TRUE THEN TRUE
                        WHEN EXISTS (
                            SELECT 1
                            FROM project_member self_pm
                            WHERE self_pm.project_id = p.id AND
                                self_pm.user_id = $4
                        ) THEN TRUE
                        ELSE FALSE
                    END AS has_permission
                FROM (SELECT 1) anchor
                LEFT JOIN project p ON p.id = $1
            ),
            node_context AS (
                SELECT
                    pc.project_exists,
                    pc.has_permission,
                    CASE
                        WHEN NOT pc.project_exists OR NOT pc.has_permission THEN FALSE
                        WHEN EXISTS (
                            SELECT 1
                            FROM project_node pn
                            WHERE pn.project_id = $1 AND
                                pn.id = $2
                        ) THEN TRUE
                        ELSE FALSE
                    END AS node_exists
                FROM project_context pc
            ),
            task_list AS (
                SELECT
                    st.id,
                    st.node_id,
                    st.name,
                    st.description,
                    st.responsible_user_id,
                    responsible_user.real_name AS responsible_real_name,
                    st.status,
                    st.progress_percent,
                    st.priority,
                    to_char(st.planned_start_date, 'YYYY-MM-DD') AS planned_start_date,
                    to_char(st.planned_end_date, 'YYYY-MM-DD') AS planned_end_date,
                    to_char(st.completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS completed_at,
                    st.created_by,
                    to_char(st.created_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS created_at,
                    to_char(st.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at,
                    st.planned_end_date AS planned_end_date_order
                FROM node_context nc
                JOIN sub_task st ON nc.node_exists AND
                    st.node_id = $2
                JOIN sys_user responsible_user ON responsible_user.id = st.responsible_user_id
                WHERE ($5 = 0 OR st.status = $5) AND
                    ($6 = 0::bigint OR st.responsible_user_id = $6)
            )
            SELECT
                nc.project_exists,
                nc.has_permission,
                nc.node_exists,
                tl.id,
                tl.node_id,
                tl.name,
                tl.description,
                tl.responsible_user_id,
                tl.responsible_real_name,
                tl.status,
                tl.progress_percent,
                tl.priority,
                tl.planned_start_date,
                tl.planned_end_date,
                tl.completed_at,
                tl.created_by,
                tl.created_at,
                tl.updated_at
            FROM node_context nc
            LEFT JOIN task_list tl ON TRUE
            ORDER BY
                tl.priority DESC NULLS LAST,
                tl.planned_end_date_order ASC NULLS LAST,
                tl.id DESC NULLS LAST
        )SQL";

        const bool isAdmin = query.currentUserRole == user_domain::SystemRole::Admin;
        const int status = query.status ? domain::toInt(*query.status) : 0;
        const std::int64_t responsibleUserId = query.responsibleUserId.value_or(0);

        try {
            const auto listResult = co_await executor->execSqlCoro(
                listNodeTasksSql,
                query.projectId,
                query.nodeId,
                isAdmin,
                query.currentUserId,
                status,
                responsibleUserId);

            TaskListResult result{
                .projectExists = listResult.front()["project_exists"].as<bool>(),
                .hasPermission = listResult.front()["has_permission"].as<bool>(),
                .nodeExists = listResult.front()["node_exists"].as<bool>(),
                .list = {}
            };

            if (!result.projectExists || !result.hasPermission || !result.nodeExists) {
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

                result.list.push_back(dto::view::TaskListItemView{
                    .id = row["id"].as<std::int64_t>(),
                    .nodeId = row["node_id"].as<std::int64_t>(),
                    .name = row["name"].as<std::string>(),
                    .description = row["description"].as<std::string>(),
                    .responsibleUserId = row["responsible_user_id"].as<std::int64_t>(),
                    .responsibleRealName = row["responsible_real_name"].as<std::string>(),
                    .status = static_cast<domain::TaskStatus>(row["status"].as<int>()),
                    .progressPercent = row["progress_percent"].as<int>(),
                    .priority = static_cast<domain::TaskPriority>(row["priority"].as<int>()),
                    .plannedStartDate = row["planned_start_date"].as<std::string>(),
                    .plannedEndDate = row["planned_end_date"].as<std::string>(),
                    .completedAt = completedAt,
                    .createdBy = row["created_by"].as<std::int64_t>(),
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

    drogon::Task<std::vector<dto::view::MyTaskListItemView>>
    TaskRepository::listMyTasks(const common::db::SqlExecutorPtr &executor,
                                const MyTaskListQuery &query) const {
        // 查询思路：
        // 1. 只返回当前用户负责的子任务，并关联补齐所属项目和阶段节点名称。
        // 2. 支持按状态和项目筛选，排序继续与节点下子任务列表保持一致。
        static const std::string listMyTasksSql = R"SQL(
            SELECT
                st.id,
                p.id AS project_id,
                p.name AS project_name,
                pn.id AS node_id,
                pn.name AS node_name,
                st.name,
                st.status,
                st.progress_percent,
                st.priority,
                to_char(st.planned_start_date, 'YYYY-MM-DD') AS planned_start_date,
                to_char(st.planned_end_date, 'YYYY-MM-DD') AS planned_end_date,
                to_char(st.completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS completed_at,
                to_char(st.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at,
                st.planned_end_date AS planned_end_date_order
            FROM sub_task st
            JOIN project_node pn ON pn.id = st.node_id
            JOIN project p ON p.id = pn.project_id
            WHERE st.responsible_user_id = $1
                AND ($2 = 0 OR st.status = $2)
                AND ($3 = 0::bigint OR p.id = $3)
            ORDER BY
                st.priority DESC,
                planned_end_date_order ASC,
                st.id DESC
        )SQL";

        const int status = query.status ? domain::toInt(*query.status) : 0;
        const std::int64_t projectId = query.projectId.value_or(0);

        try {
            const auto queryResult = co_await executor->execSqlCoro(
                listMyTasksSql,
                query.currentUserId,
                status,
                projectId);

            std::vector<dto::view::MyTaskListItemView> list;
            list.reserve(queryResult.size());

            for (const auto &row : queryResult) {
                std::optional<std::string> completedAt;
                if (!row["completed_at"].isNull()) {
                    completedAt = row["completed_at"].as<std::string>();
                }

                list.push_back(dto::view::MyTaskListItemView{
                    .id = row["id"].as<std::int64_t>(),
                    .projectId = row["project_id"].as<std::int64_t>(),
                    .projectName = row["project_name"].as<std::string>(),
                    .nodeId = row["node_id"].as<std::int64_t>(),
                    .nodeName = row["node_name"].as<std::string>(),
                    .name = row["name"].as<std::string>(),
                    .status = static_cast<domain::TaskStatus>(row["status"].as<int>()),
                    .progressPercent = row["progress_percent"].as<int>(),
                    .priority = static_cast<domain::TaskPriority>(row["priority"].as<int>()),
                    .plannedStartDate = row["planned_start_date"].as<std::string>(),
                    .plannedEndDate = row["planned_end_date"].as<std::string>(),
                    .completedAt = completedAt,
                    .updatedAt = row["updated_at"].as<std::string>()
                });
            }

            co_return list;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }
} // namespace project_tracker::modules::task::repository
