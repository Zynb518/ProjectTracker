#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <drogon/utils/coroutine.h>

#include "common/db/SqlExecutor.h"
#include "modules/project/domain/ProjectEnums.h"
#include "modules/project_member/dto/command/AddProjectMemberInput.h"
#include "modules/project_member/dto/view/AddedProjectMemberView.h"
#include "modules/project_member/dto/view/ProjectMemberCandidateView.h"
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

    // 项目成员候选列表查询条件
    struct ProjectMemberCandidateQuery {
        std::int64_t projectId;
        std::int64_t operatorUserId;
        user::domain::SystemRole operatorUserRole;
        std::string keyword;
        std::int64_t page = 1;
        std::int64_t pageSize = 10;
    };

    // 项目成员候选列表分页结果
    struct ProjectMemberCandidatePage {
        std::vector<dto::view::ProjectMemberCandidateView> list;
        std::int64_t total;
        std::int64_t page;
        std::int64_t pageSize;
    };

    // 项目成员候选列表查询结果
    struct ProjectMemberCandidateListResult {
        bool hasPermission;
        bool memberManageAllowed;
        ProjectMemberCandidatePage page;
    };

    // 添加项目成员前的项目级校验信息
    struct AddProjectMemberProjectCheckResult {
        std::int64_t ownerUserId;
        user::domain::SystemRole creatorUserRole;
        project::domain::ProjectStatus status;
    };

    class ProjectMemberRepository {
    public:
        // 查询项目成员列表
        drogon::Task<std::optional<ProjectMemberListResult>>
        listProjectMembers(const common::db::SqlExecutorPtr &executor,
                           const ProjectMemberListQuery &query) const;

        // 查询项目成员候选列表
        drogon::Task<std::optional<ProjectMemberCandidateListResult>>
        listProjectMemberCandidates(const common::db::SqlExecutorPtr &executor,
                                    const ProjectMemberCandidateQuery &query) const;

        // 锁定项目行，供添加项目成员写事务做前置检查
        drogon::Task<std::optional<AddProjectMemberProjectCheckResult>>
        findAddProjectMemberProjectCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            std::int64_t projectId) const;

        // 锁定目标用户行，供添加项目成员写事务做前置检查
        drogon::Task<std::optional<user::domain::UserStatus>>
        findAddProjectMemberTargetUserCheckResultForUpdate(
            const common::db::SqlExecutorPtr &executor,
            std::int64_t userId) const;

        // 原子添加项目成员；返回空表示目标用户已是当前项目成员
        drogon::Task<std::optional<dto::view::AddedProjectMemberView>>
        insertProjectMember(const common::db::SqlExecutorPtr &executor,
                            const dto::command::AddProjectMemberInput &input) const;
    };
} // namespace project_tracker::modules::project_member::repository
