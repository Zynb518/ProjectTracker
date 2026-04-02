#include "modules/project_member/controller/ProjectMemberController.h"

#include <drogon/drogon.h>

#include "common/api/ApiResponse.h"
#include "common/error/BusinessException.h"
#include "common/error/ErrorCode.h"
#include "common/util/JsonUtil.h"
#include "common/util/QueryParamUtil.h"

namespace project_tracker::modules::project_member::controller {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;
    namespace user_domain = modules::user::domain;
    namespace util = project_tracker::common::util;

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

        Json::Value buildProjectMemberCandidateJson(
            const dto::view::ProjectMemberCandidateView &candidate) {
            Json::Value json(Json::objectValue);
            json["id"] = candidate.id;
            json["username"] = candidate.username;
            json["real_name"] = candidate.realName;
            json["system_role"] = user_domain::toInt(candidate.systemRole);
            json["status"] = user_domain::toInt(candidate.status);

            return json;
        }

        Json::Value buildAddedProjectMemberJson(const dto::view::AddedProjectMemberView &member) {
            Json::Value json(Json::objectValue);
            json["project_id"] = member.projectId;
            json["user_id"] = member.userId;
            json["joined_at"] = member.joinedAt;

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

    drogon::Task<drogon::HttpResponsePtr>
    ProjectMemberController::addProjectMember(drogon::HttpRequestPtr request,
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

        dto::command::AddProjectMemberInput input{
            .projectId = projectId,
            .operatorUserId = *userId,
            .operatorUserRole = *systemRole
        };

        if (!util::readRequiredInt64(*json, "user_id", input.userId) ||
            input.userId <= 0 ) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "user_id 必须是大于 0 的整数");
        }

        try {
            const auto member = co_await projectMemberService_.addProjectMember(input);
            co_return api::ok(buildAddedProjectMemberJson(member));
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    ProjectMemberController::listProjectMemberCandidates(drogon::HttpRequestPtr request,
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

        repository::ProjectMemberCandidateQuery query{
            .projectId = projectId,
            .operatorUserId = *userId,
            .operatorUserRole = *systemRole
        };

        if (const auto keyword = util::readQueryString(request, "keyword")) {
            query.keyword = *keyword;
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

        try {
            const auto dbClient = drogon::app().getDbClient();
            const auto result = co_await projectMemberRepository_.listProjectMemberCandidates(
                dbClient,
                query);

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
                    "当前操作者不是管理员且不是具备成员管理权限的项目负责人");
            }

            if (!result->memberManageAllowed) {
                co_return api::fail(
                    drogon::k409Conflict,
                    error::ErrorCode::PersonalProjectMemberManageForbidden,
                    "普通员工创建的个人自用项目不允许获取成员候选列表");
            }

            Json::Value data(Json::objectValue);
            data["list"] = Json::Value(Json::arrayValue);
            for (const auto &candidate : result->page.list) {
                data["list"].append(buildProjectMemberCandidateJson(candidate));
            }
            data["total"] = result->page.total;
            data["page"] = result->page.page;
            data["page_size"] = result->page.pageSize;

            co_return api::ok(data);
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }
} // namespace project_tracker::modules::project_member::controller
