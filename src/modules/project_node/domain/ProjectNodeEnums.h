#pragma once

namespace project_tracker::modules::project_node::domain {
    // 阶段节点状态
    // 对齐 project_node.status：
    // 1 = 未开始
    // 2 = 进行中
    // 3 = 已完成
    // 4 = 已延期
    enum class ProjectNodeStatus : int {
        NotStarted = 1,
        InProgress = 2,
        Completed = 3,
        Delayed = 4
    };

    inline constexpr int toInt(ProjectNodeStatus status) {
        return static_cast<int>(status);
    }
} // namespace project_tracker::modules::project_node::domain
