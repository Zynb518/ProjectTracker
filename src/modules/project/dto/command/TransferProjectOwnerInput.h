#pragma once

#include <cstdint>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project::dto::command {
    // 转交项目负责人输入
    struct TransferProjectOwnerInput {
        std::int64_t projectId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
        std::int64_t targetUserId;
    };
} // namespace project_tracker::modules::project::dto::command
