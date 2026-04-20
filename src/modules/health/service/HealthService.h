#pragma once

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

namespace project_tracker::modules::health::service {
    class HealthService {
    public:
        drogon::Task<bool> isDatabaseReady(const drogon::orm::DbClientPtr &dbClient) const;
    };
} // namespace project_tracker::modules::health::service
