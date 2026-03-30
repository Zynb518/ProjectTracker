#pragma once

#include <cstdint>
#include <string>

namespace project_tracker::modules::project::dto::command {
    // 创建项目输入
    struct CreateProjectInput {
        std::int64_t creatorUserId;
        std::string name;
        std::string description;
        std::string plannedStartDate;
        std::string plannedEndDate;
    };
} // namespace project_tracker::modules::project::dto::command
