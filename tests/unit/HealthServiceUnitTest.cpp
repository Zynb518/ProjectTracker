#include <gtest/gtest.h>
#include <drogon/utils/coroutine.h>
#include "modules/health/service/HealthService.h"

TEST(HealthServiceUnitTest, ReturnsFalseOnNullDbClient) {
    project_tracker::modules::health::service::HealthService service;
    const auto ready = drogon::sync_wait(service.isDatabaseReady(nullptr));
    EXPECT_FALSE(ready);
}

