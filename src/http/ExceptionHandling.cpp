#include "http/ExceptionHandling.h"

#include <drogon/drogon.h>

#include "common/api/ApiResponse.h"
#include "common/error/BusinessException.h"
#include "http/RequestLogging.h"

namespace project_tracker::http {
    ExceptionProcessResult processException(const std::exception &exception) {
        ExceptionProcessResult result;
        if (const auto *businessException =
                dynamic_cast<const common::error::BusinessException *>(&exception)) {
            result.statusCode = businessException->status();
            result.errorCode = businessException->code();
            result.message = businessException->message();
            result.logAsError = (businessException->status() >= drogon::k500InternalServerError);
        } else {
            result.statusCode = drogon::k500InternalServerError;
            result.errorCode = common::error::ErrorCode::InternalError;
            result.message = "服务器内部错误";
            result.logAsError = true;
        }
        return result;
    }

    void registerExceptionHandling(drogon::HttpAppFramework &app) {
        app.setExceptionHandler(
            [](const std::exception &exception,
               const drogon::HttpRequestPtr &request,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
                const auto requestId = getOrCreateRequestId(request);
                const auto result = processException(exception);

                if (result.logAsError) {
                    LOG_ERROR << "全局异常"
                              << " | request_id=" << requestId
                              << " | 方法=" << request->getMethodString()
                              << " | 路径=" << request->getPath()
                              << " | 原因=" << exception.what();
                } else {
                    LOG_WARN << "全局异常"
                             << " | request_id=" << requestId
                             << " | 方法=" << request->getMethodString()
                             << " | 路径=" << request->getPath()
                             << " | 原因=" << exception.what();
                }

                callback(common::api::fail(result.statusCode, result.errorCode, result.message));
            });
    }
} // namespace project_tracker::http
