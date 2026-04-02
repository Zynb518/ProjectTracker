#pragma once

#include <cstdint>
#include <string>

namespace project_tracker::modules::project_member::dto::view {
    // 添加项目成员结果
    struct AddedProjectMemberView {
        std::int64_t projectId;
        std::int64_t userId;
        std::string joinedAt;
    };
} // namespace project_tracker::modules::project_member::dto::view
