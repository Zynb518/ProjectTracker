#pragma once

#include <cstdint>

namespace project_tracker::modules::project_member::dto::view {
    // 移除项目成员结果
    struct RemovedProjectMemberView {
        std::int64_t projectId;
        std::int64_t userId;
    };
} // namespace project_tracker::modules::project_member::dto::view
