#include "modules/project_node/controller/ProjectNodeController.h"

#include <regex>
#include <unordered_set>

#include <drogon/drogon.h>

#include "common/api/ApiResponse.h"
#include "common/error/BusinessException.h"
#include "common/error/ErrorCode.h"
#include "common/util/JsonUtil.h"

namespace project_tracker::modules::project_node::controller {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;
    namespace project_domain = modules::project::domain;
    namespace task_domain = modules::task::domain;
    namespace user_domain = modules::user::domain;
    namespace util = project_tracker::common::util;

    namespace {
        Json::Value buildProjectNodeJson(const dto::view::ProjectNodeListItemView &node) {
            Json::Value json(Json::objectValue);
            json["id"] = node.id;
            json["project_id"] = node.projectId;
            json["name"] = node.name;
            json["description"] = node.description;
            json["sequence_no"] = node.sequenceNo;
            json["status"] = domain::toInt(node.status);
            json["planned_start_date"] = node.plannedStartDate;
            json["planned_end_date"] = node.plannedEndDate;
            if (node.completedAt) {
                json["completed_at"] = *node.completedAt;
            } else {
                json["completed_at"] = Json::Value(Json::nullValue);
            }
            json["created_by"] = node.createdBy;
            json["created_at"] = node.createdAt;
            json["updated_at"] = node.updatedAt;
            json["sub_task_count"] = node.subTaskCount;
            json["completed_sub_task_count"] = node.completedSubTaskCount;

            return json;
        }

        Json::Value buildProjectNodeJson(const dto::view::ProjectNodeDetailView &node) {
            Json::Value json(Json::objectValue);
            json["id"] = node.id;
            json["project_id"] = node.projectId;
            json["name"] = node.name;
            json["description"] = node.description;
            json["sequence_no"] = node.sequenceNo;
            json["status"] = domain::toInt(node.status);
            json["planned_start_date"] = node.plannedStartDate;
            json["planned_end_date"] = node.plannedEndDate;
            if (node.completedAt) {
                json["completed_at"] = *node.completedAt;
            } else {
                json["completed_at"] = Json::Value(Json::nullValue);
            }
            json["created_by"] = node.createdBy;
            json["created_at"] = node.createdAt;
            json["updated_at"] = node.updatedAt;
            json["sub_task_count"] = node.subTaskCount;
            json["completed_sub_task_count"] = node.completedSubTaskCount;

            return json;
        }

        Json::Value buildProjectNodeGanttProjectJson(
            const dto::view::ProjectNodeGanttProjectView &project) {
            Json::Value json(Json::objectValue);
            json["id"] = project.id;
            json["name"] = project.name;
            json["owner_user_id"] = project.ownerUserId;
            json["owner_real_name"] = project.ownerRealName;
            json["status"] = project_domain::toInt(project.status);
            json["planned_start_date"] = project.plannedStartDate;
            json["planned_end_date"] = project.plannedEndDate;
            if (project.completedAt) {
                json["completed_at"] = *project.completedAt;
            } else {
                json["completed_at"] = Json::Value(Json::nullValue);
            }

            return json;
        }

        Json::Value buildProjectNodeGanttNodeJson(const dto::view::ProjectNodeGanttNodeView &node) {
            Json::Value json(Json::objectValue);
            json["id"] = node.id;
            json["name"] = node.name;
            json["sequence_no"] = node.sequenceNo;
            json["status"] = domain::toInt(node.status);
            json["planned_start_date"] = node.plannedStartDate;
            json["planned_end_date"] = node.plannedEndDate;
            if (node.completedAt) {
                json["completed_at"] = *node.completedAt;
            } else {
                json["completed_at"] = Json::Value(Json::nullValue);
            }

            return json;
        }

