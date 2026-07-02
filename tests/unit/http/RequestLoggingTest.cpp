#include <gtest/gtest.h>
#include "http/RequestLogging.h"

namespace project_tracker::http {

    TEST(RequestLoggingTest, SanitizePath_NoNewlines_ReturnsUnchanged) {
        EXPECT_EQ(sanitizePath("/api/v1/projects"), "/api/v1/projects");
        EXPECT_EQ(sanitizePath(""), "");
    }

    TEST(RequestLoggingTest, SanitizePath_WithNewlines_ReplacesWithUnderscore) {
        EXPECT_EQ(sanitizePath("/api/v1\r\nprojects"), "/api/v1__projects");
        EXPECT_EQ(sanitizePath("\r"), "_");
        EXPECT_EQ(sanitizePath("\n"), "_");
    }

    TEST(RequestLoggingTest, ResolveRequestLogLevel_Info_Level) {
        EXPECT_EQ(resolveRequestLogLevel(200, 500), RequestLogLevel::Info);
        EXPECT_EQ(resolveRequestLogLevel(302, 999), RequestLogLevel::Info);
    }

    TEST(RequestLoggingTest, ResolveRequestLogLevel_Warn_Level) {
        EXPECT_EQ(resolveRequestLogLevel(400, 100), RequestLogLevel::Warn);
        EXPECT_EQ(resolveRequestLogLevel(401, 50), RequestLogLevel::Warn);
        EXPECT_EQ(resolveRequestLogLevel(403, 10), RequestLogLevel::Warn);
        EXPECT_EQ(resolveRequestLogLevel(404, 5), RequestLogLevel::Warn);

        EXPECT_EQ(resolveRequestLogLevel(200, 1000), RequestLogLevel::Warn);
        EXPECT_EQ(resolveRequestLogLevel(200, 1500), RequestLogLevel::Warn);
    }

    TEST(RequestLoggingTest, ResolveRequestLogLevel_Error_Level) {
        EXPECT_EQ(resolveRequestLogLevel(500, 50), RequestLogLevel::Error);
        EXPECT_EQ(resolveRequestLogLevel(503, 1000), RequestLogLevel::Error);
    }

    TEST(RequestLoggingTest, FormatRequestLog_NormalRequest) {
        std::string log = formatRequestLog(
            "req-123",
            "GET",
            "/api/health",
            200,
            120,
            "127.0.0.1",
            "42"
        );
        EXPECT_NE(log.find("request_id=req-123"), std::string::npos);
        EXPECT_NE(log.find("方法=GET"), std::string::npos);
        EXPECT_NE(log.find("路径=/api/health"), std::string::npos);
        EXPECT_NE(log.find("状态码=200"), std::string::npos);
        EXPECT_NE(log.find("耗时_ms=120"), std::string::npos);
        EXPECT_NE(log.find("IP=127.0.0.1"), std::string::npos);
        EXPECT_NE(log.find("user_id=42"), std::string::npos);
        EXPECT_EQ(log.find("slow_request"), std::string::npos);
    }

    TEST(RequestLoggingTest, FormatRequestLog_SlowRequest) {
        std::string log = formatRequestLog(
            "req-456",
            "POST",
            "/api/projects",
            201,
            1005,
            "192.168.1.1",
            "anonymous"
        );
        EXPECT_NE(log.find("request_id=req-456"), std::string::npos);
        EXPECT_NE(log.find("方法=POST"), std::string::npos);
        EXPECT_NE(log.find("路径=/api/projects"), std::string::npos);
        EXPECT_NE(log.find("状态码=201"), std::string::npos);
        EXPECT_NE(log.find("耗时_ms=1005"), std::string::npos);
        EXPECT_NE(log.find("IP=192.168.1.1"), std::string::npos);
        EXPECT_NE(log.find("user_id=anonymous"), std::string::npos);
        EXPECT_NE(log.find("slow_request=true"), std::string::npos);
    }

} // namespace project_tracker::http
