#include "modules/project_node/repository/ProjectNodeRepository.h"

#include <string>

#include <drogon/orm/Exception.h>
#include <json/json.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::project_node::repository {
    namespace error = project_tracker::common::error;
    namespace user_domain = modules::user::domain;

    namespace {
        std::string buildApplyTemplateNodesJson(
            const dto::command::ApplyProjectNodeTemplateInput &input) {
            Json::Value nodesJson(Json::arrayValue);

            for (const auto &node : input.nodes) {
                Json::Value nodeJson(Json::objectValue);
                nodeJson["template_node_id"] = node.templateNodeId;
                nodeJson["planned_start_date"] = node.plannedStartDate;
                nodeJson["planned_end_date"] = node.plannedEndDate;
                nodesJson.append(nodeJson);
            }

            Json::StreamWriterBuilder writerBuilder;
            writerBuilder["indentation"] = "";
            return Json::writeString(writerBuilder, nodesJson);
        }
    } // namespace

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

    drogon::Task<std::optional<ProjectNodeDetailResult>>
    ProjectNodeRepository::findProjectNodeDetail(const common::db::SqlExecutorPtr &executor,
                                                 const ProjectNodeDetailQuery &query) const {
        // 查询思路：
        // 1. project_context 先确定项目是否存在，以及当前用户是否可见。
        // 2. task_stats 单独按 node_id 聚合子任务统计，避免在详情主查询里重复聚合。
        // 3. node_detail 只在有权限时按 project_id + node_id 取详情，再把统计结果回连上去。
        static const std::string findProjectNodeDetailSql = R"SQL(
            WITH project_context AS (
                SELECT
                    p.id AS project_id,
                    CASE
                        WHEN $3 = TRUE THEN TRUE
                        WHEN EXISTS (
                            SELECT 1
                            FROM project_member self_pm
                            WHERE self_pm.project_id = p.id AND
                                self_pm.user_id = $4
                        ) THEN TRUE
                        ELSE FALSE
                    END AS has_permission
                FROM project p
                WHERE p.id = $1
            ),
            task_stats AS (
                SELECT
                    st.node_id,
                    COUNT(*) AS sub_task_count,
                    COUNT(*) FILTER (WHERE st.status = 3) AS completed_sub_task_count
                FROM sub_task st
                WHERE st.node_id = $2
                GROUP BY st.node_id
            ),
            node_detail AS (
                SELECT
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
                JOIN project_node pn ON pn.project_id = pc.project_id AND
                    pn.id = $2
                LEFT JOIN task_stats ts ON ts.node_id = pn.id
                WHERE pc.has_permission
            )
            SELECT
                pc.has_permission,
                nd.id,
                nd.project_id,
                nd.name,
                nd.description,
                nd.sequence_no,
                nd.status,
                nd.planned_start_date,
                nd.planned_end_date,
                nd.completed_at,
                nd.created_by,
                nd.created_at,
                nd.updated_at,
                nd.sub_task_count,
                nd.completed_sub_task_count
            FROM project_context pc
            LEFT JOIN node_detail nd ON TRUE
        )SQL";

        const bool isAdmin = query.currentUserRole == user_domain::SystemRole::Admin;

        try {
            const auto result = co_await executor->execSqlCoro(
                findProjectNodeDetailSql,
                query.projectId,
                query.nodeId,
                isAdmin,
                query.currentUserId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            ProjectNodeDetailResult detailResult{
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

            detailResult.detail = dto::view::ProjectNodeDetailView{
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
            };

            co_return detailResult;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<std::int64_t>>
    ProjectNodeRepository::findProjectIdForUpdate(const common::db::SqlExecutorPtr &executor,
                                                  std::int64_t projectId) const {
        static const std::string findProjectIdForUpdateSql = R"SQL(
            SELECT
                p.id
            FROM project p
            WHERE p.id = $1
            FOR UPDATE OF p
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findProjectIdForUpdateSql,
                projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            co_return result.front()["id"].as<std::int64_t>();
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<ProjectNodeCreateCheckResult>>
    ProjectNodeRepository::findProjectNodeCreateCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId) const {
        static const std::string findProjectNodeCreateCheckResultForUpdateSql = R"SQL(
            SELECT
                p.owner_user_id,
                creator_user.system_role AS creator_user_role,
                p.status AS project_status,
                to_char(p.planned_start_date, 'YYYY-MM-DD') AS project_planned_start_date,
                to_char(p.planned_end_date, 'YYYY-MM-DD') AS project_planned_end_date
            FROM project p
            JOIN sys_user creator_user ON creator_user.id = p.created_by
            WHERE p.id = $1
            FOR UPDATE OF p
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findProjectNodeCreateCheckResultForUpdateSql,
                projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return ProjectNodeCreateCheckResult{
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .creatorUserRole = static_cast<user_domain::SystemRole>(
                    row["creator_user_role"].as<int>()),
                .projectStatus = static_cast<project::domain::ProjectStatus>(
                    row["project_status"].as<int>()),
                .projectPlannedStartDate = row["project_planned_start_date"].as<std::string>(),
                .projectPlannedEndDate = row["project_planned_end_date"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<dto::view::CreatedProjectNodeView>
    ProjectNodeRepository::insertProjectNode(
        const common::db::SqlExecutorPtr &executor,
        const dto::command::CreateProjectNodeInput &input) const {
        // 查询思路：
        // 1. sequence_context 先拿到当前项目已有的最大 sequence_no。
        // 2. 新节点的 sequence_no 一律取 max_sequence_no + 1，默认追加到末尾。
        static const std::string insertProjectNodeSql = R"SQL(
            WITH sequence_context AS (
                SELECT
                    COALESCE(MAX(pn.sequence_no), 0) AS max_sequence_no
                FROM project_node pn
                WHERE pn.project_id = $1
            )
            INSERT INTO project_node (
                project_id,
                name,
                description,
                sequence_no,
                status,
                planned_start_date,
                planned_end_date,
                created_by
            )
            SELECT
                $1,
                $2,
                $3,
                sc.max_sequence_no + 1,
                1,
                $4,
                $5,
                $6
            FROM sequence_context sc
            RETURNING
                id,
                project_id,
                name,
                description,
                sequence_no,
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
                insertProjectNodeSql,
                input.projectId,
                input.name,
                input.description,
                input.plannedStartDate,
                input.plannedEndDate,
                input.creatorUserId);

            const auto &row = result.front();

            std::optional<std::string> completedAt;
            if (!row["completed_at"].isNull()) {
                completedAt = row["completed_at"].as<std::string>();
            }

            co_return dto::view::CreatedProjectNodeView{
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
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::UpdatedProjectNodeBasicInfoView>>
    ProjectNodeRepository::updateProjectNodeBasicInfo(
        const common::db::SqlExecutorPtr &executor,
        const dto::command::UpdateProjectNodeBasicInfoInput &input) const {
        // 查询思路：
        // 1. target_context 先算出节点本次更新后的有效名称、说明、起止日期，并带上所属项目日期范围。
        // 2. sub_task_range 单独聚合该节点下全部子任务的最小开始日期和最大结束日期。
        // 3. 最后只在“节点日期合法、落在项目范围内、且能覆盖全部子任务”时执行更新并返回结果。

        static const std::string updateProjectNodeBasicInfoSql = R"SQL(
            WITH target_context AS (
                SELECT
                    pn.id,
                    COALESCE($3, pn.name) AS next_name,
                    COALESCE($4, pn.description) AS next_description,
                    COALESCE($5, pn.planned_start_date) AS next_planned_start_date,
                    COALESCE($6, pn.planned_end_date) AS next_planned_end_date,
                    p.planned_start_date AS project_planned_start_date,
                    p.planned_end_date AS project_planned_end_date
                FROM project_node pn
                JOIN project p ON p.id = pn.project_id
                WHERE p.id = $1 AND
                    pn.id = $2
            ),
            sub_task_range AS (
                SELECT
                    st.node_id,
                    MIN(st.planned_start_date) AS min_planned_start_date,
                    MAX(st.planned_end_date) AS max_planned_end_date
                FROM sub_task st
                WHERE st.node_id = $2
                GROUP BY st.node_id
            )
            UPDATE project_node pn
            SET
                name = tc.next_name,
                description = tc.next_description,
                planned_start_date = tc.next_planned_start_date,
                planned_end_date = tc.next_planned_end_date,
                updated_at = NOW()
            FROM target_context tc
            LEFT JOIN sub_task_range str ON str.node_id = tc.id
            WHERE pn.id = tc.id AND
                tc.next_planned_start_date <= tc.next_planned_end_date AND
                tc.next_planned_start_date >= tc.project_planned_start_date AND
                tc.next_planned_end_date <= tc.project_planned_end_date AND
                (
                    str.node_id IS NULL OR
                    (
                        tc.next_planned_start_date <= str.min_planned_start_date AND
                        tc.next_planned_end_date >= str.max_planned_end_date
                    )
                )
            RETURNING
                pn.id,
                pn.name,
                pn.description,
                to_char(pn.planned_start_date, 'YYYY-MM-DD') AS planned_start_date,
                to_char(pn.planned_end_date, 'YYYY-MM-DD') AS planned_end_date,
                to_char(pn.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                updateProjectNodeBasicInfoSql,
                input.projectId,
                input.nodeId,
                input.name,
                input.description,
                input.plannedStartDate,
                input.plannedEndDate);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return dto::view::UpdatedProjectNodeBasicInfoView{
                .id = row["id"].as<std::int64_t>(),
                .name = row["name"].as<std::string>(),
                .description = row["description"].as<std::string>(),
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

    drogon::Task<std::optional<ProjectNodeStartCheckResult>>
    ProjectNodeRepository::findProjectNodeStartCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId,
        std::int64_t nodeId) const {
        // 查询思路：
        // 1. 项目行已在写事务上一步锁定，这里只锁目标节点，继续带回所属项目上的权限/状态字段。
        // 2. 同时带回项目负责人、项目创建人角色，供 service 做权限判断。
        static const std::string findProjectNodeStartCheckResultForUpdateSql = R"SQL(
            SELECT
                p.owner_user_id,
                creator_user.system_role AS creator_user_role,
                p.status AS project_status,
                pn.status AS node_status
            FROM project p
            JOIN project_node pn ON pn.project_id = p.id
            JOIN sys_user creator_user ON creator_user.id = p.created_by
            WHERE p.id = $1 AND
                pn.id = $2
            FOR UPDATE OF pn
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findProjectNodeStartCheckResultForUpdateSql,
                projectId,
                nodeId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return ProjectNodeStartCheckResult{
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .creatorUserRole = static_cast<user_domain::SystemRole>(
                    row["creator_user_role"].as<int>()),
                .projectStatus = static_cast<project::domain::ProjectStatus>(
                    row["project_status"].as<int>()),
                .nodeStatus = static_cast<domain::ProjectNodeStatus>(
                    row["node_status"].as<int>())
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::UpdatedProjectNodeStatusView>>
    ProjectNodeRepository::updateProjectNodeStatusForStart(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId,
        std::int64_t nodeId) const {
        // -- 按北京时间取“今天”，再和 DATE 类型的 planned_end_date 比较是否已延期。
        static const std::string updateProjectNodeStatusForStartSql = R"SQL(
            UPDATE project_node pn
            SET
                status = CASE
                    WHEN (NOW() AT TIME ZONE 'Asia/Shanghai')::date > pn.planned_end_date THEN 4
                    ELSE 2
                END,
                completed_at = NULL,
                updated_at = NOW()
            WHERE pn.project_id = $1 AND
                pn.id = $2 AND
                pn.status = 1 AND
                NOT EXISTS (
                    SELECT 1
                    FROM sub_task st
                    WHERE st.node_id = pn.id
                )
            RETURNING
                pn.id,
                pn.status,
                to_char(pn.completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD\"T\"HH24:MI:SS') || '+08:00' AS completed_at,
                to_char(pn.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD\"T\"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                updateProjectNodeStatusForStartSql,
                projectId,
                nodeId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();

            std::optional<std::string> completedAt;
            if (!row["completed_at"].isNull()) {
                completedAt = row["completed_at"].as<std::string>();
            }

            co_return dto::view::UpdatedProjectNodeStatusView{
                .id = row["id"].as<std::int64_t>(),
                .status = static_cast<domain::ProjectNodeStatus>(row["status"].as<int>()),
                .completedAt = completedAt,
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::UpdatedProjectNodeStatusView>>
    ProjectNodeRepository::updateProjectNodeStatusForTaskSignal(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t nodeId) const {
        // -- 节点行已在调用方事务中锁定；这里只把仍为未开始的节点推进到节点自己的已开始态。
        static const std::string updateProjectNodeStatusForTaskSignalSql = R"SQL(
            UPDATE project_node pn
            SET
                status = CASE
                    WHEN (NOW() AT TIME ZONE 'Asia/Shanghai')::date > pn.planned_end_date THEN 4
                    ELSE 2
                END,
                completed_at = NULL,
                updated_at = NOW()
            WHERE pn.id = $1 AND
                pn.status = 1
            RETURNING
                pn.id,
                pn.status,
                to_char(pn.completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD\"T\"HH24:MI:SS') || '+08:00' AS completed_at,
                to_char(pn.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD\"T\"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                updateProjectNodeStatusForTaskSignalSql,
                nodeId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();

            std::optional<std::string> completedAt;
            if (!row["completed_at"].isNull()) {
                completedAt = row["completed_at"].as<std::string>();
            }

            co_return dto::view::UpdatedProjectNodeStatusView{
                .id = row["id"].as<std::int64_t>(),
                .status = static_cast<domain::ProjectNodeStatus>(row["status"].as<int>()),
                .completedAt = completedAt,
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<ProjectNodeCompleteCheckResult>>
    ProjectNodeRepository::findProjectNodeCompleteCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId,
        std::int64_t nodeId) const {
        // 查询思路：
        // 1. 项目行已在写事务上一步锁定，这里只锁目标节点，继续带回所属项目上的权限/状态字段。
        // 2. 同时带回项目负责人、项目创建人角色，供 service 做权限判断。
        static const std::string findProjectNodeCompleteCheckResultForUpdateSql = R"SQL(
            SELECT
                p.owner_user_id,
                creator_user.system_role AS creator_user_role,
                p.status AS project_status,
                pn.status AS node_status
            FROM project p
            JOIN project_node pn ON pn.project_id = p.id
            JOIN sys_user creator_user ON creator_user.id = p.created_by
            WHERE p.id = $1 AND
                pn.id = $2
            FOR UPDATE OF pn
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findProjectNodeCompleteCheckResultForUpdateSql,
                projectId,
                nodeId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return ProjectNodeCompleteCheckResult{
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .creatorUserRole = static_cast<user_domain::SystemRole>(
                    row["creator_user_role"].as<int>()),
                .projectStatus = static_cast<project::domain::ProjectStatus>(
                    row["project_status"].as<int>()),
                .nodeStatus = static_cast<domain::ProjectNodeStatus>(
                    row["node_status"].as<int>())
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::UpdatedProjectNodeStatusView>>
    ProjectNodeRepository::updateProjectNodeStatusForComplete(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId,
        std::int64_t nodeId) const {
        // 查询思路：
        // 1. 不在 service 里提前判断子任务是否都完成，避免未加锁的子任务集合在并发下漂移。
        // 2. 最终是否允许完成，统一压进这条原子 UPDATE 的 WHERE 条件。
        static const std::string updateProjectNodeStatusForCompleteSql = R"SQL(
            UPDATE project_node pn
            SET
                status = 3,
                completed_at = NOW(),
                updated_at = NOW()
            WHERE pn.project_id = $1 AND
                pn.id = $2 AND
                pn.status <> 3 AND
                (
                    NOT EXISTS (
                        SELECT 1
                        FROM sub_task st
                        WHERE st.node_id = pn.id
                    ) OR
                    NOT EXISTS (
                        SELECT 1
                        FROM sub_task st
                        WHERE st.node_id = pn.id AND
                            st.status <> 3
                    )
                )
            RETURNING
                pn.id,
                pn.status,
                to_char(pn.completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD\"T\"HH24:MI:SS') || '+08:00' AS completed_at,
                to_char(pn.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD\"T\"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                updateProjectNodeStatusForCompleteSql,
                projectId,
                nodeId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();

            std::optional<std::string> completedAt;
            if (!row["completed_at"].isNull()) {
                completedAt = row["completed_at"].as<std::string>();
            }

            co_return dto::view::UpdatedProjectNodeStatusView{
                .id = row["id"].as<std::int64_t>(),
                .status = static_cast<domain::ProjectNodeStatus>(row["status"].as<int>()),
                .completedAt = completedAt,
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<ProjectNodeReopenCheckResult>>
    ProjectNodeRepository::findProjectNodeReopenCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId,
        std::int64_t nodeId) const {
        // 查询思路：
        // 1. 项目行已在写事务上一步锁定，这里只锁目标节点，继续带回所属项目上的权限/状态字段。
        // 2. 同时带回项目负责人、项目创建人角色，供 service 做权限判断。
        static const std::string findProjectNodeReopenCheckResultForUpdateSql = R"SQL(
            SELECT
                p.owner_user_id,
                creator_user.system_role AS creator_user_role,
                p.status AS project_status,
                pn.status AS node_status
            FROM project p
            JOIN project_node pn ON pn.project_id = p.id
            JOIN sys_user creator_user ON creator_user.id = p.created_by
            WHERE p.id = $1 AND
                pn.id = $2
            FOR UPDATE OF pn
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findProjectNodeReopenCheckResultForUpdateSql,
                projectId,
                nodeId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return ProjectNodeReopenCheckResult{
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .creatorUserRole = static_cast<user_domain::SystemRole>(
                    row["creator_user_role"].as<int>()),
                .projectStatus = static_cast<project::domain::ProjectStatus>(
                    row["project_status"].as<int>()),
                .nodeStatus = static_cast<domain::ProjectNodeStatus>(
                    row["node_status"].as<int>())
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::UpdatedProjectNodeStatusView>>
    ProjectNodeRepository::updateProjectNodeStatusForReopen(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId,
        std::int64_t nodeId) const {
        // 按北京时间先判断是否已延期；当前 schema 没有阶段级开始信号历史，
        // 这里对已完成节点按“已存在开始信号”回退到进行中/已延期。
        static const std::string updateProjectNodeStatusForReopenSql = R"SQL(
            UPDATE project_node pn
            SET
                status = CASE
                    WHEN (NOW() AT TIME ZONE 'Asia/Shanghai')::date > pn.planned_end_date THEN 4
                    ELSE 2
                END,
                completed_at = NULL,
                updated_at = NOW()
            WHERE pn.project_id = $1 AND
                pn.id = $2 AND
                pn.status = 3
            RETURNING
                pn.id,
                pn.status,
                to_char(pn.completed_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD\"T\"HH24:MI:SS') || '+08:00' AS completed_at,
                to_char(pn.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD\"T\"HH24:MI:SS') || '+08:00' AS updated_at
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                updateProjectNodeStatusForReopenSql,
                projectId,
                nodeId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();

            std::optional<std::string> completedAt;
            if (!row["completed_at"].isNull()) {
                completedAt = row["completed_at"].as<std::string>();
            }

            co_return dto::view::UpdatedProjectNodeStatusView{
                .id = row["id"].as<std::int64_t>(),
                .status = static_cast<domain::ProjectNodeStatus>(row["status"].as<int>()),
                .completedAt = completedAt,
                .updatedAt = row["updated_at"].as<std::string>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<ProjectNodeApplyTemplateCheckResult>>
    ProjectNodeRepository::findProjectNodeApplyTemplateCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        const dto::command::ApplyProjectNodeTemplateInput &input) const {
        // 查询思路：
        // 1. 先锁项目行，和创建/删除/重排这类依赖项目状态的写操作串行化。
        // 2. 再把请求体里的模板节点映射转成临时输入表，统一统计输入条数、去重后条数。
        // 3. 最后带回模板状态、模板节点总数、命中节点数，供 service 做“模板是否完整覆盖”的细分判断。
        static const std::string findProjectNodeApplyTemplateCheckResultForUpdateSql = R"SQL(
            WITH raw_input_nodes AS (
                SELECT
                    entry.template_node_id,
                    entry.planned_start_date,
                    entry.planned_end_date
                FROM jsonb_to_recordset($2::jsonb) AS entry(
                    template_node_id BIGINT,
                    planned_start_date TEXT,
                    planned_end_date TEXT
                )
            ),
            project_context AS (
                SELECT
                    p.id AS project_id,
                    p.owner_user_id,
                    creator_user.system_role AS creator_user_role,
                    p.status AS project_status,
                    to_char(p.planned_start_date, 'YYYY-MM-DD') AS project_planned_start_date,
                    to_char(p.planned_end_date, 'YYYY-MM-DD') AS project_planned_end_date
                FROM project p
                JOIN sys_user creator_user ON creator_user.id = p.created_by
                WHERE p.id = $1
                FOR UPDATE OF p
                LIMIT 1
            ),
            project_node_stats AS (
                SELECT
                    COUNT(pn.id) AS project_node_count
                FROM project_context pc
                LEFT JOIN project_node pn ON pn.project_id = pc.project_id
            ),
            input_node_stats AS (
                SELECT
                    COUNT(*) AS input_node_count,
                    COUNT(DISTINCT raw_input_nodes.template_node_id) AS distinct_input_node_count
                FROM raw_input_nodes
            ),
            template_context AS (
                SELECT
                    pt.id AS template_id,
                    pt.status AS template_status
                FROM project_template pt
                WHERE pt.id = $3
                LIMIT 1
            ),
            template_node_stats AS (
                SELECT
                    COUNT(ptn.id) AS template_node_count,
                    COUNT(input_nodes.template_node_id) AS matched_template_node_count
                FROM template_context tc
                LEFT JOIN project_template_node ptn ON ptn.template_id = tc.template_id
                LEFT JOIN (
                    SELECT DISTINCT raw_input_nodes.template_node_id
                    FROM raw_input_nodes
                ) AS input_nodes ON input_nodes.template_node_id = ptn.id
            )
            SELECT
                pc.owner_user_id,
                pc.creator_user_role,
                pc.project_status,
                pc.project_planned_start_date,
                pc.project_planned_end_date,
                pns.project_node_count,
                tc.template_status,
                ins.input_node_count,
                ins.distinct_input_node_count,
                COALESCE(tns.template_node_count, 0) AS template_node_count,
                COALESCE(tns.matched_template_node_count, 0) AS matched_template_node_count
            FROM project_context pc
            CROSS JOIN project_node_stats pns
            CROSS JOIN input_node_stats ins
            LEFT JOIN template_context tc ON TRUE
            LEFT JOIN template_node_stats tns ON TRUE
        )SQL";

        const auto inputNodesJson = buildApplyTemplateNodesJson(input);

        try {
            const auto result = co_await executor->execSqlCoro(
                findProjectNodeApplyTemplateCheckResultForUpdateSql,
                input.projectId,
                inputNodesJson,
                input.templateId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            std::optional<project_template::domain::ProjectTemplateStatus> templateStatus;
            if (!row["template_status"].isNull()) {
                templateStatus = static_cast<project_template::domain::ProjectTemplateStatus>(
                    row["template_status"].as<int>());
            }

            co_return ProjectNodeApplyTemplateCheckResult{
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .creatorUserRole = static_cast<user_domain::SystemRole>(
                    row["creator_user_role"].as<int>()),
                .projectStatus = static_cast<project::domain::ProjectStatus>(
                    row["project_status"].as<int>()),
                .projectPlannedStartDate = row["project_planned_start_date"].as<std::string>(),
                .projectPlannedEndDate = row["project_planned_end_date"].as<std::string>(),
                .projectNodeCount = row["project_node_count"].as<std::int64_t>(),
                .templateStatus = templateStatus,
                .inputNodeCount = row["input_node_count"].as<std::int64_t>(),
                .distinctInputNodeCount = row["distinct_input_node_count"].as<std::int64_t>(),
                .templateNodeCount = row["template_node_count"].as<std::int64_t>(),
                .matchedTemplateNodeCount = row["matched_template_node_count"].as<std::int64_t>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::AppliedProjectNodeTemplateView>>
    ProjectNodeRepository::insertProjectNodesFromTemplate(
        const common::db::SqlExecutorPtr &executor,
        const dto::command::ApplyProjectNodeTemplateInput &input) const {
        // 查询思路：
        // 1. raw_input_nodes 先接住 controller 传下来的模板节点日期映射，避免在 SQL 里手拼 VALUES。
        // 2. validated_context 把“项目当前无节点、模板仍启用、映射完整且无重复、日期仍在项目范围内”压成最终写保护条件。
        // 3. inserted_nodes 再从模板表直接复制 name/description/sequence_no，保证顺序只来源于模板定义。
        static const std::string insertProjectNodesFromTemplateSql = R"SQL(
            WITH raw_input_nodes AS (
                SELECT
                    entry.template_node_id,
                    entry.planned_start_date::date AS planned_start_date,
                    entry.planned_end_date::date AS planned_end_date
                FROM jsonb_to_recordset($4::jsonb) AS entry(
                    template_node_id BIGINT,
                    planned_start_date TEXT,
                    planned_end_date TEXT
                )
            ),
            project_context AS (
                SELECT
                    p.id AS project_id,
                    p.planned_start_date AS project_planned_start_date,
                    p.planned_end_date AS project_planned_end_date
                FROM project p
                WHERE p.id = $1
                LIMIT 1
            ),
            input_node_stats AS (
                SELECT
                    COUNT(*) AS input_node_count,
                    COUNT(DISTINCT raw_input_nodes.template_node_id) AS distinct_input_node_count
                FROM raw_input_nodes
            ),
            template_node_stats AS (
                SELECT
                    COUNT(ptn.id) AS template_node_count,
                    COUNT(input_nodes.template_node_id) AS matched_template_node_count
                FROM project_template_node ptn
                LEFT JOIN (
                    SELECT DISTINCT raw_input_nodes.template_node_id
                    FROM raw_input_nodes
                ) AS input_nodes ON input_nodes.template_node_id = ptn.id
                WHERE ptn.template_id = $2
            ),
            validated_context AS (
                SELECT
                    pc.project_id
                FROM project_context pc
                JOIN project_template pt ON pt.id = $2
                CROSS JOIN input_node_stats ins
                CROSS JOIN template_node_stats tns
                WHERE pt.status = 1 AND
                    NOT EXISTS (
                        SELECT 1
                        FROM project_node pn
                        WHERE pn.project_id = pc.project_id
                    ) AND
                    ins.input_node_count = ins.distinct_input_node_count AND
                    ins.input_node_count = tns.template_node_count AND
                    tns.matched_template_node_count = tns.template_node_count AND
                    NOT EXISTS (
                        SELECT 1
                        FROM raw_input_nodes rin
                        WHERE rin.planned_start_date > rin.planned_end_date OR
                            rin.planned_start_date < pc.project_planned_start_date OR
                            rin.planned_end_date > pc.project_planned_end_date
                    )
            ),
            inserted_nodes AS (
                INSERT INTO project_node (
                    project_id,
                    name,
                    description,
                    sequence_no,
                    status,
                    planned_start_date,
                    planned_end_date,
                    created_by
                )
                SELECT
                    vc.project_id,
                    ptn.name,
                    ptn.description,
                    ptn.sequence_no,
                    1,
                    rin.planned_start_date,
                    rin.planned_end_date,
                    $3
                FROM validated_context vc
                JOIN project_template_node ptn ON ptn.template_id = $2
                JOIN raw_input_nodes rin ON rin.template_node_id = ptn.id
                ORDER BY ptn.sequence_no ASC, ptn.id ASC
                RETURNING
                    id,
                    project_id,
                    name,
                    sequence_no,
                    to_char(planned_start_date, 'YYYY-MM-DD') AS planned_start_date,
                    to_char(planned_end_date, 'YYYY-MM-DD') AS planned_end_date
            )
            SELECT
                inserted_nodes.id,
                inserted_nodes.project_id,
                inserted_nodes.name,
                inserted_nodes.sequence_no,
                inserted_nodes.planned_start_date,
                inserted_nodes.planned_end_date
            FROM inserted_nodes
            ORDER BY inserted_nodes.sequence_no ASC, inserted_nodes.id ASC
        )SQL";

        const auto inputNodesJson = buildApplyTemplateNodesJson(input);

        try {
            const auto result = co_await executor->execSqlCoro(
                insertProjectNodesFromTemplateSql,
                input.projectId,
                input.templateId,
                input.operatorUserId,
                inputNodesJson);

            if (result.empty()) {
                co_return std::nullopt;
            }

            dto::view::AppliedProjectNodeTemplateView view{
                .projectId = result.front()["project_id"].as<std::int64_t>(),
                .templateId = input.templateId,
                .generatedNodes = {}
            };
            view.generatedNodes.reserve(result.size());

            for (const auto &row : result) {
                view.generatedNodes.push_back(dto::view::AppliedProjectNodeTemplateItemView{
                    .id = row["id"].as<std::int64_t>(),
                    .name = row["name"].as<std::string>(),
                    .sequenceNo = row["sequence_no"].as<int>(),
                    .plannedStartDate = row["planned_start_date"].as<std::string>(),
                    .plannedEndDate = row["planned_end_date"].as<std::string>()
                });
            }

            co_return view;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<ProjectNodeBasicInfoUpdateCheckResult>>
    ProjectNodeRepository::findProjectNodeBasicInfoUpdateCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId,
        std::int64_t nodeId) const {
        // 查询思路：
        // 1. 项目行已在写事务上一步锁定，这里只锁目标节点，继续带回所属项目上的权限/状态字段。
        // 2. 同时带回项目负责人、项目创建人角色，供 service 做权限判断。
        static const std::string findProjectNodeBasicInfoUpdateCheckResultForUpdateSql = R"SQL(
            SELECT
                p.owner_user_id,
                creator_user.system_role AS creator_user_role,
                p.status AS project_status,
                pn.status AS node_status
            FROM project p
            JOIN project_node pn ON pn.project_id = p.id
            JOIN sys_user creator_user ON creator_user.id = p.created_by
            WHERE p.id = $1 AND
                pn.id = $2
            FOR UPDATE OF pn
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findProjectNodeBasicInfoUpdateCheckResultForUpdateSql,
                projectId,
                nodeId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return ProjectNodeBasicInfoUpdateCheckResult{
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .creatorUserRole = static_cast<user_domain::SystemRole>(
                    row["creator_user_role"].as<int>()),
                .projectStatus = static_cast<project::domain::ProjectStatus>(
                    row["project_status"].as<int>()),
                .nodeStatus = static_cast<domain::ProjectNodeStatus>(
                    row["node_status"].as<int>())
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<ProjectNodeDeleteCheckResult>>
    ProjectNodeRepository::findProjectNodeDeleteCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        std::int64_t projectId,
        std::int64_t nodeId) const {
        // 查询思路：
        // 1. 项目行已在写事务上一步锁定，这里只锁目标节点，继续带回所属项目上的权限/状态字段。
        // 2. 同时带回节点当前 sequence_no，供后续删除后补齐顺序号使用。
        static const std::string findProjectNodeDeleteCheckResultForUpdateSql = R"SQL(
            SELECT
                p.owner_user_id,
                creator_user.system_role AS creator_user_role,
                p.status AS project_status,
                pn.status AS node_status,
                pn.sequence_no
            FROM project p
            JOIN project_node pn ON pn.project_id = p.id
            JOIN sys_user creator_user ON creator_user.id = p.created_by
            WHERE p.id = $1 AND
                pn.id = $2
            FOR UPDATE OF pn
            LIMIT 1
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findProjectNodeDeleteCheckResultForUpdateSql,
                projectId,
                nodeId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return ProjectNodeDeleteCheckResult{
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .creatorUserRole = static_cast<user_domain::SystemRole>(
                    row["creator_user_role"].as<int>()),
                .projectStatus = static_cast<project::domain::ProjectStatus>(
                    row["project_status"].as<int>()),
                .nodeStatus = static_cast<domain::ProjectNodeStatus>(
                    row["node_status"].as<int>()),
                .sequenceNo = row["sequence_no"].as<int>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<bool>
    ProjectNodeRepository::deleteProjectNode(
        const common::db::SqlExecutorPtr &executor,
        const dto::command::DeleteProjectNodeInput &input) const {
        // 查询思路：
        // 1. deleted_node 先删除目标节点，并取回它原来的 sequence_no。
        // 2. shifted_nodes 再把同项目中排在它后面的节点统一前移一位。
        // 3. 整个删除与补位都收敛在同一条 statement 里，避免中间出现顺序空洞。
        static const std::string deleteProjectNodeSql = R"SQL(
            WITH deleted_node AS (
                DELETE FROM project_node pn
                WHERE pn.project_id = $1 AND
                    pn.id = $2
                RETURNING
                    pn.project_id,
                    pn.id,
                    pn.sequence_no
            ),
            shifted_nodes AS (
                UPDATE project_node pn
                SET
                    sequence_no = pn.sequence_no - 1,
                    updated_at = NOW()
                FROM deleted_node dn
                WHERE pn.project_id = dn.project_id AND
                    pn.sequence_no > dn.sequence_no
                RETURNING pn.id
            )
            SELECT
                dn.id
            FROM deleted_node dn
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                deleteProjectNodeSql,
                input.projectId,
                input.nodeId);

            co_return !result.empty();
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<ProjectNodeReorderCheckResult>>
    ProjectNodeRepository::findProjectNodeReorderCheckResultForUpdate(
        const common::db::SqlExecutorPtr &executor,
        const dto::command::ReorderProjectNodesInput &input) const {
        std::string inputNodeValuesSql;
        inputNodeValuesSql.reserve(input.nodes.size() * 32);
        for (std::size_t index = 0; index < input.nodes.size(); ++index) {
            if (index > 0) {
                inputNodeValuesSql += ", ";
            }
            inputNodeValuesSql += "(" + std::to_string(input.nodes[index].nodeId) + ")";
        }

        // 查询思路：
        // 1. 先锁项目行，和创建/删除/完成这类依赖项目状态的写操作串行化。
        // 2. 再锁当前项目下全部节点，冻结本次重排期间的节点集合与节点状态。
        // 3. 最后统计项目当前节点总数、请求命中的节点数、以及已完成节点数，供 service 做对账判断。
        const std::string findProjectNodeReorderCheckResultForUpdateSql = R"SQL(
            WITH input_nodes AS (
                SELECT DISTINCT raw_input_nodes.node_id
                FROM (VALUES
        )SQL" + inputNodeValuesSql + R"SQL(
                ) AS raw_input_nodes(node_id)
            ),
            project_context AS (
                SELECT
                    p.id AS project_id,
                    p.owner_user_id,
                    creator_user.system_role AS creator_user_role,
                    p.status AS project_status
                FROM project p
                JOIN sys_user creator_user ON creator_user.id = p.created_by
                WHERE p.id = $1
                FOR UPDATE OF p
                LIMIT 1
            ),
            project_nodes AS (
                SELECT
                    pn.id,
                    pn.status
                FROM project_context pc
                JOIN project_node pn ON pn.project_id = pc.project_id
                FOR UPDATE OF pn
            ),
            node_stats AS (
                SELECT
                    COUNT(pn.id) AS total_node_count,
                    COUNT(input_nodes.node_id) AS matched_node_count,
                    COUNT(pn.id) FILTER (WHERE pn.status = 3) AS completed_node_count
                FROM project_nodes pn
                LEFT JOIN input_nodes ON input_nodes.node_id = pn.id
            )
            SELECT
                pc.owner_user_id,
                pc.creator_user_role,
                pc.project_status,
                ns.total_node_count,
                ns.matched_node_count,
                ns.completed_node_count
            FROM project_context pc
            CROSS JOIN node_stats ns
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                findProjectNodeReorderCheckResultForUpdateSql,
                input.projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            const auto &row = result.front();
            co_return ProjectNodeReorderCheckResult{
                .ownerUserId = row["owner_user_id"].as<std::int64_t>(),
                .creatorUserRole = static_cast<user_domain::SystemRole>(
                    row["creator_user_role"].as<int>()),
                .projectStatus = static_cast<project::domain::ProjectStatus>(
                    row["project_status"].as<int>()),
                .totalNodeCount = row["total_node_count"].as<std::int64_t>(),
                .matchedNodeCount = row["matched_node_count"].as<std::int64_t>(),
                .completedNodeCount = row["completed_node_count"].as<std::int64_t>()
            };
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }

    drogon::Task<std::optional<dto::view::ReorderedProjectNodesView>>
    ProjectNodeRepository::reorderProjectNodes(
        const common::db::SqlExecutorPtr &executor,
        const dto::command::ReorderProjectNodesInput &input) const {
        std::string inputNodeValuesSql;
        inputNodeValuesSql.reserve(input.nodes.size() * 48);
        for (std::size_t index = 0; index < input.nodes.size(); ++index) {
            if (index > 0) {
                inputNodeValuesSql += ", ";
            }
            inputNodeValuesSql.push_back('(');
            inputNodeValuesSql += std::to_string(input.nodes[index].nodeId);
            inputNodeValuesSql += ", ";
            inputNodeValuesSql += std::to_string(input.nodes[index].sequenceNo);
            inputNodeValuesSql.push_back(')');
        }

        // 查询思路：
        // 1. input_nodes 把前端提交的完整节点顺序集合转成临时输入表。
        // 2. updated_nodes 用单条 UPDATE ... FROM 批量改写当前项目全部节点的 sequence_no。
        // 3. 最后按新的 sequence_no 排序返回，供 controller 直接组织响应。
        const std::string reorderProjectNodesSql = R"SQL(
            WITH input_nodes(node_id, sequence_no) AS (
                VALUES
        )SQL" + inputNodeValuesSql + R"SQL(
            ),
            updated_nodes AS (
                UPDATE project_node pn
                SET
                    sequence_no = input_nodes.sequence_no,
                    updated_at = NOW()
                FROM input_nodes
                WHERE pn.project_id = $1 AND
                    pn.id = input_nodes.node_id
                RETURNING
                    pn.project_id,
                    pn.id AS node_id,
                    pn.sequence_no,
                    to_char(pn.updated_at AT TIME ZONE 'Asia/Shanghai', 'YYYY-MM-DD"T"HH24:MI:SS') || '+08:00' AS updated_at
            )
            SELECT
                un.project_id,
                un.node_id,
                un.sequence_no,
                un.updated_at
            FROM updated_nodes un
            ORDER BY un.sequence_no ASC, un.node_id ASC
        )SQL";

        try {
            const auto result = co_await executor->execSqlCoro(
                reorderProjectNodesSql,
                input.projectId);

            if (result.empty()) {
                co_return std::nullopt;
            }

            dto::view::ReorderedProjectNodesView view{
                .projectId = result.front()["project_id"].as<std::int64_t>(),
                .nodes = {},
                .updatedAt = result.front()["updated_at"].as<std::string>()
            };
            view.nodes.reserve(result.size());

            for (const auto &row : result) {
                view.nodes.push_back(dto::view::ReorderedProjectNodeItemView{
                    .nodeId = row["node_id"].as<std::int64_t>(),
                    .sequenceNo = row["sequence_no"].as<int>()
                });
            }

            co_return view;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库操作失败");
        }
    }
} // namespace project_tracker::modules::project_node::repository
