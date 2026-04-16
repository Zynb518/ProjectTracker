#include <iostream>
#include <string>

#include <drogon/drogon.h>

#include "http/RequestLogging.h"
#include "bootstrap/ThreadNumConfig.h"
#include "modules/project/controller/ProjectController.h"

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

        project_tracker::http::registerRequestLogging(drogon::app());

        LOG_INFO << "服务启动完成，开始接收请求";

        drogon::app().run();
    } catch (const std::exception &ex) {
        LOG_ERROR << "启动失败，原因：" << ex.what();
        return 1;
    }

    return 0;
}
