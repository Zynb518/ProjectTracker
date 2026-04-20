#include "modules/health/service/HealthService.h"

namespace project_tracker::modules::health::service {
    drogon::Task<bool>
    HealthService::isDatabaseReady(const drogon::orm::DbClientPtr &dbClient) const {
        if (!dbClient) {
            co_return false;
        }

        [[maybe_unused]] const auto result = co_await dbClient->execSqlCoro("select 1");
        co_return true;
    }
} // namespace project_tracker::modules::health::service
