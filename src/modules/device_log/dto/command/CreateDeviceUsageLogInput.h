#pragma once

#include <cstdint>
#include <string>

namespace project_tracker::modules::device_log::dto::command {
    // 创建设备使用记录输入
    struct CreateDeviceUsageLogInput {
        std::int64_t deviceId;
        std::int64_t operatorUserId;
        std::string purpose;
    };
} // namespace project_tracker::modules::device_log::dto::command
