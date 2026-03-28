#include "modules/user/controller/UserController.h"

#include "common/api/ApiResponse.h"
#include "common/error/BusinessException.h"
#include "common/util/QueryParamUtil.h"

namespace project_tracker::modules::user::controller {
    namespace api = project_tracker::common::api;
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
            if (const auto systemRole = util::readQueryInt(
                    request,
                    "system_role",
                    "system_role 必须是整数")) {
                query.systemRole = *systemRole;
            }

            if (const auto status = util::readQueryInt(
                    request,
                    "status",
                    "status 必须是整数")) {
                query.status = *status;
            }

            if (const auto page = util::readPositiveQueryInt64(
                    request,
                    "page",
                    "page 必须是大于 0 的整数")) {
                query.page = *page;
            }

            if (const auto pageSize = util::readPositiveQueryInt64(
                    request,
                    "page_size",
                    "page_size 必须是大于 0 的整数")) {
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
        } catch (const common::error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }
} // namespace project_tracker::modules::user::controller
