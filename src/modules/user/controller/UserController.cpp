#include "modules/user/controller/UserController.h"

#include "common/api/ApiResponse.h"
#include "common/error/BusinessException.h"
#include "common/error/ErrorCode.h"
#include "common/util/JsonUtil.h"
#include "common/util/QueryParamUtil.h"

namespace project_tracker::modules::user::controller {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;
    namespace util = project_tracker::common::util;

    namespace {
        // 构造单个用户列表项 JSON。
        Json::Value buildUserJson(const dto::view::SysUserView &user) {
            Json::Value json(Json::objectValue);
            json["id"] = user.id;
            json["username"] = user.username;
            json["real_name"] = user.realName;
            json["system_role"] = domain::toInt(user.systemRole);
            json["email"] = user.email;
            json["phone"] = user.phone;
            json["status"] = domain::toInt(user.status);
            json["created_at"] = user.createdAt;
            json["updated_at"] = user.updatedAt;

            return json;
        }
    } // namespace

    drogon::Task<drogon::HttpResponsePtr>
    UserController::listUsers(drogon::HttpRequestPtr request) {
        try {
            repository::UserListQuery query;

            if (const auto keyword = util::readQueryString(request, "keyword")) {
                query.keyword = *keyword;
            }

            // query 参数只做基础格式校验。
            std::optional<int> systemRole;
            if (!util::readQueryInt(request, "system_role", systemRole)) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    common::error::ErrorCode::InvalidParameter,
                    "system_role 必须是整数");
            }
            if (systemRole) {
                query.systemRole = *systemRole;
            }

            std::optional<int> status;
            if (!util::readQueryInt(request, "status", status)) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    common::error::ErrorCode::InvalidParameter,
                    "status 必须是整数");
            }
            if (status) {
                query.status = *status;
            }

            std::optional<std::int64_t> page;
            if (!util::readPositiveQueryInt64(request, "page", page)) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    common::error::ErrorCode::InvalidParameter,
                    "page 必须是大于 0 的整数");
            }
            if (page) {
                query.page = *page;
            }

            std::optional<std::int64_t> pageSize;
            if (!util::readPositiveQueryInt64(request, "page_size", pageSize)) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    common::error::ErrorCode::InvalidParameter,
                    "page_size 必须是大于 0 的整数");
            }
            if (pageSize) {
                query.pageSize = *pageSize;
            }

            const auto pageResult = co_await userRepository_.listUsers(query);

            Json::Value data(Json::objectValue);
            data["list"] = Json::Value(Json::arrayValue);
            for (const auto &user : pageResult.list) {
                data["list"].append(buildUserJson(user));
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
    UserController::getUserDetail(drogon::HttpRequestPtr request,
                                  std::int64_t userId) {
        (void)request;

        if (userId <= 0) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "user_id 必须是大于 0 的整数");
        }

        try {
            const auto user = co_await userRepository_.findUserById(userId);
            if (!user) {
                co_return api::fail(
                    drogon::k404NotFound,
                    error::ErrorCode::UserNotFound,
                    "用户不存在");
            }

            co_return api::ok(buildUserJson(*user));
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    UserController::updateUserBasicInfo(drogon::HttpRequestPtr request,
                                        std::int64_t userId) {
        const auto &json = request->getJsonObject();
        if (!json || !json->isObject()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "请求体必须是 JSON 对象");
        }

        dto::command::UpdateUserBasicInfoInput input{
            .userId = userId
        };

        if (!util::readOptionalString(*json, "real_name", input.realName)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "real_name 必须是字符串");
        }

        if (!util::readOptionalInt(*json, "system_role", input.systemRole)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "system_role 必须是整数");
        }

        if (!util::readOptionalString(*json, "email", input.email)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "email 必须是字符串");
        }

        if (!util::readOptionalString(*json, "phone", input.phone)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "phone 必须是字符串");
        }

        try {
            const auto user = co_await userService_.updateUserBasicInfo(input);
            co_return api::ok(buildUserJson(user));
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    UserController::createUser(drogon::HttpRequestPtr request) {
        const auto &json = request->getJsonObject();
        if (!json || !json->isObject()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "请求体必须是 JSON 对象");
        }

        service::CreateUserCommand command;
        if (!util::readRequiredString(*json, "username", command.username)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "username 必须是非空字符串");
        }

        if (!util::readRequiredString(*json, "password", command.password)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "password 必须是非空字符串");
        }

        if (!util::readRequiredString(*json, "real_name", command.realName)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "real_name 必须是非空字符串");
        }

        if (!util::readOptionalInt(*json, "system_role", command.systemRole)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "system_role 必须是整数");
        }

        if (!util::readOptionalString(*json, "email", command.email)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "email 必须是字符串");
        }

        if (!util::readOptionalString(*json, "phone", command.phone)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "phone 必须是字符串");
        }

        try {
            const auto user = co_await userService_.createUser(command);
            co_return api::ok(buildUserJson(user));
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }
} // namespace project_tracker::modules::user::controller
