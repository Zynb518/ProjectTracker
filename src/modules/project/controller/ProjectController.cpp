#include "modules/project/controller/ProjectController.h"

#include <regex>

#include <drogon/drogon.h>

#include "common/api/ApiResponse.h"
#include "common/error/BusinessException.h"
#include "common/error/ErrorCode.h"
#include "common/util/JsonUtil.h"
#include "common/util/QueryParamUtil.h"

namespace project_tracker::modules::project::controller {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;
    namespace user_domain = modules::user::domain;
    namespace util = project_tracker::common::util;

    namespace {
        Json::Value buildProjectJson(const dto::view::ProjectListItemView &project) {
            Json::Value json(Json::objectValue);
            json["id"] = project.id;
            json["name"] = project.name;
            json["description"] = project.description;
            json["owner_user_id"] = project.ownerUserId;
            json["owner_real_name"] = project.ownerRealName;
            json["status"] = domain::toInt(project.status);
            json["planned_start_date"] = project.plannedStartDate;
            json["planned_end_date"] = project.plannedEndDate;
            if (project.completedAt) {
                json["completed_at"] = *project.completedAt;
            } else {
                json["completed_at"] = Json::Value(Json::nullValue);
            }
            json["member_count"] = project.memberCount;
            json["node_count"] = project.nodeCount;
            json["sub_task_count"] = project.subTaskCount;
            json["created_at"] = project.createdAt;
            json["updated_at"] = project.updatedAt;
            json["is_owner"] = project.isOwner;

            return json;
        }

        Json::Value buildProjectDetailJson(const dto::view::ProjectDetailView &project) {
            Json::Value json(Json::objectValue);
            json["id"] = project.id;
            json["name"] = project.name;
            json["description"] = project.description;
            json["owner_user_id"] = project.ownerUserId;
            json["owner_real_name"] = project.ownerRealName;
            json["status"] = domain::toInt(project.status);
            json["planned_start_date"] = project.plannedStartDate;
            json["planned_end_date"] = project.plannedEndDate;
            if (project.completedAt) {
                json["completed_at"] = *project.completedAt;
            } else {
                json["completed_at"] = Json::Value(Json::nullValue);
            }
            json["created_by"] = project.createdBy;
            json["created_by_real_name"] = project.createdByRealName;
            json["created_at"] = project.createdAt;
            json["updated_at"] = project.updatedAt;
            json["member_count"] = project.memberCount;
            json["node_count"] = project.nodeCount;
            json["completed_node_count"] = project.completedNodeCount;
            json["sub_task_count"] = project.subTaskCount;
            json["completed_sub_task_count"] = project.completedSubTaskCount;

            Json::Value permissions(Json::objectValue);
            permissions["can_edit_basic"] = project.permissions.canEditBasic;
            permissions["can_manage_members"] = project.permissions.canManageMembers;
            permissions["can_manage_nodes"] = project.permissions.canManageNodes;
            permissions["can_transfer_owner"] = project.permissions.canTransferOwner;
            permissions["can_delete"] = project.permissions.canDelete;
            json["permissions"] = permissions;

            return json;
        }

        Json::Value buildCreatedProjectJson(const dto::view::CreatedProjectView &project) {
            Json::Value json(Json::objectValue);
            json["id"] = project.id;
            json["name"] = project.name;
            json["description"] = project.description;
            json["owner_user_id"] = project.ownerUserId;
            json["status"] = domain::toInt(project.status);
            json["planned_start_date"] = project.plannedStartDate;
            json["planned_end_date"] = project.plannedEndDate;
            if (project.completedAt) {
                json["completed_at"] = *project.completedAt;
            } else {
                json["completed_at"] = Json::Value(Json::nullValue);
            }
            json["created_by"] = project.createdBy;
            json["created_at"] = project.createdAt;
            json["updated_at"] = project.updatedAt;

            return json;
        }

        Json::Value buildUpdatedProjectBasicInfoJson(
            const dto::view::UpdatedProjectBasicInfoView &project) {
            Json::Value json(Json::objectValue);
            json["id"] = project.id;
            json["name"] = project.name;
            json["description"] = project.description;
            json["planned_start_date"] = project.plannedStartDate;
            json["planned_end_date"] = project.plannedEndDate;
            json["updated_at"] = project.updatedAt;

            return json;
        }

        bool isValidProjectStatus(int status) {
            return status == domain::toInt(domain::ProjectStatus::NotStarted) ||
                   status == domain::toInt(domain::ProjectStatus::InProgress) ||
                   status == domain::toInt(domain::ProjectStatus::Completed) ||
                   status == domain::toInt(domain::ProjectStatus::Delayed);
        }

        bool isValidDateString(const std::string &value) {
            static const std::regex dateRegex(R"(^\d{4}-\d{2}-\d{2}$)");
            return std::regex_match(value, dateRegex);
        }
    } // namespace

    drogon::Task<drogon::HttpResponsePtr>
    ProjectController::createProject(drogon::HttpRequestPtr request) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");

