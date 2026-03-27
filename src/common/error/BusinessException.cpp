#include "common/error/BusinessException.h"

#include <utility>

namespace project_tracker::common::error {
    BusinessException::BusinessException(
        drogon::HttpStatusCode status,
        ErrorCode code,
        std::string message)
        : status_(status),
          code_(code),
          message_(std::move(message)) {
    }

    drogon::HttpStatusCode BusinessException::status() const noexcept {
        return status_;
    }

    ErrorCode BusinessException::code() const noexcept {
        return code_;
    }

    const std::string &BusinessException::message() const noexcept {
        return message_;
    }

    const char *BusinessException::what() const noexcept {
        return message_.c_str();
    }
} // namespace project_tracker::common::error
