

#include "http/RequestLogging.h"
#include <chrono>
#include <sstream>

namespace project_tracker::http {

    namespace {
        constexpr const char* kRequestStartTimeKey = "request_start_time";
    }

    void registerRequestLogging(drogon::HttpAppFramework &app) {
        app.registerPreRoutingAdvice(
            [](const drogon::HttpRequestPtr &request) {
                // 只属于当前请求
                request->attributes()->insert(
                    kRequestStartTimeKey,
                    std::chrono::steady_clock::now());
            });

        app.registerPreSendingAdvice(
            [](const drogon::HttpRequestPtr &request,
               const drogon::HttpResponsePtr &response) {
                (void) response;
                // 计算时间
                int64_t elapsedMs = 0;
                if (request->attributes()->find(kRequestStartTimeKey)) {
                    const auto startedAt = request->attributes()->
                        get<std::chrono::steady_clock::time_point>(kRequestStartTimeKey);
                    elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - startedAt).count();
                }
                // 拿到 方法/路径/状态码 IP
                const auto statusCode = static_cast<int>(response->getStatusCode());
                const auto method = request->getMethodString();
                const auto &path = request->getPath();
                const auto ip = request->getPeerAddr().toIp();

                const auto userId = request->getSession()->getOptional<std::int64_t>("user_id");
                const std::string userIdText = userId.has_value() ? std::to_string(*userId) : "anonymous";

                // 组装日志
                std::ostringstream stream;
                stream  << "请求日志"
                        << " | 方法=" << method
                        << " | 路径=" << path
                        << " | 状态码=" << statusCode
                        << " | 耗时_ms=" << elapsedMs
                        << " | IP=" << ip
                        << " | user_id=" << userIdText;
                const auto message = stream.str();

                if (statusCode >= 500) {
                    LOG_ERROR << message;
                } else if (statusCode >= 400) {
                    LOG_WARN << message;
                } else {
                    LOG_INFO << message;
                }

            });

    }
}
