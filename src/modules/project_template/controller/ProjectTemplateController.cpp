#include "modules/project_template/controller/ProjectTemplateController.h"

#include "common/api/ApiResponse.h"
#include "common/error/BusinessException.h"
#include "common/error/ErrorCode.h"
#include "common/util/QueryParamUtil.h"

namespace project_tracker::modules::project_template::controller {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;
    namespace util = project_tracker::common::util;

    namespace {
        // 构造项目模板列表项 JSON。
        Json::Value buildTemplateJson(const dto::view::ProjectTemplateListItemView &item) {
            Json::Value json(Json::objectValue);
            json["id"] = item.id;
            json["name"] = item.name;
            json["description"] = item.description;
            json["status"] = domain::toInt(item.status);
            json["node_count"] = item.nodeCount;

            return json;
        }
    } // namespace

    drogon::Task<drogon::HttpResponsePtr>
    ProjectTemplateController::listTemplates(drogon::HttpRequestPtr request) {
        try {
            std::optional<domain::ProjectTemplateStatus> status;

            std::optional<int> statusValue;
            if (!util::readQueryInt(request, "status", statusValue)) {
                co_return api::fail(
                    drogon::k400BadRequest,
                    error::ErrorCode::InvalidParameter,
                    "status 必须是整数");
            }
            if (statusValue) {
                if (*statusValue != domain::toInt(domain::ProjectTemplateStatus::Enabled) &&
                    *statusValue != domain::toInt(domain::ProjectTemplateStatus::Disabled)) {
                    co_return api::fail(
                        drogon::k400BadRequest,
                        error::ErrorCode::InvalidParameter,
                        "status 只能是 1 或 2");
                }

                status = static_cast<domain::ProjectTemplateStatus>(*statusValue);
            }

            const auto list = co_await projectTemplateRepository_.listTemplates(status);

            Json::Value data(Json::objectValue);
            data["list"] = Json::Value(Json::arrayValue);
            for (const auto &item : list) {
                data["list"].append(buildTemplateJson(item));
            }

            co_return api::ok(data);
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }
} // namespace project_tracker::modules::project_template::controller
