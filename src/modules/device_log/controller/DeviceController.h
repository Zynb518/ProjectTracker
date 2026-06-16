#pragma once

#include <drogon/HttpController.h>
#include <drogon/utils/coroutine.h>

#include "filters/LoginRequiredFilter.h"
#include "modules/device_log/service/DeviceService.h"

namespace project_tracker::modules::device_log::controller {
    class DeviceController : public drogon::HttpController<DeviceController> {
    public:
        METHOD_LIST_BEGIN
            ADD_METHOD_TO(DeviceController::createDevice,
                          "/api/devices",
                          drogon::Post,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(DeviceController::deleteDevice,
                          "/api/devices/{device_id}",
                          drogon::Delete,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(DeviceController::listDevices,
                          "/api/devices",
                          drogon::Get,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(DeviceController::createDeviceUsageLog,
                          "/api/devices/{device_id}/logs",
                          drogon::Post,
                          filters::LoginRequiredFilter::classTypeName());
            ADD_METHOD_TO(DeviceController::listDeviceUsageLogs,
                          "/api/devices/{device_id}/logs",
                          drogon::Get,
                          filters::LoginRequiredFilter::classTypeName());
        METHOD_LIST_END

        drogon::Task<drogon::HttpResponsePtr>
        createDevice(drogon::HttpRequestPtr request);

        drogon::Task<drogon::HttpResponsePtr>
        deleteDevice(drogon::HttpRequestPtr request, std::int64_t deviceId);

        drogon::Task<drogon::HttpResponsePtr>
        listDevices(drogon::HttpRequestPtr request);

        drogon::Task<drogon::HttpResponsePtr>
        createDeviceUsageLog(drogon::HttpRequestPtr request, std::int64_t deviceId);

        drogon::Task<drogon::HttpResponsePtr>
        listDeviceUsageLogs(drogon::HttpRequestPtr request, std::int64_t deviceId);

    private:
        service::DeviceService deviceService_;
    };
} // namespace project_tracker::modules::device_log::controller
