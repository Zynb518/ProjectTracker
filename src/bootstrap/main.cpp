#include <iostream>
#include <string>

#include <drogon/drogon.h>

#include "bootstrap/ThreadNumConfig.h"

int main(int argc, char **argv) {
    std::string configPath = "config/config.dev.json";
    if (argc > 1) {
        configPath = argv[1];
    }

    try {
        drogon::app().loadConfigFile(configPath);
        const auto threadNum = project_tracker::bootstrap::readConfiguredThreadNum(
            drogon::app().getCustomConfig());
        if (threadNum) {
            drogon::app().setThreadNum(*threadNum);
        }
        drogon::app().enableSession(
            24 * 60 * 60,
            drogon::Cookie::SameSite::kLax,
            "JSESSIONID");
        drogon::app().run();
    } catch (const std::exception &ex) {
        std::cerr << "failed to start: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
