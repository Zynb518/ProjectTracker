#pragma once

#include <cstdint>
#include <string>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project_member::dto::view {
    // 项目成员候选项展示对象
    struct ProjectMemberCandidateView {
        std::int64_t id;
        std::string username;
        std::string realName;
        user::domain::SystemRole systemRole;
        user::domain::UserStatus status;
    };
} // namespace project_tracker::modules::project_member::dto::view
