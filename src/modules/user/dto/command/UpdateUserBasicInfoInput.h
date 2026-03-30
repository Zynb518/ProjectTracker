#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace project_tracker::modules::user::dto::command {
    // 修改用户基础信息输入
    struct UpdateUserBasicInfoInput {
        std::int64_t userId;
        std::optional<std::string> realName;
        std::optional<int> systemRole;
        std::optional<std::string> email;
        std::optional<std::string> phone;
    };
} // namespace project_tracker::modules::user::dto::command