        Json::Value buildProjectNodeGanttTaskJson(
            const dto::view::ProjectNodeGanttTaskItemView &task) {
            Json::Value json(Json::objectValue);
            json["id"] = task.id;
            json["node_id"] = task.nodeId;
            json["node_name"] = task.nodeName;
            json["name"] = task.name;
            json["responsible_user_id"] = task.responsibleUserId;
            json["responsible_real_name"] = task.responsibleRealName;
            json["status"] = task_domain::toInt(task.status);
            json["progress_percent"] = task.progressPercent;
            json["priority"] = task_domain::toInt(task.priority);
            json["planned_start_date"] = task.plannedStartDate;
            json["planned_end_date"] = task.plannedEndDate;
            if (task.completedAt) {
                json["completed_at"] = *task.completedAt;
            } else {
                json["completed_at"] = Json::Value(Json::nullValue);
            }

            return json;
        }

        Json::Value buildUpdatedProjectNodeBasicInfoJson(
            const dto::view::UpdatedProjectNodeBasicInfoView &node) {
            Json::Value json(Json::objectValue);
            json["id"] = node.id;
            json["name"] = node.name;
            json["description"] = node.description;
            json["planned_start_date"] = node.plannedStartDate;
            json["planned_end_date"] = node.plannedEndDate;
            json["updated_at"] = node.updatedAt;

            return json;
        }

        Json::Value buildUpdatedProjectNodeStatusJson(
            const dto::view::UpdatedProjectNodeStatusView &node) {
            Json::Value json(Json::objectValue);
            json["id"] = node.id;
            json["status"] = domain::toInt(node.status);
            if (node.completedAt) {
                json["completed_at"] = *node.completedAt;
            } else {
                json["completed_at"] = Json::Value(Json::nullValue);
            }
            json["updated_at"] = node.updatedAt;

            return json;
        }

        Json::Value buildCreatedProjectNodeJson(
            const dto::view::CreatedProjectNodeView &node) {
            Json::Value json(Json::objectValue);
            json["id"] = node.id;
            json["project_id"] = node.projectId;
            json["name"] = node.name;
            json["description"] = node.description;
            json["sequence_no"] = node.sequenceNo;
            json["status"] = domain::toInt(node.status);
            json["planned_start_date"] = node.plannedStartDate;
            json["planned_end_date"] = node.plannedEndDate;
            if (node.completedAt) {
                json["completed_at"] = *node.completedAt;
            } else {
                json["completed_at"] = Json::Value(Json::nullValue);
            }
            json["created_by"] = node.createdBy;
            json["created_at"] = node.createdAt;
            json["updated_at"] = node.updatedAt;

            return json;
        }

        Json::Value buildAppliedProjectNodeTemplateJson(
            const dto::view::AppliedProjectNodeTemplateView &result) {
            Json::Value json(Json::objectValue);
            json["project_id"] = result.projectId;
            json["template_id"] = result.templateId;
            json["generated_nodes"] = Json::Value(Json::arrayValue);

            for (const auto &node : result.generatedNodes) {
                Json::Value nodeJson(Json::objectValue);
                nodeJson["id"] = node.id;
                nodeJson["name"] = node.name;
                nodeJson["sequence_no"] = node.sequenceNo;
                nodeJson["planned_start_date"] = node.plannedStartDate;
                nodeJson["planned_end_date"] = node.plannedEndDate;
                json["generated_nodes"].append(nodeJson);
            }

            return json;
        }

        Json::Value buildReorderedProjectNodesJson(
            const dto::view::ReorderedProjectNodesView &result) {
            Json::Value json(Json::objectValue);
            json["project_id"] = result.projectId;
            json["nodes"] = Json::Value(Json::arrayValue);

            for (const auto &node : result.nodes) {
                Json::Value nodeJson(Json::objectValue);
                nodeJson["node_id"] = node.nodeId;
                nodeJson["sequence_no"] = node.sequenceNo;
                json["nodes"].append(nodeJson);
            }

            json["updated_at"] = result.updatedAt;
            return json;
        }

        bool isValidDateString(const std::string &value) {
            static const std::regex dateRegex(R"(^\d{4}-\d{2}-\d{2}$)");
            return std::regex_match(value, dateRegex);
        }
    } // namespace

    drogon::Task<drogon::HttpResponsePtr>
    ProjectNodeController::createProjectNode(drogon::HttpRequestPtr request,
                                             std::int64_t projectId) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");
        const auto systemRole = session->getOptional<user_domain::SystemRole>("system_role");

