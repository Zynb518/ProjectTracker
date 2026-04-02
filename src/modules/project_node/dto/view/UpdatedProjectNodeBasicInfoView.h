#pragma once

#include <cstdint>
#include <string>

namespace project_tracker::modules::project_node::dto::view {
    // 修改阶段节点基础信息成功后的展示对象
    struct UpdatedProjectNodeBasicInfoView {
        std::int64_t id;
        std::string name;
        std::string description;
        std::string plannedStartDate;
        std::string plannedEndDate;
        std::string updatedAt;
    };
} // namespace project_tracker::modules::project_node::dto::view
