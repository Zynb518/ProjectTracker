#pragma once

#include <drogon/utils/coroutine.h>

#include "modules/project_member/dto/command/AddProjectMemberInput.h"
#include "modules/project_member/dto/command/RemoveProjectMemberInput.h"
#include "modules/project_member/dto/view/AddedProjectMemberView.h"
#include "modules/project_member/dto/view/RemovedProjectMemberView.h"
#include "modules/project_member/repository/ProjectMemberRepository.h"

namespace project_tracker::modules::project_member::service {
    class ProjectMemberService {
    public:
        // 添加项目成员
        drogon::Task<dto::view::AddedProjectMemberView>
        addProjectMember(const dto::command::AddProjectMemberInput &input) const;

        // 移除项目成员
        drogon::Task<dto::view::RemovedProjectMemberView>
        removeProjectMember(const dto::command::RemoveProjectMemberInput &input) const;

    private:
        repository::ProjectMemberRepository projectMemberRepository_;
    };
} // namespace project_tracker::modules::project_member::service
