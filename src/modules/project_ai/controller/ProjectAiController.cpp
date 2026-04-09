#include "modules/project_ai/controller/ProjectAiController.h"

#include <drogon/drogon.h>

#include "common/api/ApiResponse.h"
#include "common/error/BusinessException.h"
#include "common/error/ErrorCode.h"
#include "common/util/JsonUtil.h"

namespace project_tracker::modules::project_ai::controller {
    namespace api = project_tracker::common::api;
    namespace error = project_tracker::common::error;
    namespace util = project_tracker::common::util;

    drogon::Task<drogon::HttpResponsePtr>
    ProjectAiController::generateProjectDraft(drogon::HttpRequestPtr request) {
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

        dto::command::GenerateProjectDraftInput input{};
        if (!util::readRequiredString(*json, "prompt", input.prompt)) {
            co_return api::fail(
                drogon::k400BadRequest,
                error::ErrorCode::InvalidParameter,
                "prompt 必须是非空字符串");
        }

        try {
            const auto generatedDraft = co_await projectAiService_.generateProjectDraft(input);
            co_return api::ok(generatedDraft);
        } catch (const error::BusinessException &exception) {
            co_return api::fromException(exception);
        }
    }
} // namespace project_tracker::modules::project_ai::controller
