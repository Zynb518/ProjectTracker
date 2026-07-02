#include "filters/LoginRateLimitFilter.h"
#include "common/api/ApiResponse.h"
#include "common/error/ErrorCode.h"

namespace project_tracker::filters {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;

    void LoginRateLimitFilter::doFilter(
        const drogon::HttpRequestPtr &request,
        drogon::FilterCallback &&failureCallback,
        drogon::FilterChainCallback &&successCallback) {
        
        // 1. 获取客户端真实 IP（考虑 Nginx 反向代理）
        std::string realIp = request->getHeader("x-real-ip");
        std::string forwardedFor = request->getHeader("x-forwarded-for");
        std::string peerIp = request->peerAddr().toIp();

        std::string ip = determineClientIp(realIp, forwardedFor, peerIp);

        // 2. 限流判定
        if (!rateLimiter_.isAllowed(ip)) {
            // 令牌不足，拦截请求并返回 429
            failureCallback(api::fail(
                drogon::k429TooManyRequests,
                error::ErrorCode::InvalidParameter,
                "登录过于频繁，请稍后再试"));
            return;
        }

        successCallback();
    }
} // namespace project_tracker::filters
