#pragma once

#include <exception>
#include <string>

#include <drogon/HttpTypes.h>

#include "common/error/ErrorCode.h"

namespace project_tracker::common::error {
    // 业务异常
    // 说明：
    // 1. Service 层可以直接抛出这个异常
    // 2. 后续统一异常处理可以把它转换成标准 JSON 返回
    class BusinessException : public std::exception {
    public:
        BusinessException(
            drogon::HttpStatusCode status,
            ErrorCode code,
            std::string message);

        [[nodiscard]] drogon::HttpStatusCode status() const noexcept;

        [[nodiscard]] ErrorCode code() const noexcept;

        [[nodiscard]] const std::string &message() const noexcept;

        [[nodiscard]] const char *what() const noexcept override;

    private:
        drogon::HttpStatusCode status_;
        ErrorCode code_;
        std::string message_;
    };
} // namespace project_tracker::common::error