        if (!userId || *userId <= 0 || !systemRole) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        if (projectId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "project_id 必须是大于 0 的整数");
        }

        const auto &json = request->getJsonObject();
        if (!json || !json->isObject()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "请求体必须是 JSON 对象");
        }

        dto::command::CreateProjectNodeInput input{
            .projectId = projectId,
            .creatorUserId = *userId,
            .creatorUserRole = *systemRole
        };

        if (!util::readRequiredString(*json, "name", input.name)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "name 必须是非空字符串");
        }

        if (!util::readOptionalString(*json, "description", input.description)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "description 必须是字符串");
        }

        if (!util::readRequiredString(*json, "planned_start_date", input.plannedStartDate)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "planned_start_date 必须是非空字符串");
        }
        if (!isValidDateString(input.plannedStartDate)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "planned_start_date 必须是 YYYY-MM-DD 格式");
        }

        if (!util::readRequiredString(*json, "planned_end_date", input.plannedEndDate)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "planned_end_date 必须是非空字符串");
        }
        if (!isValidDateString(input.plannedEndDate)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "planned_end_date 必须是 YYYY-MM-DD 格式");
        }

        try {
            const auto node = co_await projectNodeService_.createProjectNode(input);
            co_return api::ok(buildCreatedProjectNodeJson(node));
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectNodeController::applyProjectNodeTemplate(drogon::HttpRequestPtr request,
                                                    std::int64_t projectId) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");
        const auto systemRole = session->getOptional<user_domain::SystemRole>("system_role");

        if (!userId || *userId <= 0 || !systemRole) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        if (projectId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "project_id 必须是大于 0 的整数");
        }

        const auto &json = request->getJsonObject();
        if (!json || !json->isObject()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "请求体必须是 JSON 对象");
        }

        dto::command::ApplyProjectNodeTemplateInput input{
            .projectId = projectId,
            .operatorUserId = *userId,
            .operatorUserRole = *systemRole
        };

        if (!util::readRequiredInt64(*json, "template_id", input.templateId) ||
            input.templateId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "template_id 必须是大于 0 的整数");
        }

        if (!json->isMember("nodes") || !(*json)["nodes"].isArray()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "nodes 必须是数组");
        }

        const Json::Value &nodesJson = (*json)["nodes"];
        if (nodesJson.empty()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "nodes 不能为空数组");
        }

        input.nodes.reserve(nodesJson.size());

        for (Json::ArrayIndex index = 0; index < nodesJson.size(); ++index) {
            const Json::Value &nodeJson = nodesJson[index];
            if (!nodeJson.isObject()) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "nodes 中的每一项都必须是 JSON 对象");
            }

            dto::command::ApplyProjectNodeTemplateItem nodeInput{};
            if (!util::readRequiredInt64(nodeJson, "template_node_id", nodeInput.templateNodeId) ||
                nodeInput.templateNodeId <= 0) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "nodes[].template_node_id 必须是大于 0 的整数");
            }

            if (!util::readRequiredString(nodeJson, "planned_start_date", nodeInput.plannedStartDate)) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "nodes[].planned_start_date 必须是非空字符串");
            }
            if (!isValidDateString(nodeInput.plannedStartDate)) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "nodes[].planned_start_date 必须是 YYYY-MM-DD 格式");
            }

            if (!util::readRequiredString(nodeJson, "planned_end_date", nodeInput.plannedEndDate)) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "nodes[].planned_end_date 必须是非空字符串");
            }
            if (!isValidDateString(nodeInput.plannedEndDate)) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "nodes[].planned_end_date 必须是 YYYY-MM-DD 格式");
            }

            input.nodes.push_back(std::move(nodeInput));
        }

        try {
            const auto result = co_await projectNodeService_.applyProjectNodeTemplate(input);
            co_return api::ok(buildAppliedProjectNodeTemplateJson(result));
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectNodeController::listProjectNodes(drogon::HttpRequestPtr request,
                                            std::int64_t projectId) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");
        const auto systemRole = session->getOptional<user_domain::SystemRole>("system_role");

        if (!userId || *userId <= 0 || !systemRole) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        if (projectId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "project_id 必须是大于 0 的整数");
        }

        try {
            const auto dbClient = drogon::app().getDbClient();
            const auto result = co_await projectNodeRepository_.listProjectNodes(
                dbClient,
                repository::ProjectNodeListQuery{
                    .projectId = projectId,
                    .currentUserId = *userId,
                    .currentUserRole = *systemRole
                });

            if (!result) {
                co_return api::fail(
                    drogon::k404NotFound,
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            if (!result->hasPermission) {
                co_return api::fail(
                    drogon::k403Forbidden,
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目成员");
            }

            Json::Value data(Json::objectValue);
            data["list"] = Json::Value(Json::arrayValue);
            for (const auto &node : result->list) {
                data["list"].append(buildProjectNodeJson(node));
            }

            co_return api::ok(data);
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectNodeController::getProjectNodeDetail(drogon::HttpRequestPtr request,
                                                std::int64_t projectId,
                                                std::int64_t nodeId) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");
        const auto systemRole = session->getOptional<user_domain::SystemRole>("system_role");

        if (!userId || *userId <= 0 || !systemRole) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        if (projectId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "project_id 必须是大于 0 的整数");
        }

        if (nodeId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "node_id 必须是大于 0 的整数");
        }

        try {
            const auto dbClient = drogon::app().getDbClient();
            const auto result = co_await projectNodeRepository_.findProjectNodeDetail(
                dbClient,
                repository::ProjectNodeDetailQuery{
                    .projectId = projectId,
                    .nodeId = nodeId,
                    .currentUserId = *userId,
                    .currentUserRole = *systemRole
                });

            if (!result) {
                co_return api::fail(
                    drogon::k404NotFound,
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            if (!result->hasPermission) {
                co_return api::fail(
                    drogon::k403Forbidden,
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目成员");
            }

            if (!result->detail) {
                co_return api::fail(
                    drogon::k404NotFound,
                    error::ErrorCode::PhaseNotFound,
                    "阶段节点不存在");
            }

            co_return api::ok(buildProjectNodeJson(*result->detail));
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectNodeController::getProjectNodeGantt(drogon::HttpRequestPtr request,
                                               std::int64_t projectId,
                                               std::int64_t nodeId) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");
        const auto systemRole = session->getOptional<user_domain::SystemRole>("system_role");

        if (!userId || *userId <= 0 || !systemRole) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        if (projectId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "project_id 必须是大于 0 的整数");
        }

        if (nodeId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "node_id 必须是大于 0 的整数");
        }

        try {
            const auto dbClient = drogon::app().getDbClient();
            const auto result = co_await projectNodeRepository_.findProjectNodeGantt(
                dbClient,
                repository::ProjectNodeGanttQuery{
                    .projectId = projectId,
                    .nodeId = nodeId,
                    .currentUserId = *userId,
                    .currentUserRole = *systemRole
                });

            if (!result) {
                co_return api::fail(
                    drogon::k404NotFound,
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            if (!result->hasPermission) {
                co_return api::fail(
                    drogon::k403Forbidden,
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目成员");
            }

            if (!result->detail) {
                co_return api::fail(
                    drogon::k404NotFound,
                    error::ErrorCode::PhaseNotFound,
                    "阶段节点不存在");
            }

            Json::Value data(Json::objectValue);
            data["project"] = buildProjectNodeGanttProjectJson(result->detail->project);
            data["node"] = buildProjectNodeGanttNodeJson(result->detail->node);
            data["subtasks"] = Json::Value(Json::arrayValue);
            for (const auto &task : result->detail->subtasks) {
                data["subtasks"].append(buildProjectNodeGanttTaskJson(task));
            }

            co_return api::ok(data);
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectNodeController::updateProjectNodeBasicInfo(drogon::HttpRequestPtr request,
                                                      std::int64_t projectId,
                                                      std::int64_t nodeId) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");
        const auto systemRole = session->getOptional<user_domain::SystemRole>("system_role");

        if (!userId || *userId <= 0 || !systemRole) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        if (projectId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "project_id 必须是大于 0 的整数");
        }

        if (nodeId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "node_id 必须是大于 0 的整数");
        }

        const auto &json = request->getJsonObject();
        if (!json || !json->isObject()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "请求体必须是 JSON 对象");
        }

        dto::command::UpdateProjectNodeBasicInfoInput input{
            .projectId = projectId,
            .nodeId = nodeId,
            .operatorUserId = *userId,
            .operatorUserRole = *systemRole
        };

        if (!util::readOptionalString(*json, "name", input.name)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "name 必须是字符串");
        }
        if (input.name && input.name->empty()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "name 不能为空字符串");
        }

        if (!util::readOptionalString(*json, "description", input.description)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "description 必须是字符串");
        }

        if (!util::readOptionalString(*json, "planned_start_date", input.plannedStartDate)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "planned_start_date 必须是字符串");
        }
        if (input.plannedStartDate && input.plannedStartDate->empty()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "planned_start_date 不能为空字符串");
        }
        if (input.plannedStartDate && !isValidDateString(*input.plannedStartDate)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "planned_start_date 必须是 YYYY-MM-DD 格式");
        }

        if (!util::readOptionalString(*json, "planned_end_date", input.plannedEndDate)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "planned_end_date 必须是字符串");
        }
        if (input.plannedEndDate && input.plannedEndDate->empty()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "planned_end_date 不能为空字符串");
        }
        if (input.plannedEndDate && !isValidDateString(*input.plannedEndDate)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "planned_end_date 必须是 YYYY-MM-DD 格式");
        }

        if (!input.name && !input.description &&
            !input.plannedStartDate && !input.plannedEndDate) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "至少需要提供一个可修改字段");
        }

        try {
            const auto node = co_await projectNodeService_.updateProjectNodeBasicInfo(input);
            co_return api::ok(buildUpdatedProjectNodeBasicInfoJson(node));
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectNodeController::startProjectNode(drogon::HttpRequestPtr request,
                                            std::int64_t projectId,
                                            std::int64_t nodeId) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");
        const auto systemRole = session->getOptional<user_domain::SystemRole>("system_role");

        if (!userId || *userId <= 0 || !systemRole) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        if (projectId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "project_id 必须是大于 0 的整数");
        }

        if (nodeId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "node_id 必须是大于 0 的整数");
        }

        const auto &json = request->getJsonObject();
        if (!json || !json->isObject()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "请求体必须是 JSON 对象");
        }

        try {
            const auto node = co_await projectNodeService_.startProjectNode(
                dto::command::ProjectNodeStatusActionInput{
                    .projectId = projectId,
                    .nodeId = nodeId,
                    .operatorUserId = *userId,
                    .operatorUserRole = *systemRole
                });

            co_return api::ok(buildUpdatedProjectNodeStatusJson(node));
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectNodeController::completeProjectNode(drogon::HttpRequestPtr request,
                                               std::int64_t projectId,
                                               std::int64_t nodeId) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");
        const auto systemRole = session->getOptional<user_domain::SystemRole>("system_role");

        if (!userId || *userId <= 0 || !systemRole) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        if (projectId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "project_id 必须是大于 0 的整数");
        }

        if (nodeId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "node_id 必须是大于 0 的整数");
        }

        const auto &json = request->getJsonObject();
        if (!json || !json->isObject()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "请求体必须是 JSON 对象");
        }

        try {
            const auto node = co_await projectNodeService_.completeProjectNode(
                dto::command::ProjectNodeStatusActionInput{
                    .projectId = projectId,
                    .nodeId = nodeId,
                    .operatorUserId = *userId,
                    .operatorUserRole = *systemRole
                });

            co_return api::ok(buildUpdatedProjectNodeStatusJson(node));
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectNodeController::reopenProjectNode(drogon::HttpRequestPtr request,
                                             std::int64_t projectId,
                                             std::int64_t nodeId) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");
        const auto systemRole = session->getOptional<user_domain::SystemRole>("system_role");

        if (!userId || *userId <= 0 || !systemRole) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        if (projectId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "project_id 必须是大于 0 的整数");
        }

        if (nodeId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "node_id 必须是大于 0 的整数");
        }

        const auto &json = request->getJsonObject();
        if (!json || !json->isObject()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "请求体必须是 JSON 对象");
        }

        try {
            const auto node = co_await projectNodeService_.reopenProjectNode(
                dto::command::ProjectNodeStatusActionInput{
                    .projectId = projectId,
                    .nodeId = nodeId,
                    .operatorUserId = *userId,
                    .operatorUserRole = *systemRole
                });

            co_return api::ok(buildUpdatedProjectNodeStatusJson(node));
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectNodeController::deleteProjectNode(drogon::HttpRequestPtr request,
                                             std::int64_t projectId,
                                             std::int64_t nodeId) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");
        const auto systemRole = session->getOptional<user_domain::SystemRole>("system_role");

        if (!userId || *userId <= 0 || !systemRole) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        if (projectId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "project_id 必须是大于 0 的整数");
        }

        if (nodeId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "node_id 必须是大于 0 的整数");
        }

        try {
            const auto deletedNodeId = co_await projectNodeService_.deleteProjectNode(
                dto::command::DeleteProjectNodeInput{
                    .projectId = projectId,
                    .nodeId = nodeId,
                    .operatorUserId = *userId,
                    .operatorUserRole = *systemRole
                });

            Json::Value data(Json::objectValue);
            data["id"] = deletedNodeId;
            co_return api::ok(data);
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectNodeController::reorderProjectNodes(drogon::HttpRequestPtr request,
                                               std::int64_t projectId) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");
        const auto systemRole = session->getOptional<user_domain::SystemRole>("system_role");

        if (!userId || *userId <= 0 || !systemRole) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        if (projectId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "project_id 必须是大于 0 的整数");
        }

        const auto &json = request->getJsonObject();
        if (!json || !json->isObject()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "请求体必须是 JSON 对象");
        }

        if (!json->isMember("nodes") || !(*json)["nodes"].isArray()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "nodes 必须是数组");
        }

        const Json::Value &nodesJson = (*json)["nodes"];
        if (nodesJson.empty()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "nodes 不能为空数组");
        }

        dto::command::ReorderProjectNodesInput input{
            .projectId = projectId,
            .operatorUserId = *userId,
            .operatorUserRole = *systemRole,
            .nodes = {}
        };
        input.nodes.reserve(nodesJson.size());

        std::unordered_set<std::int64_t> seenNodeIds;
        seenNodeIds.reserve(nodesJson.size());
        std::vector<bool> seenSequenceNos(nodesJson.size() + 1, false);

        for (Json::ArrayIndex index = 0; index < nodesJson.size(); ++index) {
            const Json::Value &nodeJson = nodesJson[index];
            if (!nodeJson.isObject()) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "nodes 中的每一项都必须是 JSON 对象");
            }

            std::int64_t nodeId = 0;
            if (!util::readRequiredInt64(nodeJson, "node_id", nodeId) || nodeId <= 0) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "nodes[].node_id 必须是大于 0 的整数");
            }

            int sequenceNo = 0;
            if (!util::readRequiredInt(nodeJson, "sequence_no", sequenceNo) || sequenceNo <= 0) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "nodes[].sequence_no 必须是大于 0 的整数");
            }

            if (!seenNodeIds.insert(nodeId).second) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "nodes[].node_id 不能重复");
            }

            if (static_cast<Json::ArrayIndex>(sequenceNo) > nodesJson.size() ||
                seenSequenceNos[sequenceNo]) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "nodes[].sequence_no 必须从 1 连续递增");
            }

            seenSequenceNos[sequenceNo] = true;
            input.nodes.push_back(dto::command::ReorderProjectNodeItem{
                .nodeId = nodeId,
                .sequenceNo = sequenceNo
            });
        }

        try {
            const auto result = co_await projectNodeService_.reorderProjectNodes(input);
            co_return api::ok(buildReorderedProjectNodesJson(result));
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }
} // namespace project_tracker::modules::project_node::controller
