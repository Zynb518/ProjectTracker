#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace project_tracker::modules::project_node::dto::view {
    // 调整阶段节点顺序成功后，单个节点的返回对象
    struct ReorderedProjectNodeItemView {
        std::int64_t nodeId;
        int sequenceNo;
    };

    // 调整阶段节点顺序成功后的展示对象
    struct ReorderedProjectNodesView {
        std::int64_t projectId;
        std::vector<ReorderedProjectNodeItemView> nodes;
        std::string updatedAt;
    };
} // namespace project_tracker::modules::project_node::dto::view
