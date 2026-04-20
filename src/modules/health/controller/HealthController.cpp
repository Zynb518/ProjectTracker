#include "modules/health/controller/HealthController.h"

#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>
#include <json/value.h>

#include "common/api/ApiResponse.h"

namespace project_tracker::modules::health::controller {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;

    namespace {
        Json::Value buildHealthData() {
            Json::Value data(Json::objectValue);
            data["status"] = "ok";
            return data;
        }

        Json::Value buildReadyData(const std::string &status, const std::string &dbStatus) {
            Json::Value data(Json::objectValue);
            data["status"] = status;
            data["checks"] = Json::Value(Json::objectValue);
            data["checks"]["db"] = dbStatus;
            return data;
        }
    } // namespace

    drogon::Task<drogon::HttpResponsePtr>
    HealthController::health(drogon::HttpRequestPtr request) {
        (void)request;

        co_return api::ok(buildHealthData());
    }

    drogon::Task<drogon::HttpResponsePtr>
    HealthController::ready(drogon::HttpRequestPtr request) {
        (void)request;

        try {
            const auto dbClient = drogon::app().getDbClient();
            const auto ready = co_await healthService_.isDatabaseReady(dbClient);
            if (!ready) {
                co_return api::make(
                    drogon::k503ServiceUnavailable,
                    error::ErrorCode::InternalError,
                    "service unavailable",
                    buildReadyData("fail", "down"));
            }
            co_return api::ok(buildReadyData("ok", "up"));
        } catch (const drogon::orm::DrogonDbException &) {
            co_return api::make(
                drogon::k503ServiceUnavailable,
                error::ErrorCode::InternalError,
                "service unavailable",
                buildReadyData("fail", "down"));
        } catch (const std::exception &) {
            co_return api::make(
                drogon::k503ServiceUnavailable,
                error::ErrorCode::InternalError,
                "service unavailable",
                buildReadyData("fail", "down"));
        }
    }
} // namespace project_tracker::modules::health::controller
