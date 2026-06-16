#pragma once

#include <cstdint>
#include <string>

namespace project_tracker::modules::device_log::dto::view {
    // 设备列表项展示对象
    struct DeviceListItemView {
        std::int64_t id;
        std::string name;
        std::string description;
        std::string createdAt;
        std::string updatedAt;
    };
} // namespace project_tracker::modules::device_log::dto::view
