#pragma once

#include <drogon/utils/coroutine.h>

#include "modules/project/dto/ProjectDto.h"
#include "modules/project/repository/ProjectRepository.h"

namespace project_tracker::modules::project::service {
    class ProjectService {
    public:

        // 创建项目
        drogon::Task<dto::view::CreatedProjectView>
        createProject(const dto::command::CreateProjectInput &input) const;

        // 修改项目基础信息
        drogon::Task<dto::view::UpdatedProjectBasicInfoView>
        updateProjectBasicInfo(const dto::command::UpdateProjectBasicInfoInput &input) const;

        // 删除项目
        drogon::Task<std::int64_t>
        deleteProject(const dto::command::DeleteProjectInput &input) const;

        // 手动开始项目
        drogon::Task<dto::view::UpdatedProjectStatusView>
        startProject(const dto::command::ProjectStatusActionInput &input) const;

        // 手动完成项目
        drogon::Task<dto::view::UpdatedProjectStatusView>
        completeProject(const dto::command::ProjectStatusActionInput &input) const;

        // 撤销项目完成
        drogon::Task<dto::view::UpdatedProjectStatusView>
        reopenProject(const dto::command::ProjectStatusActionInput &input) const;

    private:
        repository::ProjectRepository projectRepository_;
    };
} // namespace project_tracker::modules::project::service
