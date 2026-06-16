#pragma once

#include <drogon/utils/coroutine.h>
#include "common/db/SqlExecutor.h"
#include "modules/device_log/dto/command/CreateDeviceInput.h"
#include "modules/device_log/dto/command/CreateDeviceUsageLogInput.h"
#include "modules/device_log/dto/view/CreatedObjectIdView.h"
#include "modules/device_log/dto/view/DeviceListItemView.h"
#include "modules/device_log/dto/view/DeviceUsageLogItemView.h"
#include <vector>

namespace project_tracker::modules::device_log::repository {
    struct DeviceListQuery {
        std::int64_t page = 1;
        std::int64_t pageSize = 10;
    };

    struct DeviceListPage {
        std::vector<dto::view::DeviceListItemView> list;
        std::int64_t total;
        std::int64_t page;
        std::int64_t pageSize;
    };

    struct DeviceUsageLogListQuery {
        std::int64_t deviceId;
        std::int64_t page = 1;
        std::int64_t pageSize = 10;
    };

    struct DeviceUsageLogListPage {
        std::vector<dto::view::DeviceUsageLogItemView> list;
        std::int64_t total;
        std::int64_t page;
        std::int64_t pageSize;
    };

    class DeviceRepository {
    public:
        drogon::Task<dto::view::CreatedObjectIdView>
        insertDevice(const common::db::SqlExecutorPtr &executor,
                     const dto::command::CreateDeviceInput &input) const;

        drogon::Task<bool>
        deleteDevice(const common::db::SqlExecutorPtr &executor,
                     std::int64_t deviceId) const;

        drogon::Task<DeviceListPage>
        listDevices(const common::db::SqlExecutorPtr &executor,
                    const DeviceListQuery &query) const;

        drogon::Task<bool>
        checkDeviceExistsForUpdate(const common::db::SqlExecutorPtr &executor,
                                   std::int64_t deviceId) const;

        drogon::Task<bool>
        checkDeviceExists(const common::db::SqlExecutorPtr &executor,
                          std::int64_t deviceId) const;

        drogon::Task<dto::view::CreatedObjectIdView>
        insertDeviceUsageLog(const common::db::SqlExecutorPtr &executor,
                             const dto::command::CreateDeviceUsageLogInput &input) const;

        drogon::Task<DeviceUsageLogListPage>
        listDeviceUsageLogs(const common::db::SqlExecutorPtr &executor,
                            const DeviceUsageLogListQuery &query) const;
    };
} // namespace project_tracker::modules::device_log::repository
