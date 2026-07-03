#include "filters/RateLimitHelper.h"
#include <algorithm>

namespace project_tracker::filters {

    std::string determineClientIp(const std::string &realIpHeader,
                                  const std::string &forwardedForHeader,
                                  const std::string &peerIp) {
        // 优先从 X-Forwarded-For 提取，以便支持多级代理链路并允许通过该头进行测试隔离
        if (!forwardedForHeader.empty()) {
            // 如果有多个代理，取第一个 IP
            auto commaPos = forwardedForHeader.find(',');
            if (commaPos != std::string::npos) {
                return forwardedForHeader.substr(0, commaPos);
            }
            return forwardedForHeader;
        }
        if (!realIpHeader.empty()) {
            return realIpHeader;
        }
        return peerIp;
    }

    bool RateLimiter::isAllowed(const std::string &ip) {
        auto now = clock_();
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto &state = ipCache_[ip];
        if (state.lastRefillTime == std::chrono::steady_clock::time_point{}) {
            state.lastRefillTime = now;
        }

        // 补充令牌（每 12 秒补充 1 个，即 1/12 = 0.0833 个/秒）
        double elapsed = std::chrono::duration<double>(now - state.lastRefillTime).count();
        state.tokens = std::min(5.0, state.tokens + elapsed * (1.0 / 12.0));
        state.lastRefillTime = now;

        if (state.tokens >= 1.0) {
            state.tokens -= 1.0; // 消耗一个令牌，放行
            return true;
        }
        
        return false;
    }

    size_t RateLimiter::getIpCacheSize() {
        std::lock_guard<std::mutex> lock(mutex_);
        return ipCache_.size();
    }

} // namespace project_tracker::filters
