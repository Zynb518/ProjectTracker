#pragma once

#include <cstdint>
#include <string>

#include <drogon/HttpAppFramework.h>

namespace project_tracker::http {
    enum class RequestLogLevel {
        Info,
        Warn,
        Error
    };

    std::string sanitizePath(const std::string &path);
    std::string formatRequestLog(const std::string &requestId,
                                 const std::string &method,
                                 const std::string &path,
                                 int statusCode,
                                 std::int64_t elapsedMs,
                                 const std::string &ip,
                                 const std::string &userIdText);

    std::string getOrCreateRequestId(const drogon::HttpRequestPtr &request);
    RequestLogLevel resolveRequestLogLevel(int statusCode, std::int64_t elapsedMs);
    void registerRequestLogging(drogon::HttpAppFramework &app);
} // namespace project_tracker::http
