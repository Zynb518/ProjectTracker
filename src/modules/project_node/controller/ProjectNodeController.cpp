#include "modules/project_node/controller/ProjectNodeController.h"

#include <drogon/drogon.h>

#include "common/api/ApiResponse.h"
#include "common/error/BusinessException.h"
#include "common/error/ErrorCode.h"

namespace project_tracker::modules::project_node::controller {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;
    namespace user_domain = modules::user::domain;

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
} // namespace project_tracker::modules::project_node::controller