        if (!userId || *userId <= 0) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        const auto &json = request->getJsonObject();
        if (!json || !json->isObject()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "请求体必须是 JSON 对象");
        }

        dto::command::CreateProjectInput input{
            .creatorUserId = *userId
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
            const auto project = co_await projectService_.createProject(input);
            co_return api::ok(buildCreatedProjectJson(project));
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectController::listProjects(drogon::HttpRequestPtr request) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");
        const auto systemRole = session->getOptional<user_domain::SystemRole>("system_role");

        if (!userId || *userId <= 0 || !systemRole) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        try {
            repository::ProjectListQuery query{
                .currentUserId = *userId,
                .currentUserRole = *systemRole
            };

            if (const auto keyword = util::readQueryString(request, "keyword")) {
                query.keyword = *keyword;
            }

            std::optional<int> statusValue;
            if (!util::readQueryInt(request, "status", statusValue)) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "status 必须是整数");
            }
            if (statusValue && !isValidProjectStatus(*statusValue)) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "status 只能是 1、2、3 或 4");
            }
            if (statusValue) {
                query.status = static_cast<domain::ProjectStatus>(*statusValue);
            }

            if (!util::readPositiveQueryInt64(request, "owner_user_id", query.ownerUserId)) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "owner_user_id 必须是大于 0 的整数");
            }
            if (query.ownerUserId && *systemRole != user_domain::SystemRole::Admin) {
                co_return api::fail(
                    drogon::k403Forbidden,
                    error::ErrorCode::Forbidden,
                    "owner_user_id 仅管理员可用");
            }

            std::optional<std::int64_t> page;
            if (!util::readPositiveQueryInt64(request, "page", page)) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "page 必须是大于 0 的整数");
            }
            if (page) {
                query.page = *page;
            }

            std::optional<std::int64_t> pageSize;
            if (!util::readPositiveQueryInt64(request, "page_size", pageSize)) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "page_size 必须是大于 0 的整数");
            }
            if (pageSize) {
                query.pageSize = *pageSize;
            }

            const auto dbClient = drogon::app().getDbClient();
            const auto pageResult = co_await projectRepository_.listProjects(dbClient, query);

            Json::Value data(Json::objectValue);
            data["list"] = Json::Value(Json::arrayValue);
            for (const auto &project : pageResult.list) {
                data["list"].append(buildProjectJson(project));
            }
            data["total"] = pageResult.total;
            data["page"] = pageResult.page;
            data["page_size"] = pageResult.pageSize;

            co_return api::ok(data);
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectController::getProjectDetail(drogon::HttpRequestPtr request,
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
            const auto project = co_await projectRepository_.findProjectDetail(
                dbClient,
                projectId,
                *userId,
                *systemRole);

            if (!project) {
                co_return api::fail(
                    drogon::k404NotFound,
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            co_return api::ok(buildProjectDetailJson(*project));
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectController::updateProjectBasicInfo(drogon::HttpRequestPtr request,
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

        dto::command::UpdateProjectBasicInfoInput input{
            .projectId = projectId,
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

        try {
            const auto project = co_await projectService_.updateProjectBasicInfo(input);
            co_return api::ok(buildUpdatedProjectBasicInfoJson(project));
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectController::deleteProject(drogon::HttpRequestPtr request,
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
            const auto deletedProjectId = co_await projectService_.deleteProject(
                dto::command::DeleteProjectInput{
                    .projectId = projectId,
                    .operatorUserId = *userId,
                    .operatorUserRole = *systemRole
                });

            Json::Value data(Json::objectValue);
            data["id"] = deletedProjectId;
            co_return api::ok(data);
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectController::startProject(drogon::HttpRequestPtr request,
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

        try {
            const auto project = co_await projectService_.startProject(
                dto::command::ProjectStatusActionInput{
                    .projectId = projectId,
                    .operatorUserId = *userId,
                    .operatorUserRole = *systemRole
                });

            Json::Value data(Json::objectValue);
            data["id"] = project.id;
            data["status"] = domain::toInt(project.status);
            if (project.completedAt) {
                data["completed_at"] = *project.completedAt;
            } else {
                data["completed_at"] = Json::Value(Json::nullValue);
            }
            data["updated_at"] = project.updatedAt;

            co_return api::ok(data);
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectController::completeProject(drogon::HttpRequestPtr request,
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

        try {
            const auto project = co_await projectService_.completeProject(
                dto::command::ProjectStatusActionInput{
                    .projectId = projectId,
                    .operatorUserId = *userId,
                    .operatorUserRole = *systemRole
                });

            Json::Value data(Json::objectValue);
            data["id"] = project.id;
            data["status"] = domain::toInt(project.status);
            if (project.completedAt) {
                data["completed_at"] = *project.completedAt;
            } else {
                data["completed_at"] = Json::Value(Json::nullValue);
            }
            data["updated_at"] = project.updatedAt;

            co_return api::ok(data);
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectController::reopenProject(drogon::HttpRequestPtr request,
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

        try {
            const auto project = co_await projectService_.reopenProject(
                dto::command::ProjectStatusActionInput{
                    .projectId = projectId,
                    .operatorUserId = *userId,
                    .operatorUserRole = *systemRole
                });

            Json::Value data(Json::objectValue);
            data["id"] = project.id;
            data["status"] = domain::toInt(project.status);
            if (project.completedAt) {
                data["completed_at"] = *project.completedAt;
            } else {
                data["completed_at"] = Json::Value(Json::nullValue);
            }
            data["updated_at"] = project.updatedAt;

            co_return api::ok(data);
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }
} // namespace project_tracker::modules::project::controller
