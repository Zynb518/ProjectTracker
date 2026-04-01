#pragma once

#include <cstdint>
#include <string>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project::dto::view {
    // 项目负责人转交候选项展示对象
    struct ProjectOwnerCandidateView {
        std::int64_t id;
        std::string username;
        std::string realName;
        user::domain::SystemRole systemRole;
        user::domain::UserStatus status;
        bool isProjectMember;
    };
} // namespace project_tracker::modules::project::dto::view
