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

    drogon::Task<std::optional<TaskBasicInfoUpdateProjectCheckResult>>
    TaskRepository::findTaskBasicInfoUpdateProjectCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t subTaskId) const {
        // 查询思路：
        // 1. 通过 sub_task -> project_node -> project 定位所属项目。
        // 2. 这一段只锁项目行，供 service 先做项目级权限与冻结判断。
        static const std::string findTaskBasicInfoUpdateProjectCheckResultForUpdateSql = R"SQL(
            SELECT
                p.owner_user_id,
                creator_user.system_role AS creator_user_role,
                p.status AS project_status
            FROM project p
            JOIN project_node pn ON pn.project_id = p.id
            JOIN sub_task st ON st.node_id = pn.id
            JOIN sys_user creator_user ON creator_user.id = p.created_by
            WHERE st.id = $1
            FOR UPDATE OF p
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findTaskBasicInfoUpdateProjectCheckResultForUpdateSql,
                subTaskId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return TaskBasicInfoUpdateProjectCheckResult{
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

    drogon::Task<std::optional<TaskStartProjectCheckResult>>
    TaskRepository::findTaskStartProjectCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t subTaskId) const {
        // 查询思路：
        // 1. 通过 sub_task -> project_node -> project 定位所属项目。
        // 2. 这一段只锁项目行，供 service 先做项目级权限与冻结判断。
        static const std::string findTaskStartProjectCheckResultForUpdateSql = R"SQL(
            SELECT
                p.id AS project_id,
                p.owner_user_id,
                creator_user.system_role AS creator_user_role,
                p.status AS project_status
            FROM project p
            JOIN project_node pn ON pn.project_id = p.id
            JOIN sub_task st ON st.node_id = pn.id
            JOIN sys_user creator_user ON creator_user.id = p.created_by
            WHERE st.id = $1
            FOR UPDATE OF p
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findTaskStartProjectCheckResultForUpdateSql,
                subTaskId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return TaskStartProjectCheckResult{
                .projectId = row["project_id"].as<std::int64_t>(),
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

    drogon::Task<std::optional<TaskStartTaskCheckResult>>
    TaskRepository::findTaskStartTaskCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t subTaskId) const {
        // 查询思路：
        // 1. 项目行已在写事务上一步锁定，这里只锁目标子任务行。
        // 2. 同时带回负责人、所属节点状态与子任务当前状态，供 service 做权限与冻结校验。
        static const std::string findTaskStartTaskCheckResultForUpdateSql = R"SQL(
            SELECT
                pn.id AS node_id,
                st.responsible_user_id,
                pn.status AS node_status,
                st.status AS task_status
            FROM sub_task st
            JOIN project_node pn ON pn.id = st.node_id
            WHERE st.id = $1
            FOR UPDATE OF st
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findTaskStartTaskCheckResultForUpdateSql,
                subTaskId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return TaskStartTaskCheckResult{
                .nodeId = row["node_id"].as<std::int64_t>(),
                .responsibleUserId = row["responsible_user_id"].as<std::int64_t>(),
                .nodeStatus = static_cast<project_node_domain::ProjectNodeStatus>(
                    row["node_status"].as<int>()),
                .taskStatus = static_cast<domain::TaskStatus>(
                    row["task_status"].as<int>())
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<TaskBasicInfoUpdateTaskCheckResult>>
    TaskRepository::findTaskBasicInfoUpdateTaskCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t subTaskId) const {
        // 查询思路：
        // 1. 项目行已在写事务上一步锁定，这里只锁目标子任务行。
        // 2. 同时带回所属节点状态与子任务当前状态，供 service 做冻结校验。
        static const std::string findTaskBasicInfoUpdateTaskCheckResultForUpdateSql = R"SQL(
            SELECT
                pn.status AS node_status,
                st.status AS task_status
            FROM sub_task st
            JOIN project_node pn ON pn.id = st.node_id
            WHERE st.id = $1
            FOR UPDATE OF st
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findTaskBasicInfoUpdateTaskCheckResultForUpdateSql,
                subTaskId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return TaskBasicInfoUpdateTaskCheckResult{
                .nodeStatus = static_cast<project_node_domain::ProjectNodeStatus>(
                    row["node_status"].as<int>()),
                .taskStatus = static_cast<domain::TaskStatus>(
                    row["task_status"].as<int>())
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::UpdatedTaskStatusView>>
    TaskRepository::updateTaskStatusForStart(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t subTaskId) const {
        // -- 按北京时间取“今天”，再和 DATE 类型的 planned_end_date 比较是否已延期。
        static const std::string updateTaskStatusForStartSql = R"SQL(
            UPDATE sub_task st
            SET
                status = CASE
                    WHEN (NOW() AT TIME ZONE 'Asia/Shanghai')::date > st.planned_end_date THEN 4
                    ELSE 2
                END,
                progress_percent = 0,
                completed_at = NULL,
                updated_at = NOW()
            WHERE st.id = $1 AND
                st.status = 1
            RETURNING
                st.id,
                st.status,
                st.progress_percent,
                to_char(st.completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS completed_at,
                to_char(st.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                updateTaskStatusForStartSql,
                subTaskId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();

            std::optional<std::string> completedAt;
            if (!row["completed_at"].isNull()) {
                completedAt = row["completed_at"].as<std::string>();
            }

            co_return dto::view::UpdatedTaskStatusView{
                .id = row["id"].as<std::int64_t>(),
                .status = static_cast<domain::TaskStatus>(row["status"].as<int>()),
                .progressPercent = row["progress_percent"].as<int>(),
                .completedAt = completedAt,
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<dto::view::TaskProgressRecordView>
    TaskRepository::insertTaskProgressRecordForStart(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t subTaskId,
        std::int64_t operatorUserId,
        const std::string &progressNote,
        domain::TaskStatus status) const {
        static const std::string insertTaskProgressRecordForStartSql = R"SQL(
            INSERT INTO sub_task_progress (
                sub_task_id,
                operator_user_id,
                progress_note,
                progress_percent,
                status
            )
            VALUES ($1, $2, $3, 0, $4)
            RETURNING
                id,
                sub_task_id,
                operator_user_id,
                progress_note,
                progress_percent,
                status,
                to_char(created_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS created_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                insertTaskProgressRecordForStartSql,
                subTaskId,
                operatorUserId,
                progressNote,
                domain::toInt(status));

            const auto &row = result.front();
            co_return dto::view::TaskProgressRecordView{
                .id = row["id"].as<std::int64_t>(),
                .subTaskId = row["sub_task_id"].as<std::int64_t>(),
                .operatorUserId = row["operator_user_id"].as<std::int64_t>(),
                .progressNote = row["progress_note"].as<std::string>(),
                .progressPercent = row["progress_percent"].as<int>(),
                .status = static_cast<domain::TaskStatus>(row["status"].as<int>()),
                .createdAt = row["created_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<TaskReopenRestoreSnapshot>
    TaskRepository::findTaskReopenRestoreSnapshot(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t subTaskId) const {
        // 查询思路：
        // 1. 这里不额外锁历史表，依赖写链路先锁同一条 sub_task 再写当前值/历史值。
        // 2. 只取最近一条未完成历史，供 service 恢复 progress_percent 与基础状态。
        static const std::string findTaskReopenRestoreSnapshotSql = R"SQL(
            WITH latest_unfinished_progress AS (
                SELECT
                    stp.status,
                    stp.progress_percent
                FROM sub_task_progress stp
                WHERE stp.sub_task_id = $1 AND
                    stp.status <> $2
                ORDER BY stp.created_at DESC, stp.id DESC
                LIMIT 1
            )
            SELECT
                lup.status,
                lup.progress_percent
            FROM (SELECT 1) anchor
            LEFT JOIN latest_unfinished_progress lup ON TRUE
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findTaskReopenRestoreSnapshotSql,
                subTaskId,
                domain::toInt(domain::TaskStatus::Completed));

            if (result.empty()) {
                co_return TaskReopenRestoreSnapshot{
                    .latestUnfinishedStatus = std::nullopt,
                    .latestUnfinishedProgressPercent = std::nullopt
                };
            }

            const auto &row = result.front();

            std::optional<domain::TaskStatus> latestUnfinishedStatus;
            if (!row["status"].isNull()) {
                latestUnfinishedStatus = static_cast<domain::TaskStatus>(row["status"].as<int>());
            }

            std::optional<int> latestUnfinishedProgressPercent;
            if (!row["progress_percent"].isNull()) {
                latestUnfinishedProgressPercent = row["progress_percent"].as<int>();
            }

            co_return TaskReopenRestoreSnapshot{
                .latestUnfinishedStatus = latestUnfinishedStatus,
                .latestUnfinishedProgressPercent = latestUnfinishedProgressPercent
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::UpdatedTaskStatusView>>
    TaskRepository::updateTaskStatusForReopen(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t subTaskId,
        domain::TaskStatus status,
        int progressPercent) const {
        // -- 先恢复最近一条未完成历史；如果当前按北京时间已经逾期，则最终状态强制落到已延期。
        static const std::string updateTaskStatusForReopenSql = R"SQL(
            UPDATE sub_task st
            SET
                status = CASE
                    WHEN $2 = $4 THEN $4
                    WHEN (NOW() AT TIME ZONE 'Asia/Shanghai')::date > st.planned_end_date THEN $4
                    ELSE $2
                END,
                progress_percent = $3,
                completed_at = NULL,
                updated_at = NOW()
            WHERE st.id = $1 AND
                st.status = $5
            RETURNING
                st.id,
                st.status,
                st.progress_percent,
                to_char(st.completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS completed_at,
                to_char(st.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                updateTaskStatusForReopenSql,
                subTaskId,
                domain::toInt(status),
                progressPercent,
                domain::toInt(domain::TaskStatus::Delayed),
                domain::toInt(domain::TaskStatus::Completed));

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();

            std::optional<std::string> completedAt;
            if (!row["completed_at"].isNull()) {
                completedAt = row["completed_at"].as<std::string>();
            }

            co_return dto::view::UpdatedTaskStatusView{
                .id = row["id"].as<std::int64_t>(),
                .status = static_cast<domain::TaskStatus>(row["status"].as<int>()),
                .progressPercent = row["progress_percent"].as<int>(),
                .completedAt = completedAt,
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<TaskBasicInfoUpdateResult>
    TaskRepository::updateTaskBasicInfo(
        const common::db::SqlExecutorPtr &executor,
        const dto::command::UpdateTaskBasicInfoInput &input) const {
        // 查询思路：
        // 1. target_context 先算出本次更新后的有效字段，并带上所属节点日期范围与所属项目ID。
        // 2. validation_context 统一判断“日期区间是否合法、是否仍被节点覆盖、负责人是否仍属于项目成员”。
        // 3. 最终只在 failure_reason 为空时执行 UPDATE，并把失败原因一并回传给 service 做错误映射。
        static const std::string updateTaskBasicInfoSql = R"SQL(
            WITH target_context AS (
                SELECT
                    st.id,
                    COALESCE($2, st.name) AS next_name,
                    COALESCE($3, st.description) AS next_description,
                    COALESCE($4::bigint, st.responsible_user_id) AS next_responsible_user_id,
                    COALESCE($5::integer, st.priority) AS next_priority,
                    COALESCE($6::date, st.planned_start_date) AS next_planned_start_date,
                    COALESCE($7::date, st.planned_end_date) AS next_planned_end_date,
                    pn.project_id,
                    pn.planned_start_date AS node_planned_start_date,
                    pn.planned_end_date AS node_planned_end_date
                FROM sub_task st
                JOIN project_node pn ON pn.id = st.node_id
                WHERE st.id = $1
            ),
            validation_context AS (
                SELECT
                    tc.id,
                    CASE
                        WHEN tc.next_planned_start_date > tc.next_planned_end_date THEN
                            'invalid_date_range'
                        WHEN tc.next_planned_start_date < tc.node_planned_start_date OR
                             tc.next_planned_end_date > tc.node_planned_end_date THEN
                            'invalid_date_range'
                        WHEN NOT EXISTS (
                            SELECT 1
                            FROM project_member pm
                            WHERE pm.project_id = tc.project_id AND
                                pm.user_id = tc.next_responsible_user_id
                        ) THEN
                            'responsible_user_invalid'
                        ELSE NULL
                    END AS failure_reason
                FROM target_context tc
            ),
            updated_task AS (
                UPDATE sub_task st
                SET
                    name = tc.next_name,
                    description = tc.next_description,
                    responsible_user_id = tc.next_responsible_user_id,
                    priority = tc.next_priority,
                    planned_start_date = tc.next_planned_start_date,
                    planned_end_date = tc.next_planned_end_date,
                    updated_at = NOW()
                FROM target_context tc
                JOIN validation_context vc ON vc.id = tc.id
                WHERE st.id = tc.id AND
                    vc.failure_reason IS NULL
                RETURNING
                    st.id,
                    st.name,
                    st.description,
                    st.responsible_user_id,
                    st.priority,
                    to_char(st.planned_start_date, 'YYYY-MM-DD') AS planned_start_date,
                    to_char(st.planned_end_date, 'YYYY-MM-DD') AS planned_end_date,
                    to_char(st.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
            )
            SELECT
                vc.failure_reason,
                ut.id,
                ut.name,
                ut.description,
                ut.responsible_user_id,
                ut.priority,
                ut.planned_start_date,
                ut.planned_end_date,
                ut.updated_at
            FROM validation_context vc
            LEFT JOIN updated_task ut ON ut.id = vc.id
        )SQL";

        const std::optional<int> priorityValue = input.priority
                                                     ? std::make_optional(domain::toInt(*input.priority))
                                                     : std::nullopt;

        try {
            const auto result = co_await executor->execSqlCoro(
                updateTaskBasicInfoSql,
                input.subTaskId,
                input.name,
                input.description,
                input.responsibleUserId,
                priorityValue,
                input.plannedStartDate,
                input.plannedEndDate);

            if (result.empty()) {
                co_return TaskBasicInfoUpdateResult{
                    .task = std::nullopt,
                    .failureReason = std::nullopt
                };
            }

            const auto &row = result.front();

            std::optional<TaskBasicInfoUpdateFailureReason> failureReason;
            if (!row["failure_reason"].isNull()) {
                const auto failureReasonText = row["failure_reason"].as<std::string>();
                if (failureReasonText == "invalid_date_range") {
                    failureReason = TaskBasicInfoUpdateFailureReason::InvalidDateRange;
                } else if (failureReasonText == "responsible_user_invalid") {
                    failureReason = TaskBasicInfoUpdateFailureReason::ResponsibleUserInvalid;
                }
            }

            if (row["id"].isNull()) {
                co_return TaskBasicInfoUpdateResult{
                    .task = std::nullopt,
                    .failureReason = failureReason
                };
            }

            co_return TaskBasicInfoUpdateResult{
                .task = dto::view::UpdatedTaskBasicInfoView{
                    .id = row["id"].as<std::int64_t>(),
                    .name = row["name"].as<std::string>(),
                    .description = row["description"].as<std::string>(),
                    .responsibleUserId = row["responsible_user_id"].as<std::int64_t>(),
                    .priority = static_cast<domain::TaskPriority>(row["priority"].as<int>()),
                    .plannedStartDate = row["planned_start_date"].as<std::string>(),
                    .plannedEndDate = row["planned_end_date"].as<std::string>(),
                    .updatedAt = row["updated_at"].as<std::string>()
                },
                .failureReason = std::nullopt
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<TaskDeleteResult>
    TaskRepository::deleteTask(const common::db::SqlExecutorPtr &executor,
                               const dto::command::DeleteTaskInput &input) const {
        // 查询思路：
        // 1. target_context 先定位子任务所属项目、节点和权限判断所需字段。
        // 2. deleted_task 只在权限和冻结条件都满足时执行真正删除。
        // 3. 最终统一返回 deleted_task_id 或 failure_reason，供 service 映射 HTTP 语义。
        static const std::string deleteTaskSql = R"SQL(
            WITH target_context AS (
                SELECT
                    st.id AS sub_task_id,
                    p.owner_user_id,
                    creator_user.system_role AS creator_user_role,
                    p.status AS project_status,
                    pn.status AS node_status,
                    st.status AS task_status
                FROM sub_task st
                JOIN project_node pn ON pn.id = st.node_id
                JOIN project p ON p.id = pn.project_id
                JOIN sys_user creator_user ON creator_user.id = p.created_by
                WHERE st.id = $1
            ),
            deleted_task AS (
                DELETE FROM sub_task st
                USING target_context tc
                WHERE st.id = tc.sub_task_id AND
                    (
                        ($3 = TRUE AND tc.creator_user_role <> $4) OR
                        ($3 = FALSE AND tc.owner_user_id = $2)
                    ) AND
                    tc.project_status <> $5 AND
                    tc.node_status <> $6 AND
                    tc.task_status <> $7
                RETURNING st.id
            )
            SELECT
                CASE
                    WHEN tc.sub_task_id IS NULL THEN 'not_found'
                    WHEN $3 = TRUE AND tc.creator_user_role = $4 THEN 'forbidden'
                    WHEN $3 = FALSE AND tc.owner_user_id <> $2 THEN 'forbidden'
                    WHEN tc.project_status = $5 THEN 'project_completed'
                    WHEN tc.node_status = $6 THEN 'node_completed'
                    WHEN tc.task_status = $7 THEN 'task_completed'
                    ELSE NULL
                END AS failure_reason,
                dt.id AS deleted_task_id
            FROM (SELECT 1) anchor
            LEFT JOIN target_context tc ON TRUE
            LEFT JOIN deleted_task dt ON TRUE
            LIMIT 1
        )SQL";

        const bool isAdmin = input.operatorUserRole == user_domain::SystemRole::Admin;

        try {
            const auto result = co_await executor->execSqlCoro(
                deleteTaskSql,
                input.subTaskId,
                input.operatorUserId,
                isAdmin,
                user_domain::toInt(user_domain::SystemRole::Employee),
                project_domain::toInt(project_domain::ProjectStatus::Completed),
                project_node_domain::toInt(project_node_domain::ProjectNodeStatus::Completed),
                domain::toInt(domain::TaskStatus::Completed));

            if (result.empty()) {
                co_return TaskDeleteResult{
                    .deletedTaskId = std::nullopt,
                    .failureReason = std::nullopt
                };
            }

            const auto &row = result.front();

            std::optional<TaskDeleteFailureReason> failureReason;
            if (!row["failure_reason"].isNull()) {
                const auto failureReasonText = row["failure_reason"].as<std::string>();
                if (failureReasonText == "not_found") {
                    failureReason = TaskDeleteFailureReason::NotFound;
                } else if (failureReasonText == "forbidden") {
                    failureReason = TaskDeleteFailureReason::Forbidden;
                } else if (failureReasonText == "project_completed") {
                    failureReason = TaskDeleteFailureReason::ProjectCompleted;
                } else if (failureReasonText == "node_completed") {
                    failureReason = TaskDeleteFailureReason::NodeCompleted;
                } else if (failureReasonText == "task_completed") {
                    failureReason = TaskDeleteFailureReason::TaskCompleted;
                }
            }

            std::optional<std::int64_t> deletedTaskId;
            if (!row["deleted_task_id"].isNull()) {
                deletedTaskId = row["deleted_task_id"].as<std::int64_t>();
            }

            co_return TaskDeleteResult{
                .deletedTaskId = deletedTaskId,
                .failureReason = failureReason
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
