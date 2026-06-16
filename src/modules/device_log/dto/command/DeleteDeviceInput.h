#pragma once

#include <cstdint>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::device_log::dto::command {
    // 删除设备输入
    struct DeleteDeviceInput {
        std::int64_t deviceId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
    };
} // namespace project_tracker::modules::device_log::dto::command
