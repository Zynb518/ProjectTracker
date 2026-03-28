#include "modules/auth/controller/AuthController.h"

#include "common/api/ApiResponse.h"
#include "common/error/BusinessException.h"
#include "common/error/ErrorCode.h"
#include "common/util/JsonUtil.h"


namespace project_tracker::modules::auth::controller {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;
    namespace user_domain = modules::user::domain;
    namespace util = project_tracker::common::util;
    namespace user_view = modules::user::dto::view;

    namespace {
        Json::Value buildUserJson(const user_view::SysUserView &user) {
            Json::Value json(Json::objectValue);
            json["id"] = user.id;
            json["username"] = user.username;
            json["real_name"] = user.realName;
            json["system_role"] = user_domain::toInt(user.systemRole);
            json["email"] = user.email;
            json["phone"] = user.phone;
            json["status"] = user_domain::toInt(user.status);

            return json;
        }
    } // namespace

    drogon::Task<drogon::HttpResponsePtr>
    AuthController::login(drogon::HttpRequestPtr request) {
        const auto &json = request->getJsonObject();
        if (!json || !json->isObject()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "请求体必须是 JSON 对象");
        }

        std::string username;
        if (!util::readRequiredString(*json, "username", username)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "username 必须是非空字符串");
        }

        std::string password;
        if (!util::readRequiredString(*json, "password", password)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "password 必须是非空字符串");
        }

        try {
            const auto user = co_await authService_.login(username, password);
            auto session = request->getSession();
            session->clear();
            session->insert("user_id", user.id);
            session->insert("system_role", user.systemRole);
            session->changeSessionIdToClient();

            Json::Value data(Json::objectValue);
            data["user"] = buildUserJson(user);
            co_return api::ok(data);
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }

    drogon::Task<drogon::HttpResponsePtr>
    AuthController::logout(drogon::HttpRequestPtr request) {
        auto session = request->getSession();
        session->clear();
        session->changeSessionIdToClient();

        co_return api::ok();
    }

    drogon::Task<drogon::HttpResponsePtr>
    AuthController::me(drogon::HttpRequestPtr request) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");

        const auto user = co_await authRepository_.findUserById(*userId);
        if (!user) {
            session->clear();
            session->changeSessionIdToClient();

            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        co_return api::ok(buildUserJson(*user));
    }
} // namespace project_tracker::modules::auth::controller
