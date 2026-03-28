#include "filters/LoginRequiredFilter.h"

#include <cstdint>

#include "common/api/ApiResponse.h"
#include "common/error/ErrorCode.h"

namespace project_tracker::filters {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;

    void LoginRequiredFilter::doFilter(
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

        successCallback();
    }
} // namespace project_tracker::filters
