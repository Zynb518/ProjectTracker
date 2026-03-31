#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "modules/project/domain/ProjectEnums.h"

namespace project_tracker::modules::project::dto::view {
    // 修改项目状态成功后的展示对象
    struct UpdatedProjectStatusView {
        std::int64_t id;
        domain::ProjectStatus status;
        std::optional<std::string> completedAt;
        std::string updatedAt;
    };
} // namespace project_tracker::modules::project::dto::view
