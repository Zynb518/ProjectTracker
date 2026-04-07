#include "modules/task/controller/TaskController.h"

#include <drogon/drogon.h>

#include "common/api/ApiResponse.h"
#include "common/error/BusinessException.h"
#include "common/error/ErrorCode.h"
#include "common/util/JsonUtil.h"
#include "common/util/QueryParamUtil.h"

namespace project_tracker::modules::task::controller {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;
    namespace user_domain = modules::user::domain;
    namespace util = project_tracker::common::util;

    namespace {
        Json::Value buildTaskJson(const dto::view::TaskListItemView &task) {
            Json::Value json(Json::objectValue);
            json["id"] = task.id;
            json["node_id"] = task.nodeId;
            json["name"] = task.name;
            json["description"] = task.description;
            json["responsible_user_id"] = task.responsibleUserId;
            json["responsible_real_name"] = task.responsibleRealName;
            json["status"] = domain::toInt(task.status);
            json["progress_percent"] = task.progressPercent;
            json["priority"] = domain::toInt(task.priority);
            json["planned_start_date"] = task.plannedStartDate;
            json["planned_end_date"] = task.plannedEndDate;
            if (task.completedAt) {
                json["completed_at"] = *task.completedAt;
            } else {
                json["completed_at"] = Json::Value(Json::nullValue);
            }
            json["created_by"] = task.createdBy;
            json["created_at"] = task.createdAt;
            json["updated_at"] = task.updatedAt;

            return json;
        }

        Json::Value buildMyTaskJson(const dto::view::MyTaskListItemView &task) {
            Json::Value json(Json::objectValue);
            json["id"] = task.id;
            json["project_id"] = task.projectId;
            json["project_name"] = task.projectName;
            json["node_id"] = task.nodeId;
            json["node_name"] = task.nodeName;
            json["name"] = task.name;
            json["status"] = domain::toInt(task.status);
            json["progress_percent"] = task.progressPercent;
            json["priority"] = domain::toInt(task.priority);
            json["planned_start_date"] = task.plannedStartDate;
            json["planned_end_date"] = task.plannedEndDate;
            if (task.completedAt) {
                json["completed_at"] = *task.completedAt;
            } else {
                json["completed_at"] = Json::Value(Json::nullValue);
            }
            json["updated_at"] = task.updatedAt;

            return json;
        }

        bool isValidTaskStatus(int status) {
            return status == domain::toInt(domain::TaskStatus::NotStarted) ||
                   status == domain::toInt(domain::TaskStatus::InProgress) ||
                   status == domain::toInt(domain::TaskStatus::Completed) ||
                   status == domain::toInt(domain::TaskStatus::Delayed);
        }
    } // namespace

    drogon::Task<drogon::HttpResponsePtr>
    TaskController::listNodeTasks(drogon::HttpRequestPtr request,
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

        // 读取Query 参数
        repository::TaskListQuery query{
            .projectId = projectId,
            .nodeId = nodeId,
            .currentUserId = *userId,
            .currentUserRole = *systemRole
        };

        std::optional<int> statusValue;
        if (!util::readQueryInt(request, "status", statusValue)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "status 必须是整数");
        }
        if (statusValue && !isValidTaskStatus(*statusValue)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "status 只能是 1、2、3 或 4");
        }
        if (statusValue) {
            query.status = static_cast<domain::TaskStatus>(*statusValue);
        }

        if (!util::readPositiveQueryInt64(
            request,
            "responsible_user_id",
            query.responsibleUserId)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "responsible_user_id 必须是大于 0 的整数");
        }
        try {

            const auto dbClient = drogon::app().getDbClient();
            const auto result = co_await taskRepository_.listNodeTasks(dbClient, query);

            if (!result.projectExists) {
                co_return api::fail(
                    drogon::k404NotFound,
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            if (!result.hasPermission) {
                co_return api::fail(
                    drogon::k403Forbidden,
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目成员");
            }

            if (!result.nodeExists) {
                co_return api::fail(
                    drogon::k404NotFound,
                    error::ErrorCode::PhaseNotFound,
                    "阶段节点不存在");
            }

            Json::Value data(Json::objectValue);
            data["list"] = Json::Value(Json::arrayValue);
            for (const auto &task : result.list) {
                data["list"].append(buildTaskJson(task));
            }

            co_return api::ok(data);
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    TaskController::listMyTasks(drogon::HttpRequestPtr request) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");

        if (!userId || *userId <= 0) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        repository::MyTaskListQuery query{
            .currentUserId = *userId
        };

        std::optional<int> statusValue;
        if (!util::readQueryInt(request, "status", statusValue)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "status 必须是整数");
        }
        if (statusValue && !isValidTaskStatus(*statusValue)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "status 只能是 1、2、3 或 4");
        }
        if (statusValue) {
            query.status = static_cast<domain::TaskStatus>(*statusValue);
        }

        if (!util::readPositiveQueryInt64(request, "project_id", query.projectId)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "project_id 必须是大于 0 的整数");
        }

        try {
            const auto dbClient = drogon::app().getDbClient();
            const auto list = co_await taskRepository_.listMyTasks(dbClient, query);

            Json::Value data(Json::objectValue);
            data["list"] = Json::Value(Json::arrayValue);

            for (const auto &task : list) {
                data["list"].append(buildMyTaskJson(task));
            }

            co_return api::ok(data);
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }
} // namespace project_tracker::modules::task::controller
