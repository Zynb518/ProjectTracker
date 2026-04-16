#pragma once

#include <drogon/HttpAppFramework.h>

namespace project_tracker::http {
    void registerRequestLogging(drogon::HttpAppFramework &app);
} // project_tracker::http