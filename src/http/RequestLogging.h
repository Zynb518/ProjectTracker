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

    std::string getOrCreateRequestId(const drogon::HttpRequestPtr &request);
    RequestLogLevel resolveRequestLogLevel(int statusCode, std::int64_t elapsedMs);
    void registerRequestLogging(drogon::HttpAppFramework &app);
} // namespace project_tracker::http
