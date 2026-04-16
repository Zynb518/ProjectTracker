#pragma once

#include <drogon/HttpAppFramework.h>

namespace project_tracker::http {
    void registerExceptionHandling(drogon::HttpAppFramework &app);
} // namespace project_tracker::http
