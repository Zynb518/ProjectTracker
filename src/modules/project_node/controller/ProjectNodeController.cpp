#include "modules/project_node/controller/ProjectNodeController.h"

#include <regex>

#include <drogon/drogon.h>

#include "common/api/ApiResponse.h"
#include "common/error/BusinessException.h"
#include "common/error/ErrorCode.h"
#include "common/util/JsonUtil.h"

namespace project_tracker::modules::project_node::controller {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;
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

        bool isValidDateString(const std::string &value) {
            static const std::regex dateRegex(R"(^\d{4}-\d{2}-\d{2}$)");
            return std::regex_match(value, dateRegex);
        }
    } // namespace

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
} // namespace project_tracker::modules::project_node::controller
