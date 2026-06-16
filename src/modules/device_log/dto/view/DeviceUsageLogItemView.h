#pragma once

#include <cstdint>
#include <string>

namespace project_tracker::modules::device_log::dto::view {
    // 设备使用记录项展示对象
    struct DeviceUsageLogItemView {
        std::int64_t id;
        std::int64_t deviceId;
        std::int64_t userId;
        std::string operatorName;
        std::string purpose;
        std::string createdAt;
    };
} // namespace project_tracker::modules::device_log::dto::view
