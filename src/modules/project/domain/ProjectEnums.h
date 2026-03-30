#pragma once

namespace project_tracker::modules::project::domain {
    // 项目状态
    // 对齐 project.status：
    // 1 = 未开始
    // 2 = 进行中
    // 3 = 已完成
    // 4 = 已延期
    enum class ProjectStatus : int {
        NotStarted = 1,
        InProgress = 2,
        Completed = 3,
        Delayed = 4
    };

    inline constexpr int toInt(ProjectStatus status) {
        return static_cast<int>(status);
    }
} // namespace project_tracker::modules::project::domain
