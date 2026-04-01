#pragma once

#include <cstdint>
#include <string>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project::dto::command {
    // 获取项目负责人转交候选列表输入
    struct ListProjectOwnerCandidatesInput {
        std::int64_t projectId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
        std::string keyword;
        std::int64_t page = 1;
        std::int64_t pageSize = 10;
    };
} // namespace project_tracker::modules::project::dto::command
