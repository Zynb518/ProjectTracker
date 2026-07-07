#include <gtest/gtest.h>
#include "filters/RateLimitHelper.h"

namespace project_tracker::filters {

    TEST(RateLimitHelperTest, DetermineClientIp_RealIpHeaderPresent_ReturnsRealIp) {
        EXPECT_EQ(determineClientIp("192.168.1.10", "10.0.0.1, 10.0.0.2", "127.0.0.1"), "10.0.0.1");
        EXPECT_EQ(determineClientIp("192.168.1.10", "", "127.0.0.1"), "192.168.1.10");
    }

    TEST(RateLimitHelperTest, DetermineClientIp_ForwardedForPresentNoRealIp_ReturnsFirstIp) {
        EXPECT_EQ(determineClientIp("", "10.0.0.5, 10.0.0.6", "127.0.0.1"), "10.0.0.5");
        EXPECT_EQ(determineClientIp("", "10.0.0.5", "127.0.0.1"), "10.0.0.5");
    }

    TEST(RateLimitHelperTest, DetermineClientIp_OnlyPeerIp_ReturnsPeerIp) {
        EXPECT_EQ(determineClientIp("", "", "127.0.0.1"), "127.0.0.1");
    }

    TEST(RateLimitHelperTest, RateLimiter_NormalTrafficAllowed) {
        RateLimiter limiter;
        for (int i = 0; i < 5; ++i) {
            EXPECT_TRUE(limiter.isAllowed("192.168.1.1"));
        }
    }

    TEST(RateLimitHelperTest, RateLimiter_OverLimitBlocked) {
        RateLimiter limiter;
        for (int i = 0; i < 5; ++i) {
            EXPECT_TRUE(limiter.isAllowed("192.168.1.1"));
        }
        EXPECT_FALSE(limiter.isAllowed("192.168.1.1"));
    }

    TEST(RateLimitHelperTest, RateLimiter_IpIsolation) {
        RateLimiter limiter;
        for (int i = 0; i < 5; ++i) {
            EXPECT_TRUE(limiter.isAllowed("192.168.1.1"));
        }
        EXPECT_FALSE(limiter.isAllowed("192.168.1.1"));

        EXPECT_TRUE(limiter.isAllowed("192.168.1.2"));
    }

    TEST(RateLimitHelperTest, RateLimiter_TokenRefill) {
        std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
        auto mockClock = [&currentTime]() {
            return currentTime;
        };

        RateLimiter limiter(mockClock);

        for (int i = 0; i < 5; ++i) {
            EXPECT_TRUE(limiter.isAllowed("192.168.1.1"));
        }
        EXPECT_FALSE(limiter.isAllowed("192.168.1.1"));

        currentTime += std::chrono::seconds(12);

        EXPECT_TRUE(limiter.isAllowed("192.168.1.1"));
        EXPECT_FALSE(limiter.isAllowed("192.168.1.1"));

        currentTime += std::chrono::seconds(60);

        for (int i = 0; i < 5; ++i) {
            EXPECT_TRUE(limiter.isAllowed("192.168.1.1"));
        }
        EXPECT_FALSE(limiter.isAllowed("192.168.1.1"));
    }

} // namespace project_tracker::filters
