#include "modules/device_log/service/DeviceService.h"
#include <drogon/drogon.h>
#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::device_log::service {
    namespace error = project_tracker::common::error;

    drogon::Task<dto::view::CreatedObjectIdView>
    DeviceService::createDevice(const dto::command::CreateDeviceInput &input) const {
        try {
            const auto dbClient = drogon::app().getDbClient();
            co_return co_await deviceRepository_.insertDevice(dbClient, input);
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "获取数据库客户端失败");
        }
    }

    drogon::Task<void>
    DeviceService::deleteDevice(const dto::command::DeleteDeviceInput &input) const {
        if (input.operatorUserRole != modules::user::domain::SystemRole::Admin) {
            error::throwForbidden(
                error::ErrorCode::Forbidden,
                "当前操作者无权限删除设备");
        }

        try {
            const auto dbClient = drogon::app().getDbClient();
            bool deleted = co_await deviceRepository_.deleteDevice(dbClient, input.deviceId);
            if (!deleted) {
                error::throwNotFound(
                    error::ErrorCode::DeviceNotFound,
                    "设备不存在");
            }
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "删除设备失败：数据库错误");
        }
    }

    drogon::Task<repository::DeviceListPage>
    DeviceService::listDevices(const repository::DeviceListQuery &query) const {
        try {
            const auto dbClient = drogon::app().getDbClient();
            co_return co_await deviceRepository_.listDevices(dbClient, query);
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "获取设备列表失败：数据库错误");
        }
    }

    drogon::Task<dto::view::CreatedObjectIdView>
    DeviceService::createDeviceUsageLog(const dto::command::CreateDeviceUsageLogInput &input) const {
        try {
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();
            
            bool exists = co_await deviceRepository_.checkDeviceExistsForUpdate(transaction, input.deviceId);
            if (!exists) {
                error::throwNotFound(
                    error::ErrorCode::DeviceNotFound,
                    "设备不存在");
            }
            
            co_return co_await deviceRepository_.insertDeviceUsageLog(transaction, input);
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "新建设备使用记录失败：数据库错误");
        }
    }

    drogon::Task<repository::DeviceUsageLogListPage>
    DeviceService::listDeviceUsageLogs(const repository::DeviceUsageLogListQuery &query) const {
        try {
            const auto dbClient = drogon::app().getDbClient();
            
            bool exists = co_await deviceRepository_.checkDeviceExists(dbClient, query.deviceId);
            if (!exists) {
                error::throwNotFound(
                    error::ErrorCode::DeviceNotFound,
                    "设备不存在");
            }
            
            co_return co_await deviceRepository_.listDeviceUsageLogs(dbClient, query);
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "获取设备使用记录失败：数据库错误");
        }
    }
} // namespace project_tracker::modules::device_log::service
