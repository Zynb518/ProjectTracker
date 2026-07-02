#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <functional>

namespace project_tracker::filters {

    // 纯函数：根据各类代理 header 和 peer IP 确定最终客户端真实 IP
    std::string determineClientIp(const std::string &realIpHeader,
                                  const std::string &forwardedForHeader,
                                  const std::string &peerIp);

    // 令牌桶状态
    struct LimitState {
        std::chrono::steady_clock::time_point lastRefillTime;
        double tokens = 5.0; // 最大容量
    };

    // 线程安全的令牌桶限流器类
    class RateLimiter {
    public:
        // 支持自定义 clock 获取当前时间，便于测试时模拟时间流逝
        using ClockFn = std::function<std::chrono::steady_clock::time_point()>;

        explicit RateLimiter(ClockFn clock = []() { return std::chrono::steady_clock::now(); })
            : clock_(std::move(clock)) {}

        // 判断指定 IP 的请求是否允许放行。如果允许，消耗一个令牌并返回 true；否则返回 false。
        bool isAllowed(const std::string &ip);

        // 获取缓存的 IP 数量（供测试断言使用）
        size_t getIpCacheSize();

    private:
        ClockFn clock_;
        std::unordered_map<std::string, LimitState> ipCache_;
        std::mutex mutex_;
    };

} // namespace project_tracker::filters
