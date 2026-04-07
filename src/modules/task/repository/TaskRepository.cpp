#include "modules/task/repository/TaskRepository.h"

#include <string>

#include <drogon/orm/Exception.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::task::repository {
    namespace error = project_tracker::common::error;
    namespace project_domain = modules::project::domain;
    namespace project_node_domain = modules::project_node::domain;
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

    drogon::Task<std::optional<TaskDetailResult>>
    TaskRepository::findTaskDetail(const common::db::SqlExecutorPtr &executor,
                                   const TaskDetailQuery &query) const {
        // 查询思路：
        // 1. task_context 先定位子任务所属项目，子任务不存在时整条查询直接返回空。
        // 2. permission_context 在子任务存在前提下判断当前用户是否可见该项目。
        // 3. task_detail 只在有权限时回填完整详情字段。
        static const std::string findTaskDetailSql = R"SQL(
            WITH task_context AS (
                SELECT
                    st.id AS sub_task_id,
                    pn.project_id
                FROM sub_task st
                JOIN project_node pn ON pn.id = st.node_id
                WHERE st.id = $1
            ),
            permission_context AS (
                SELECT
                    tc.sub_task_id,
                    CASE
                        WHEN $2 = TRUE THEN TRUE
                        WHEN EXISTS (
                            SELECT 1
                            FROM project_member self_pm
                            WHERE self_pm.project_id = tc.project_id AND
                                self_pm.user_id = $3
                        ) THEN TRUE
                        ELSE FALSE
                    END AS has_permission
                FROM task_context tc
            ),
            task_detail AS (
                SELECT
                    st.id,
                    p.id AS project_id,
                    p.name AS project_name,
                    pn.id AS node_id,
                    pn.name AS node_name,
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
                    to_char(st.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
                FROM permission_context pc
                JOIN sub_task st ON st.id = pc.sub_task_id
                JOIN project_node pn ON pn.id = st.node_id
                JOIN project p ON p.id = pn.project_id
                JOIN sys_user responsible_user ON responsible_user.id = st.responsible_user_id
                WHERE pc.has_permission
            )
            SELECT
                pc.has_permission,
                td.id,
                td.project_id,
                td.project_name,
                td.node_id,
                td.node_name,
                td.name,
                td.description,
                td.responsible_user_id,
                td.responsible_real_name,
                td.status,
                td.progress_percent,
                td.priority,
                td.planned_start_date,
                td.planned_end_date,
                td.completed_at,
                td.created_by,
                td.created_at,
                td.updated_at
            FROM permission_context pc
            LEFT JOIN task_detail td ON TRUE
        )SQL";

        const bool isAdmin = query.currentUserRole == user_domain::SystemRole::Admin;

        try {
            const auto result = co_await executor->execSqlCoro(
                findTaskDetailSql,
                query.subTaskId,
                isAdmin,
                query.currentUserId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            TaskDetailResult detailResult{
                .hasPermission = result.front()["has_permission"].as<bool>(),
                .detail = std::nullopt
            };

            if (!detailResult.hasPermission || result.front()["id"].isNull()) {
                co_return detailResult;
            }

            const auto &row = result.front();

            std::optional<std::string> completedAt;
            if (!row["completed_at"].isNull()) {
                completedAt = row["completed_at"].as<std::string>();
            }

            detailResult.detail = dto::view::TaskDetailView{
                .id = row["id"].as<std::int64_t>(),
                .projectId = row["project_id"].as<std::int64_t>(),
                .projectName = row["project_name"].as<std::string>(),
                .nodeId = row["node_id"].as<std::int64_t>(),
                .nodeName = row["node_name"].as<std::string>(),
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
            };

            co_return detailResult;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<TaskCreateProjectCheckResult>>
    TaskRepository::findTaskCreateProjectCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId) const {
        static const std::string findTaskCreateProjectCheckResultForUpdateSql = R"SQL(
            SELECT
                p.owner_user_id,
                creator_user.system_role AS creator_user_role,
                p.status AS project_status
            FROM project p
            JOIN sys_user creator_user ON creator_user.id = p.created_by
            WHERE p.id = $1
            FOR UPDATE OF p
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findTaskCreateProjectCheckResultForUpdateSql,
                projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return TaskCreateProjectCheckResult{
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .creatorUserRole = static_cast<user_domain::SystemRole>(
                    row["creator_user_role"].as<int>()),
                .projectStatus = static_cast<project_domain::ProjectStatus>(
                    row["project_status"].as<int>())
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<TaskCreateNodeCheckResult>>
    TaskRepository::findTaskCreateNodeCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId,
        std::int64_t nodeId) const {
        // 查询思路：
        // 1. 项目行已在写事务上一步锁定，这里只锁目标节点。
        // 2. 同时带回节点状态与日期范围，供 service 做冻结和区间校验。
        static const std::string findTaskCreateNodeCheckResultForUpdateSql = R"SQL(
            SELECT
                pn.status AS node_status,
                to_char(pn.planned_start_date, 'YYYY-MM-DD') AS node_planned_start_date,
                to_char(pn.planned_end_date, 'YYYY-MM-DD') AS node_planned_end_date
            FROM project_node pn
            WHERE pn.project_id = $1 AND
                pn.id = $2
            FOR UPDATE OF pn
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findTaskCreateNodeCheckResultForUpdateSql,
                projectId,
                nodeId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return TaskCreateNodeCheckResult{
                .nodeStatus = static_cast<project_node_domain::ProjectNodeStatus>(
                    row["node_status"].as<int>()),
                .nodePlannedStartDate = row["node_planned_start_date"].as<std::string>(),
                .nodePlannedEndDate = row["node_planned_end_date"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::CreatedTaskView>>
    TaskRepository::insertTask(const common::db::SqlExecutorPtr &executor,
                               const dto::command::CreateTaskInput &input,
                               std::int64_t responsibleUserId,
                               domain::TaskPriority priority) const {
        // 查询思路：
        // 1. 目标节点已在写事务上一步锁定，这里直接基于节点插入子任务。
        // 2. 负责人成员关系继续在 INSERT ... SELECT 里做最终保护，避免用旧读取抢先决定结果。
        static const std::string insertTaskSql = R"SQL(
            INSERT INTO sub_task (
                node_id,
                name,
                description,
                responsible_user_id,
                status,
                progress_percent,
                priority,
                planned_start_date,
                planned_end_date,
                created_by
            )
            SELECT
                pn.id,
                $3,
                $4,
                $5,
                1,
                0,
                $6,
                $7,
                $8,
                $9
            FROM project_node pn
            WHERE pn.project_id = $1 AND
                pn.id = $2 AND
                EXISTS (
                    SELECT 1
                    FROM project_member pm
                    WHERE pm.project_id = $1 AND
                        pm.user_id = $5
                )
            RETURNING
                id,
                node_id,
                name,
                description,
                responsible_user_id,
                status,
                progress_percent,
                priority,
                to_char(planned_start_date, 'YYYY-MM-DD') AS planned_start_date,
                to_char(planned_end_date, 'YYYY-MM-DD') AS planned_end_date,
                to_char(completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS completed_at,
                created_by,
                to_char(created_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS created_at,
                to_char(updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                insertTaskSql,
                input.projectId,
                input.nodeId,
                input.name,
                input.description,
                responsibleUserId,
                domain::toInt(priority),
                input.plannedStartDate,
                input.plannedEndDate,
                input.creatorUserId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();

            std::optional<std::string> completedAt;
            if (!row["completed_at"].isNull()) {
                completedAt = row["completed_at"].as<std::string>();
            }

            co_return dto::view::CreatedTaskView{
                .id = row["id"].as<std::int64_t>(),
                .nodeId = row["node_id"].as<std::int64_t>(),
                .name = row["name"].as<std::string>(),
                .description = row["description"].as<std::string>(),
                .responsibleUserId = row["responsible_user_id"].as<std::int64_t>(),
                .status = static_cast<domain::TaskStatus>(row["status"].as<int>()),
                .progressPercent = row["progress_percent"].as<int>(),
                .priority = static_cast<domain::TaskPriority>(row["priority"].as<int>()),
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
} // namespace project_tracker::modules::task::repository
