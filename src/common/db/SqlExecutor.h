#pragma once

#include <drogon/orm/DbClient.h>

namespace project_tracker::common::db {
    using SqlExecutorPtr = drogon::orm::DbClientPtr;
} // namespace project_tracker::common::db
