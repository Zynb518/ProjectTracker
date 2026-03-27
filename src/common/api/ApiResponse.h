#pragma once

#include <drogon/HttpResponse.h>
#include <json/value.h>

#include "common/error/BusinessException.h"
#include "common/error/ErrorCode.h"

namespace project_tracker::common::api {
    // 统一组织 HTTP 返回结果
    // 说明：
    // 1. HTTP 状态码和业务错误码分开维护
    // 2. Controller 层直接返回 HttpResponsePtr，减少重复拼装 JSON 的代码
    inline drogon::HttpResponsePtr make(
        drogon::HttpStatusCode status,
        error::ErrorCode code,
        const std::string &message,
        const Json::Value &data) {
        Json::Value body(Json::objectValue);
        body["code"] = error::toInt(code);
        body["message"] = message;
        body["data"] = data;

        auto response = drogon::HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(status);
        return response;
    }

    // 成功返回
    inline drogon::HttpResponsePtr ok(
        const Json::Value &data = Json::Value(Json::nullValue),
        const std::string &message = "ok") {
        return make(drogon::k200OK, error::ErrorCode::kSuccess, message, data);
    }

    // 失败返回
    inline drogon::HttpResponsePtr fail(
        drogon::HttpStatusCode status,
        error::ErrorCode code,
        const std::string &message) {
        return make(status, code, message, Json::Value(Json::nullValue));
    }

    // 把业务异常转换成统一返回结果
    inline drogon::HttpResponsePtr fromException(
        const error::BusinessException &exception) {
        return fail(exception.status(), exception.code(), exception.message());
    }
} // namespace project_tracker::common::api
