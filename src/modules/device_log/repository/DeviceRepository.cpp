#include "modules/device_log/repository/DeviceRepository.h"
#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::device_log::repository {
    drogon::Task<dto::view::CreatedObjectIdView>
    DeviceRepository::insertDevice(const common::db::SqlExecutorPtr &executor,
                                   const dto::command::CreateDeviceInput &input) const {
        try {
            const auto sql = R"(
                INSERT INTO device (name, description, created_at, updated_at)
                VALUES ($1, $2, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)
                RETURNING id;
            )";
            const auto result = co_await executor->execSqlCoro(sql, input.name, input.description);
            if (result.empty()) {
                common::error::throwInternalError(
                    common::error::ErrorCode::InternalError,
                    "新建设备失败：数据库无返回值");
            }
            co_return dto::view::CreatedObjectIdView{
                .id = result[0]["id"].as<std::int64_t>()
            };

        } catch (const drogon::orm::DrogonDbException &e) {
            common::error::throwInternalError(
                common::error::ErrorCode::InternalError,
                std::string("新建设备失败：") + e.base().what());
        }
    }

    drogon::Task<bool>
    DeviceRepository::deleteDevice(const common::db::SqlExecutorPtr &executor,
                                   std::int64_t deviceId) const {
        try {
            const auto sql = R"(
                DELETE FROM device
                WHERE id = $1
            )";
            const auto result = co_await executor->execSqlCoro(sql, deviceId);
            co_return result.affectedRows() > 0;
        } catch (const drogon::orm::DrogonDbException &e) {
            common::error::throwInternalError(
                common::error::ErrorCode::InternalError,
                std::string("删除设备失败：") + e.base().what());
        }
    }

    drogon::Task<DeviceListPage>
    DeviceRepository::listDevices(const common::db::SqlExecutorPtr &executor,
                                  const DeviceListQuery &query) const {
        try {
            const auto countSql = "SELECT COUNT(*) FROM device";
            const auto dataSql = R"(
                SELECT id, name, description,
                       to_char(created_at, 'YYYY-MM-DD"T"HH24:MI:SSOF') as created_at_str,
                       to_char(updated_at, 'YYYY-MM-DD"T"HH24:MI:SSOF') as updated_at_str
                FROM device
                ORDER BY id DESC
                LIMIT $1 OFFSET $2
            )";

            const auto countResult = co_await executor->execSqlCoro(countSql);
            std::int64_t total = 0;
            if (!countResult.empty()) {
                total = countResult[0][0].as<std::int64_t>();
            }

            std::vector<dto::view::DeviceListItemView> list;
            if (total > 0) {
                std::int64_t offset = (query.page - 1) * query.pageSize;
                const auto dataResult = co_await executor->execSqlCoro(dataSql, query.pageSize, offset);
                for (const auto &row : dataResult) {
                    list.push_back({
                        .id = row["id"].as<std::int64_t>(),
                        .name = row["name"].as<std::string>(),
                        .description = row["description"].as<std::string>(),
                        .createdAt = row["created_at_str"].as<std::string>(),
                        .updatedAt = row["updated_at_str"].as<std::string>()
                    });
                }
            }

            co_return DeviceListPage{
                .list = std::move(list),
                .total = total,
                .page = query.page,
                .pageSize = query.pageSize
            };
        } catch (const drogon::orm::DrogonDbException &e) {
            common::error::throwInternalError(
                common::error::ErrorCode::InternalError,
                std::string("获取设备列表失败：") + e.base().what());
        }
    }

    drogon::Task<bool>
    DeviceRepository::checkDeviceExistsForUpdate(const common::db::SqlExecutorPtr &executor,
                                                 std::int64_t deviceId) const {
        try {
            const auto sql = "SELECT 1 FROM device WHERE id = $1 FOR UPDATE";
            const auto result = co_await executor->execSqlCoro(sql, deviceId);
            co_return !result.empty();
        } catch (const drogon::orm::DrogonDbException &e) {
            common::error::throwInternalError(
                common::error::ErrorCode::InternalError,
                std::string("校验设备失败：") + e.base().what());
        }
    }

    drogon::Task<bool>
    DeviceRepository::checkDeviceExists(const common::db::SqlExecutorPtr &executor,
                                        std::int64_t deviceId) const {
        try {
            const auto sql = "SELECT 1 FROM device WHERE id = $1 LIMIT 1";
            const auto result = co_await executor->execSqlCoro(sql, deviceId);
            co_return !result.empty();
        } catch (const drogon::orm::DrogonDbException &e) {
            common::error::throwInternalError(
                common::error::ErrorCode::InternalError,
                std::string("校验设备失败：") + e.base().what());
        }
    }

    drogon::Task<dto::view::CreatedObjectIdView>
    DeviceRepository::insertDeviceUsageLog(const common::db::SqlExecutorPtr &executor,
                                           const dto::command::CreateDeviceUsageLogInput &input) const {
        try {
            const auto sql = R"(
                INSERT INTO device_usage_log (device_id, user_id, purpose, created_at)
                VALUES ($1, $2, $3, CURRENT_TIMESTAMP)
                RETURNING id;
            )";
            const auto result = co_await executor->execSqlCoro(sql, input.deviceId, input.operatorUserId, input.purpose);
            if (result.empty()) {
                common::error::throwInternalError(
                    common::error::ErrorCode::InternalError,
                    "新建设备使用记录失败：数据库无返回值");
            }
            co_return dto::view::CreatedObjectIdView{
                .id = result[0]["id"].as<std::int64_t>()
            };
        } catch (const drogon::orm::DrogonDbException &e) {
            common::error::throwInternalError(
                common::error::ErrorCode::InternalError,
                std::string("新建设备使用记录失败：") + e.base().what());
        }
    }

    drogon::Task<DeviceUsageLogListPage>
    DeviceRepository::listDeviceUsageLogs(const common::db::SqlExecutorPtr &executor,
                                          const DeviceUsageLogListQuery &query) const {
        try {
            const auto countSql = "SELECT COUNT(*) FROM device_usage_log WHERE device_id = $1";
            const auto countResult = co_await executor->execSqlCoro(countSql, query.deviceId);
            std::int64_t total = 0;
            if (!countResult.empty()) {
                total = countResult[0][0].as<std::int64_t>();
            }

            std::vector<dto::view::DeviceUsageLogItemView> list;
            if (total > 0) {
                const auto dataSql = R"(
                    SELECT
                        l.id, l.device_id, l.user_id, l.purpose,
                        to_char(l.created_at, 'YYYY-MM-DD"T"HH24:MI:SSOF') as created_at_str,
                        u.real_name as operator_name
                    FROM device_usage_log l
                    LEFT JOIN sys_user u ON l.user_id = u.id
                    WHERE l.device_id = $1
                    ORDER BY l.id DESC
                    LIMIT $2 OFFSET $3
                )";
                std::int64_t offset = (query.page - 1) * query.pageSize;
                const auto dataResult = co_await executor->execSqlCoro(dataSql, query.deviceId, query.pageSize, offset);
                for (const auto &row : dataResult) {
                    list.push_back({
                        .id = row["id"].as<std::int64_t>(),
                        .deviceId = row["device_id"].as<std::int64_t>(),
                        .userId = row["user_id"].as<std::int64_t>(),
                        .operatorName = row["operator_name"].isNull() ? "" : row["operator_name"].as<std::string>(),
                        .purpose = row["purpose"].as<std::string>(),
                        .createdAt = row["created_at_str"].as<std::string>()
                    });
                }
            }

            co_return DeviceUsageLogListPage{
                .list = std::move(list),
                .total = total,
                .page = query.page,
                .pageSize = query.pageSize
            };
        } catch (const drogon::orm::DrogonDbException &e) {
            common::error::throwInternalError(
                common::error::ErrorCode::InternalError,
                std::string("获取设备使用记录失败：") + e.base().what());
        }
    }
} // namespace project_tracker::modules::device_log::repository
