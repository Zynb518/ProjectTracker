#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <drogon/utils/coroutine.h>

#include "modules/project/domain/ProjectEnums.h"
#include "modules/project/dto/view/ProjectListItemView.h"
#include "modules/user/domain/UserEnums.h"

namespace project_tracker::modules::project::repository {
    // 项目列表查询条件
    struct ProjectListQuery {
        std::string keyword;
        std::optional<domain::ProjectStatus> status;
        std::optional<std::int64_t> ownerUserId;
        std::int64_t currentUserId;
        user::domain::SystemRole currentUserRole;
        std::int64_t page = 1;
        std::int64_t pageSize = 10;
    };

    // 项目列表分页结果
    struct ProjectListPage {
        std::vector<dto::view::ProjectListItemView> list;
        std::int64_t total;
        std::int64_t page;
        std::int64_t pageSize;
    };

    class ProjectRepository {
    public:
        // 查询项目分页列表
        drogon::Task<ProjectListPage>
        listProjects(const ProjectListQuery &query) const;
    };
} // namespace project_tracker::modules::project::repository
