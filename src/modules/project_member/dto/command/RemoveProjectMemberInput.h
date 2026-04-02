#pragma once

#include <cstdint>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project_member::dto::command {
    // 移除项目成员输入
    struct RemoveProjectMemberInput {
        std::int64_t projectId;
        std::int64_t memberUserId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
    };
} // namespace project_tracker::modules::project_member::dto::command
