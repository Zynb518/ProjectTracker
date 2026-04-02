#pragma once

#include <cstdint>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project_member::dto::command {
    // 添加项目成员输入
    struct AddProjectMemberInput {
        std::int64_t projectId;
        std::int64_t userId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
    };
} // namespace project_tracker::modules::project_member::dto::command
