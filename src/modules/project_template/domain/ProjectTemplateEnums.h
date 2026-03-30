#pragma once

namespace project_tracker::modules::project_template::domain {
    // 项目模板状态
    // 对齐 project_template.status：
    // 1 = 启用
    // 2 = 停用
    enum class ProjectTemplateStatus : int {
        Enabled = 1,
        Disabled = 2
    };

    inline constexpr int toInt(ProjectTemplateStatus status) {
        return static_cast<int>(status);
    }
} // namespace project_tracker::modules::project_template::domain
