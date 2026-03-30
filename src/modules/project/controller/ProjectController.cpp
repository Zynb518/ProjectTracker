#include "modules/project/controller/ProjectController.h"

#include "common/api/ApiResponse.h"
#include "common/error/BusinessException.h"
#include "common/error/ErrorCode.h"
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

        bool isValidProjectStatus(int status) {
            return status == domain::toInt(domain::ProjectStatus::NotStarted) ||
                   status == domain::toInt(domain::ProjectStatus::InProgress) ||
                   status == domain::toInt(domain::ProjectStatus::Completed) ||
                   status == domain::toInt(domain::ProjectStatus::Delayed);
        }
    } // namespace

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

            const auto pageResult = co_await projectRepository_.listProjects(query);

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
} // namespace project_tracker::modules::project::controller
