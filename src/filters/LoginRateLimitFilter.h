#pragma once

#include <drogon/HttpFilter.h>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <string>

namespace project_tracker::filters {

    // 登录接口限流过滤器
    // 采用令牌桶算法实现，限制每个客户端 IP 的登录尝试频率
    class LoginRateLimitFilter : public drogon::HttpFilter<LoginRateLimitFilter> {
    public:
        void doFilter(const drogon::HttpRequestPtr &request,
                      drogon::FilterCallback &&failureCallback,
                      drogon::FilterChainCallback &&successCallback) override;

    private:
        struct LimitState {
            std::chrono::steady_clock::time_point lastRefillTime = std::chrono::steady_clock::now();
            double tokens = 5.0; // 桶的最大容量，初始为满
        };

        std::unordered_map<std::string, LimitState> ipCache_;
        std::mutex mutex_; // 保护 ipCache_ 线程安全
    };
    
} // namespace project_tracker::filters
