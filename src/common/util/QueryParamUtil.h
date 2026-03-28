#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include <drogon/HttpRequest.h>

namespace project_tracker::common::util {
    // 读取可选 query 字符串参数。
    inline std::optional<std::string> readQueryString(const drogon::HttpRequestPtr &request,
                                                      const std::string &key) {
        const auto &parameters = request->getParameters();
        const auto it = parameters.find(key);
        if (it == parameters.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    // 读取可选 query 整数参数。
    inline bool readQueryInt(const drogon::HttpRequestPtr &request,
                             const std::string &key,
                             std::optional<int> &value) {
        if (request->getParameters().find(key) == request->getParameters().end()) {
            value = std::nullopt;
            return true;
        }

        const auto parsedValue = request->getOptionalParameter<int>(key);
        if (!parsedValue) {
            value = std::nullopt;
            return false;
        }

        value = *parsedValue;
        return true;
    }

    // 读取可选正整数 query 参数。
    inline bool readPositiveQueryInt(const drogon::HttpRequestPtr &request,
                                     const std::string &key,
                                     std::optional<int> &value) {
        if (!readQueryInt(request, key, value)) {
            return false;
        }

        return !value || *value > 0;
    }

    // 读取可选正整数 query 长整数参数。
    inline bool readPositiveQueryInt64(const drogon::HttpRequestPtr &request,
                                       const std::string &key,
                                       std::optional<std::int64_t> &value) {
        if (request->getParameters().find(key) == request->getParameters().end()) {
            value = std::nullopt;
            return true;
        }

        const auto parsedValue = request->getOptionalParameter<std::int64_t>(key);
        if (!parsedValue) {
            value = std::nullopt;
            return false;
        }

        value = *parsedValue;
        return *value > 0;
    }
} // namespace project_tracker::common::util
