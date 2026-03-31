#include "modules/project/service/ProjectService.h"

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::project::service {
    namespace error = project_tracker::common::error;

    drogon::Task<dto::view::CreatedProjectView>
    ProjectService::createProject(const dto::command::CreateProjectInput &input) const {
        if (input.plannedStartDate > input.plannedEndDate) {
            error::throwConflict(
                error::ErrorCode::InvalidDateRange,
                "planned_start_date 不能晚于 planned_end_date");
        }

        co_return co_await projectRepository_.createProject(input);
    }

    drogon::Task<dto::view::UpdatedProjectBasicInfoView>
    ProjectService::updateProjectBasicInfo(
        const dto::command::UpdateProjectBasicInfoInput &input) const {
        if (!input.name && !input.description &&
            !input.plannedStartDate && !input.plannedEndDate) {
            error::throwBadRequest(
                error::ErrorCode::InvalidParameter,
                "至少需要提供一个可修改字段");
        }

        if (input.plannedStartDate && input.plannedEndDate &&
            *input.plannedStartDate > *input.plannedEndDate) {
            error::throwConflict(
                error::ErrorCode::InvalidDateRange,
                "planned_start_date 不能晚于 planned_end_date");
        }

        const auto project = co_await projectRepository_.updateProjectBasicInfo(input);
        if (!project) {

            // 当前空结果统一包含三种情况：
            // 1. 项目不存在
            // 2. 项目已完成，被更新 SQL 的 status 条件拦截
            // 3. 当前操作者不是管理员且不是项目负责人

            error::throwNotFound(
                error::ErrorCode::ProjectNotFound,
                "项目不存在/项目已完成/当前操作者不是管理员且不是项目负责人");
        }

        co_return *project;
    }
} // namespace project_tracker::modules::project::service
