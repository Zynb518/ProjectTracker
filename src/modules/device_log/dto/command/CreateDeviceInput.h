#pragma once

#include <string>

namespace project_tracker::modules::device_log::dto::command {
    // 创建设备输入
    struct CreateDeviceInput {
        std::string name;
        std::string description;
    };
} // namespace project_tracker::modules::device_log::dto::command
