#include <string>

#include <drogon/drogon.h>

#include "bootstrap/StartupLogMessage.h"
#include "bootstrap/ThreadNumConfig.h"

int main(int argc, char **argv) {
    std::string configPath = "config/config.dev.json";
    if (argc > 1) {
        configPath = argv[1];
    }

    try {
        LOG_INFO << project_tracker::bootstrap::buildStartupConfigMessage(configPath);
        drogon::app().loadConfigFile(configPath);
        const auto threadNum = project_tracker::bootstrap::readConfiguredThreadNum(
            drogon::app().getCustomConfig());
        if (threadNum) {
            drogon::app().setThreadNum(*threadNum);
        }
        LOG_INFO << project_tracker::bootstrap::buildThreadNumMessage(threadNum);
        drogon::app().enableSession(
            24 * 60 * 60,
            drogon::Cookie::SameSite::kLax,
            "JSESSIONID");
        LOG_INFO << "Session 已启用，Cookie 名称: JSESSIONID，SameSite=Lax";
        drogon::app().run();
    } catch (const std::exception &ex) {
        LOG_ERROR << project_tracker::bootstrap::buildStartupFailureMessage(
            configPath,
            ex.what());
        return 1;
    }

    return 0;
}
