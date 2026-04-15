#pragma once

#include <optional>
#include <string>

namespace project_tracker::bootstrap {
    inline std::string buildStartupConfigMessage(const std::string &configPath) {
        return "Project Tracker 启动中，配置文件: " + configPath;
    }

    inline std::string buildThreadNumMessage(
        const std::optional<std::size_t> &threadNum) {
        if (threadNum.has_value()) {
            return "工作线程数已配置为: " + std::to_string(*threadNum);
        }

        return "工作线程数: 使用 Drogon 默认值";
    }

    inline std::string buildStartupFailureMessage(const std::string &configPath,
                                                  const std::string &reason) {
        return "启动失败，配置文件 " + configPath + "，原因: " + reason;
    }
} // namespace project_tracker::bootstrap
