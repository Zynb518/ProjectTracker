#pragma once

#include <cstdint>
#include <string>

#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project_member::dto::view {
    // 项目成员列表项展示对象
    struct ProjectMemberListItemView {
        std::int64_t projectId;
        std::int64_t userId;
        std::string username;
        std::string realName;
        user::domain::SystemRole systemRole;
        user::domain::UserStatus status;
        std::string joinedAt;
        bool isOwner;
    };
} // namespace project_tracker::modules::project_member::dto::view
