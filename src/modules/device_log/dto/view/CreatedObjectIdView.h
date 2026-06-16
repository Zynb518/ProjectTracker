#pragma once

#include <cstdint>

namespace project_tracker::modules::device_log::dto::view {
    // 创建对象后的统一主键返回视图
    struct CreatedObjectIdView {
        std::int64_t id;
    };
} // namespace project_tracker::modules::device_log::dto::view
