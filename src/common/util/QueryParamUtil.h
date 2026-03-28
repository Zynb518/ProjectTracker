#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <drogon/HttpRequest.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::common::util {
    // 读取可选 query 字符串参数。
    inline std::optional<std::string> readQueryString(
        const drogon::HttpRequestPtr &request,
        const std::string &key) {
        const auto &parameters = request->getParameters();
        const auto it = parameters.find(key);
        if (it == parameters.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    // 读取可选 query 整数参数。
    inline std::optional<int> readQueryInt(
        const drogon::HttpRequestPtr &request,
        const std::string &key,
        const std::string &invalidMessage) {
        if (request->getParameters().find(key) == request->getParameters().end()) {
            return std::nullopt;
        }

        const auto value = request->getOptionalParameter<int>(key);
        if (!value) {
            error::throwBadRequest(
                error::ErrorCode::InvalidParameter,
                invalidMessage);
        }

        return value;
    }

    // 读取可选正整数 query 参数。
    inline std::optional<int> readPositiveQueryInt(
        const drogon::HttpRequestPtr &request,
        const std::string &key,
        const std::string &invalidMessage) {
        const auto value = readQueryInt(request, key, invalidMessage);
        if (value && *value <= 0) {
            error::throwBadRequest(
                error::ErrorCode::InvalidParameter,
                invalidMessage);
        }

        return value;
    }

    // 读取可选正整数 query 长整数参数。
    inline std::optional<std::int64_t> readPositiveQueryInt64(
        const drogon::HttpRequestPtr &request,
        const std::string &key,
        const std::string &invalidMessage) {
        if (request->getParameters().find(key) == request->getParameters().end()) {
            return std::nullopt;
        }

        const auto value = request->getOptionalParameter<std::int64_t>(key);
        if (!value || *value <= 0) {
            error::throwBadRequest(
                error::ErrorCode::InvalidParameter,
                invalidMessage);
        }

        return value;
    }
} // namespace project_tracker::common::util
