#include "http/RequestLogging.h"

#include <chrono>
#include <sstream>

#include <drogon/utils/Utilities.h>

namespace project_tracker::http {
    namespace {
        constexpr const char *kRequestIdKey = "request_id";
        constexpr const char *kRequestStartTimeKey = "request_start_time";
        constexpr std::int64_t kSlowRequestThresholdMs = 1000;
    }

    std::string getOrCreateRequestId(const drogon::HttpRequestPtr &request) {
        if (request->attributes()->find(kRequestIdKey)) {
            return request->attributes()->get<std::string>(kRequestIdKey);
        }

        const auto requestId = drogon::utils::getUuid();
        request->attributes()->insert(kRequestIdKey, requestId);
        return requestId;
    }

    RequestLogLevel resolveRequestLogLevel(int statusCode, std::int64_t elapsedMs) {
        if (statusCode >= 500) {
            return RequestLogLevel::Error;
        }
        if (statusCode >= 400 || elapsedMs >= kSlowRequestThresholdMs) {
            return RequestLogLevel::Warn;
        }
        return RequestLogLevel::Info;
    }

    void registerRequestLogging(drogon::HttpAppFramework &app) {
        app.registerPreRoutingAdvice(
            [](const drogon::HttpRequestPtr &request) {
                request->attributes()->insert(
                    kRequestStartTimeKey,
                    std::chrono::steady_clock::now());
                (void)getOrCreateRequestId(request);
            });

        app.registerPreSendingAdvice(
            [](const drogon::HttpRequestPtr &request,
               const drogon::HttpResponsePtr &response) {
                int64_t elapsedMs = 0;
                if (request->attributes()->find(kRequestStartTimeKey)) {
                    const auto startedAt = request->attributes()->
                        get<std::chrono::steady_clock::time_point>(kRequestStartTimeKey);
                    elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - startedAt).count();
                }

                const auto statusCode = static_cast<int>(response->getStatusCode());
                const auto method = request->getMethodString();
                const auto &path = request->getPath();
                const auto ip = request->getPeerAddr().toIp();
                const auto requestId = getOrCreateRequestId(request);
                const auto logLevel = resolveRequestLogLevel(statusCode, elapsedMs);
                const auto isSlowRequest = elapsedMs >= kSlowRequestThresholdMs;

                const auto userId = request->getSession()->getOptional<std::int64_t>("user_id");
                const std::string userIdText = userId.has_value() ? std::to_string(*userId) : "anonymous";

                std::ostringstream stream;
                stream << "请求日志"
                       << " | request_id=" << requestId
                       << " | 方法=" << method
                       << " | 路径=" << path
                       << " | 状态码=" << statusCode
                       << " | 耗时_ms=" << elapsedMs
                       << " | IP=" << ip
                       << " | user_id=" << userIdText;
                if (isSlowRequest) {
                    stream << " | slow_request=true";
                }
                const auto message = stream.str();

                if (logLevel == RequestLogLevel::Error) {
                    LOG_ERROR << message;
                } else if (logLevel == RequestLogLevel::Warn) {
                    LOG_WARN << message;
                } else {
                    LOG_INFO << message;
                }
            });
    }
} // namespace project_tracker::http
