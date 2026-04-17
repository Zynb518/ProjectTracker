#include <iostream>
#include <string>

#include <drogon/drogon.h>
#include <unistd.h>

#include "http/RequestLogging.h"
#include "http/ExceptionHandling.h"
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

        project_tracker::http::registerExceptionHandling(drogon::app());
        project_tracker::http::registerRequestLogging(drogon::app());

        const std::string threadNumText = threadNum.has_value()
                                              ? std::to_string(*threadNum)
                                              : "default";
        LOG_INFO << "服务启动完成"
                 << " | config_path=" << configPath
                 << " | threads_num=" << threadNumText
                 << " | session_cookie=JSESSIONID"
                 << " | pid=" << ::getpid()
                 << " | 开始接收请求";


        drogon::app().run();
    } catch (const std::exception &ex) {
        LOG_ERROR << "启动失败，原因：" << ex.what();
        return 1;
    }

    return 0;
}
