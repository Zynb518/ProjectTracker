#pragma once

#include <limits>
#include <optional>
#include <stdexcept>

#include <json/value.h>

namespace project_tracker::bootstrap {
    inline std::optional<std::size_t>
    readConfiguredThreadNum(const Json::Value &customConfig) {
        if (customConfig.isNull()) {
            return std::nullopt;
        }

        if (!customConfig.isObject()) {
            throw std::runtime_error("custom_config 必须是 JSON 对象");
        }

        const Json::Value &threadNumValue = customConfig["threads_num"];
        if (threadNumValue.isNull()) {
            return std::nullopt;
        }

        if (!threadNumValue.isInt64()) {
            throw std::runtime_error("custom_config.threads_num 必须是大于等于 0 的整数");
        }

        const auto threadNum = threadNumValue.asInt64();
        if (threadNum < 0 ||
            static_cast<unsigned long long>(threadNum) >
                std::numeric_limits<std::size_t>::max()) {
            throw std::runtime_error("custom_config.threads_num 必须是大于等于 0 的整数");
        }

        return static_cast<std::size_t>(threadNum);
    }
} // namespace project_tracker::bootstrap
