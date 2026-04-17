#include "http/ExceptionHandling.h"

#include <drogon/drogon.h>

#include "common/api/ApiResponse.h"
#include "common/error/BusinessException.h"
#include "http/RequestLogging.h"

namespace project_tracker::http {
    void registerExceptionHandling(drogon::HttpAppFramework &app) {
        app.setExceptionHandler(
            [](const std::exception &exception,
               const drogon::HttpRequestPtr &request,
               std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
                const auto requestId = getOrCreateRequestId(request);

                if (const auto *businessException =
                        dynamic_cast<const common::error::BusinessException *>(
                            &exception)) {
                    if (businessException->status() >= drogon::k500InternalServerError) {
                        LOG_ERROR << "全局异常"
                                  << " | request_id=" << requestId
                                  << " | 方法=" << request->getMethodString()
                                  << " | 路径=" << request->getPath()
                                  << " | 原因=" << businessException->what();
                    } else {
                        LOG_WARN << "全局异常"
                                 << " | request_id=" << requestId
                                 << " | 方法=" << request->getMethodString()
                                 << " | 路径=" << request->getPath()
                                 << " | 原因=" << businessException->what();
                    }

                    callback(common::api::fromException(*businessException));
                    return;
                }

                LOG_ERROR << "全局异常"
                          << " | request_id=" << requestId
                          << " | 方法=" << request->getMethodString()
                          << " | 路径=" << request->getPath()
                          << " | 原因=" << exception.what();

                callback(common::api::fail(
                    drogon::k500InternalServerError,
                    common::error::ErrorCode::InternalError,
                    "服务器内部错误"));
            });
    }
} // namespace project_tracker::http
