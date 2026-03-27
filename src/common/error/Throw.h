#pragma once

#include <string>

#include "common/error/BusinessException.h"

namespace project_tracker::common::error {
    // 抛出通用业务异常
    [[noreturn]] void throwBusinessError(drogon::HttpStatusCode status,
                                         ErrorCode code,
                                         const std::string &message);

    // 常见 HTTP 状态对应的快捷抛出函数
    [[noreturn]] void throwBadRequest(ErrorCode code,
                                      const std::string &message);

    [[noreturn]] void throwUnauthorized(ErrorCode code,
                                        const std::string &message);

    [[noreturn]] void throwForbidden(ErrorCode code,
                                     const std::string &message);

    [[noreturn]] void throwNotFound(ErrorCode code,
                                    const std::string &message);

    [[noreturn]] void throwConflict(ErrorCode code,
                                    const std::string &message);

    [[noreturn]] void throwInternalError(ErrorCode code,
                                         const std::string &message);
} // namespace project_tracker::common::error
