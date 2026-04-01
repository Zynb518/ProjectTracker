#include "modules/project_member/controller/ProjectMemberController.h"

#include <drogon/drogon.h>

#include "common/api/ApiResponse.h"
#include "common/error/BusinessException.h"
#include "common/error/ErrorCode.h"

namespace project_tracker::modules::project_member::controller {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;
    namespace user_domain = modules::user::domain;

    namespace {
        Json::Value buildProjectMemberJson(const dto::view::ProjectMemberListItemView &member) {
            Json::Value json(Json::objectValue);
            json["project_id"] = member.projectId;
            json["user_id"] = member.userId;
            json["username"] = member.username;
            json["real_name"] = member.realName;
            json["system_role"] = user_domain::toInt(member.systemRole);
            json["status"] = user_domain::toInt(member.status);
            json["joined_at"] = member.joinedAt;
            json["is_owner"] = member.isOwner;

            return json;
        }
    } // namespace

    drogon::Task<drogon::HttpResponsePtr>
    ProjectMemberController::listProjectMembers(drogon::HttpRequestPtr request,
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
            const auto result = co_await projectMemberRepository_.listProjectMembers(
                dbClient,
                repository::ProjectMemberListQuery{
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
            for (const auto &member : result->list) {
                data["list"].append(buildProjectMemberJson(member));
            }

            co_return api::ok(data);
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }
} // namespace project_tracker::modules::project_member::controller
