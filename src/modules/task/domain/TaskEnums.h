#pragma once

namespace project_tracker::modules::task::domain {
    // 子任务状态
    // 对齐 sub_task.status：
    // 1 = 未开始
    // 2 = 进行中
    // 3 = 已完成
    // 4 = 已延期
    enum class TaskStatus : int {
        NotStarted = 1,
        InProgress = 2,
        Completed = 3,
        Delayed = 4
    };

    // 子任务优先级
    // 对齐 sub_task.priority：
    // 1 = 低
    // 2 = 中
    // 3 = 高
    enum class TaskPriority : int {
        Low = 1,
        Medium = 2,
        High = 3
    };

    inline constexpr int toInt(TaskStatus status) {
        return static_cast<int>(status);
    }

    inline constexpr int toInt(TaskPriority priority) {
        return static_cast<int>(priority);
    }
} // namespace project_tracker::modules::task::domain
