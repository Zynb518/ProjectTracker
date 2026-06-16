#pragma once

#include <drogon/utils/coroutine.h>
#include "modules/device_log/dto/command/CreateDeviceInput.h"
#include "modules/device_log/dto/command/CreateDeviceUsageLogInput.h"
#include "modules/device_log/dto/command/DeleteDeviceInput.h"
#include "modules/device_log/dto/view/CreatedObjectIdView.h"
#include "modules/device_log/repository/DeviceRepository.h"

namespace project_tracker::modules::device_log::service {
    class DeviceService {
    public:
        drogon::Task<dto::view::CreatedObjectIdView>
        createDevice(const dto::command::CreateDeviceInput &input) const;

        drogon::Task<void>
        deleteDevice(const dto::command::DeleteDeviceInput &input) const;

        drogon::Task<repository::DeviceListPage>
        listDevices(const repository::DeviceListQuery &query) const;

        drogon::Task<dto::view::CreatedObjectIdView>
        createDeviceUsageLog(const dto::command::CreateDeviceUsageLogInput &input) const;

        drogon::Task<repository::DeviceUsageLogListPage>
        listDeviceUsageLogs(const repository::DeviceUsageLogListQuery &query) const;

    private:
        repository::DeviceRepository deviceRepository_;
    };
} // namespace project_tracker::modules::device_log::service
