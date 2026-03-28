#include "filters/AdminRequiredFilter.h"

#include <cstdint>

#include "common/api/ApiResponse.h"
#include "common/error/ErrorCode.h"
#include "modules/user/domain/UserEnums.h"

namespace project_tracker::filters {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;
    namespace user_domain = project_tracker::modules::user::domain;

    void AdminRequiredFilter::doFilter(
        const drogon::HttpRequestPtr &request,
        drogon::FilterCallback &&failureCallback,
        drogon::FilterChainCallback &&successCallback) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");
        if (!userId || *userId <= 0) {
            failureCallback(api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效"));
            return;
        }

        const auto systemRole =
            session->getOptional<user_domain::SystemRole>("system_role");
        if (!systemRole) {
            failureCallback(api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效"));
            return;
        }

        if (*systemRole != user_domain::SystemRole::Admin) {
            failureCallback(api::fail(
                drogon::k403Forbidden,
                error::ErrorCode::Forbidden,
                "无权限访问"));
            return;
        }

        successCallback();
    }
} // namespace project_tracker::filters
