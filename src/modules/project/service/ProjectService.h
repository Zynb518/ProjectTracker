#pragma once

#include <drogon/utils/coroutine.h>

#include "modules/project/dto/command/CreateProjectInput.h"
#include "modules/project/dto/command/UpdateProjectBasicInfoInput.h"
#include "modules/project/dto/view/CreatedProjectView.h"
#include "modules/project/dto/view/UpdatedProjectBasicInfoView.h"
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

    private:
        repository::ProjectRepository projectRepository_;
    };
} // namespace project_tracker::modules::project::service
