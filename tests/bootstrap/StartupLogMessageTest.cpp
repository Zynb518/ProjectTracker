#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>

#include "bootstrap/StartupLogMessage.h"

int main() {
    {
        const auto message = project_tracker::bootstrap::buildStartupConfigMessage(
            "/opt/project-tracker/config/config.prod.json");
        if (message !=
            "Project Tracker 启动中，配置文件: /opt/project-tracker/config/config.prod.json") {
            std::cerr << "expected startup config message to include config path\n";
            return EXIT_FAILURE;
        }
    }

    {
        const auto message =
            project_tracker::bootstrap::buildThreadNumMessage(std::optional<std::size_t>(8));
        if (message != "工作线程数已配置为: 8") {
            std::cerr << "expected configured thread number message\n";
            return EXIT_FAILURE;
        }
    }

    {
        const auto message =
            project_tracker::bootstrap::buildThreadNumMessage(std::nullopt);
        if (message != "工作线程数: 使用 Drogon 默认值") {
            std::cerr << "expected default thread number message\n";
            return EXIT_FAILURE;
        }
    }

    {
        const auto message = project_tracker::bootstrap::buildStartupFailureMessage(
            "/opt/project-tracker/config/config.prod.json",
            "database authentication failed");
        if (message !=
            "启动失败，配置文件 /opt/project-tracker/config/config.prod.json，原因: "
            "database authentication failed") {
            std::cerr << "expected startup failure message to include config path and reason\n";
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
