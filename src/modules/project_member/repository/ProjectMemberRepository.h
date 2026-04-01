#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <drogon/utils/coroutine.h>

#include "common/db/SqlExecutor.h"
#include "modules/project_member/dto/view/ProjectMemberListItemView.h"
#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project_member::repository {
    // 项目成员列表查询条件
    struct ProjectMemberListQuery {
        std::int64_t projectId;
        std::int64_t currentUserId;
        user::domain::SystemRole currentUserRole;
    };

    // 项目成员列表查询结果
    struct ProjectMemberListResult {
        bool hasPermission;
        std::vector<dto::view::ProjectMemberListItemView> list;
    };

    class ProjectMemberRepository {
    public:
        // 查询项目成员列表
        drogon::Task<std::optional<ProjectMemberListResult>>
        listProjectMembers(const common::db::SqlExecutorPtr &executor,
                           const ProjectMemberListQuery &query) const;
    };
} // namespace project_tracker::modules::project_member::repository
