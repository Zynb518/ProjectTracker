#include "filters/LoginRateLimitFilter.h"
#include <algorithm>
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
        std::string ip = request->getHeader("x-real-ip");
        if (ip.empty()) {
            ip = request->getHeader("x-forwarded-for");
            if (!ip.empty()) {
                // 如果有多个代理，取第一个 IP
                auto commaPos = ip.find(',');
                if (commaPos != std::string::npos) {
                    ip = ip.substr(0, commaPos);
                }
            }
        }
        if (ip.empty()) {
            ip = request->peerAddr().toIp(); // 回退到直接对端连接 IP
        }

        // 2. 令牌桶限流判定
        auto now = std::chrono::steady_clock::now();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto &state = ipCache_[ip];

            // 补充令牌（每 12 秒补充 1 个，即 1/12 = 0.0833 个/秒）
            double elapsed = std::chrono::duration<double>(now - state.lastRefillTime).count();
            state.tokens = std::min(5.0, state.tokens + elapsed * (1.0 / 12.0));
            state.lastRefillTime = now;

            if (state.tokens >= 1.0) {
                state.tokens -= 1.0; // 消耗一个令牌，放行
            } else {
                // 令牌不足，拦截请求并返回 429
                failureCallback(api::fail(
                    drogon::k429TooManyRequests,
                    error::ErrorCode::InvalidParameter,
                    "登录过于频繁，请稍后再试"));
                return;
            }
        }

        successCallback();
    }
} // namespace project_tracker::filters
