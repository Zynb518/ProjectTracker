#pragma once

#include <drogon/HttpAppFramework.h>
#include <drogon/HttpTypes.h>
#include <exception>
#include <string>
#include "common/error/ErrorCode.h"

namespace project_tracker::http {
    struct ExceptionProcessResult {
        drogon::HttpStatusCode statusCode;
        common::error::ErrorCode errorCode;
        std::string message;
        bool logAsError; // true 代表需要 LOG_ERROR，false 代表需要 LOG_WARN
    };

    ExceptionProcessResult processException(const std::exception &exception);

    void registerExceptionHandling(drogon::HttpAppFramework &app);
} // namespace project_tracker::http
