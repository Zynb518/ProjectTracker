#pragma once

#include <string>

#include <json/value.h>

namespace project_tracker::common::util {
    // 读取必填字符串字段
    inline bool readRequiredString(
        const Json::Value &json,
        const std::string &key,
        std::string &value) {
        if (!json.isObject() || !json.isMember(key)) {
            return false;
        }

        const Json::Value &field = json[key];
        if (!field.isString()) {
            return false;
        }

        value = field.asString();
        return !value.empty();
    }

    // 读取必填整数字段
    inline bool readRequiredInt(const Json::Value &json,
                                const std::string &key,
                                int &value) {
        if (!json.isObject() || !json.isMember(key)) {
            return false;
        }

        const Json::Value &field = json[key];
        if (!field.isInt()) {
            return false;
        }

        value = field.asInt();
        return true;
    }
} // namespace project_tracker::common::util
