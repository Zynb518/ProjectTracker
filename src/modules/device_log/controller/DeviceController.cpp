#include "modules/device_log/controller/DeviceController.h"

#include <drogon/drogon.h>

#include "common/api/ApiResponse.h"
#include "common/error/ErrorCode.h"
#include "common/util/JsonUtil.h"
#include "common/util/QueryParamUtil.h"
#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::device_log::controller {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;
    namespace util = project_tracker::common::util;

    drogon::Task<drogon::HttpResponsePtr>
    DeviceController::createDevice(drogon::HttpRequestPtr request) {
        const auto &json = request->getJsonObject();
        if (!json || !json->isObject()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "请求体必须是 JSON 对象");
        }

        dto::command::CreateDeviceInput input;

        if (!util::readRequiredString(*json, "name", input.name)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "name 必须是非空字符串");
        }

        if (!util::readOptionalString(*json, "description", input.description)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "description 必须是字符串");
        }

        const auto view = co_await deviceService_.createDevice(input);
        
        Json::Value result(Json::objectValue);
        result["id"] = view.id;
        co_return api::ok(result);
    }

    drogon::Task<drogon::HttpResponsePtr>
    DeviceController::deleteDevice(drogon::HttpRequestPtr request, std::int64_t deviceId) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");
        if (!userId || *userId <= 0) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        const auto systemRole = session->getOptional<user::domain::SystemRole>("system_role");
        if (!systemRole) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        dto::command::DeleteDeviceInput input{
            .deviceId = deviceId,
            .operatorUserId = *userId,
            .operatorUserRole = *systemRole
        };

        co_await deviceService_.deleteDevice(input);
        co_return api::ok(Json::Value(Json::nullValue));
    }

    drogon::Task<drogon::HttpResponsePtr>
    DeviceController::listDevices(drogon::HttpRequestPtr request) {
        repository::DeviceListQuery query;
        
        std::optional<std::int64_t> pageOpt;
        if (!util::readPositiveQueryInt64(request, "page", pageOpt)) {
            co_return api::fail(drogon::k400BadRequest, error::ErrorCode::InvalidParameter, "page 必须是正整数");
        }
        query.page = pageOpt.value_or(1);

        std::optional<std::int64_t> pageSizeOpt;
        if (!util::readPositiveQueryInt64(request, "page_size", pageSizeOpt)) {
            co_return api::fail(drogon::k400BadRequest, error::ErrorCode::InvalidParameter, "page_size 必须是正整数");
        }
        query.pageSize = pageSizeOpt.value_or(10);

        if (query.pageSize > 100) query.pageSize = 100;

        const auto pageResult = co_await deviceService_.listDevices(query);

        Json::Value listJson(Json::arrayValue);
        for (const auto &item : pageResult.list) {
            Json::Value itemJson(Json::objectValue);
            itemJson["id"] = item.id;
            itemJson["name"] = item.name;
            itemJson["description"] = item.description;
            itemJson["created_at"] = item.createdAt;
            itemJson["updated_at"] = item.updatedAt;
            listJson.append(itemJson);
        }

        Json::Value result(Json::objectValue);
        result["list"] = listJson;
        result["total"] = pageResult.total;
        result["page"] = pageResult.page;
        result["page_size"] = pageResult.pageSize;

        co_return api::ok(result);
    }

    drogon::Task<drogon::HttpResponsePtr>
    DeviceController::createDeviceUsageLog(drogon::HttpRequestPtr request, std::int64_t deviceId) {
        const auto &session = request->getSession();
        const auto userId = session->getOptional<std::int64_t>("user_id");
        if (!userId || *userId <= 0) {
            co_return api::fail(
                drogon::k401Unauthorized,
                error::ErrorCode::Unauthorized,
                "未登录或登录态失效");
        }

        const auto &json = request->getJsonObject();
        if (!json || !json->isObject()) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "请求体必须是 JSON 对象");
        }

        dto::command::CreateDeviceUsageLogInput input{
            .deviceId = deviceId,
            .operatorUserId = *userId
        };

        if (!util::readRequiredString(*json, "purpose", input.purpose)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "purpose 必须是非空字符串");
        }

        const auto view = co_await deviceService_.createDeviceUsageLog(input);
        
        Json::Value result(Json::objectValue);
        result["id"] = view.id;
        co_return api::ok(result);
    }

    drogon::Task<drogon::HttpResponsePtr>
    DeviceController::listDeviceUsageLogs(drogon::HttpRequestPtr request, std::int64_t deviceId) {
        repository::DeviceUsageLogListQuery query;
        query.deviceId = deviceId;
        
        std::optional<std::int64_t> pageOpt;
        if (!util::readPositiveQueryInt64(request, "page", pageOpt)) {
            co_return api::fail(drogon::k400BadRequest, error::ErrorCode::InvalidParameter, "page 必须是正整数");
        }
        query.page = pageOpt.value_or(1);

        std::optional<std::int64_t> pageSizeOpt;
        if (!util::readPositiveQueryInt64(request, "page_size", pageSizeOpt)) {
            co_return api::fail(drogon::k400BadRequest, error::ErrorCode::InvalidParameter, "page_size 必须是正整数");
        }
        query.pageSize = pageSizeOpt.value_or(10);

        if (query.pageSize > 100) query.pageSize = 100;

        const auto pageResult = co_await deviceService_.listDeviceUsageLogs(query);

        Json::Value listJson(Json::arrayValue);
        for (const auto &item : pageResult.list) {
            Json::Value itemJson(Json::objectValue);
            itemJson["id"] = item.id;
            itemJson["device_id"] = item.deviceId;
            itemJson["user_id"] = item.userId;
            itemJson["operator_name"] = item.operatorName;
            itemJson["purpose"] = item.purpose;
            itemJson["created_at"] = item.createdAt;
            listJson.append(itemJson);
        }

        Json::Value result(Json::objectValue);
        result["list"] = listJson;
        result["total"] = pageResult.total;
        result["page"] = pageResult.page;
        result["page_size"] = pageResult.pageSize;

        co_return api::ok(result);
    }
} // namespace project_tracker::modules::device_log::controller
