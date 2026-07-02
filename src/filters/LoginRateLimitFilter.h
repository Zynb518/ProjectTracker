#pragma once

#include <drogon/HttpFilter.h>
#include "filters/RateLimitHelper.h"

namespace project_tracker::filters {

    // 登录接口限流过滤器
    // 采用令牌桶算法实现，限制每个客户端 IP 的登录尝试频率
    class LoginRateLimitFilter : public drogon::HttpFilter<LoginRateLimitFilter> {
    public:
        void doFilter(const drogon::HttpRequestPtr &request,
                      drogon::FilterCallback &&failureCallback,
                      drogon::FilterChainCallback &&successCallback) override;

    private:
        RateLimiter rateLimiter_;
    };
    
} // namespace project_tracker::filters
