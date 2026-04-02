#include "modules/project_member/service/ProjectMemberService.h"

#include <drogon/drogon.h>
#include <drogon/orm/Exception.h>

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"

namespace project_tracker::modules::project_member::service {
    namespace error = project_tracker::common::error;
    namespace project_domain = modules::project::domain;
    namespace user_domain = modules::user::domain;

    drogon::Task<dto::view::AddedProjectMemberView>
    ProjectMemberService::addProjectMember(const dto::command::AddProjectMemberInput &input) const {
        try {
            const auto dbClient = drogon::app().getDbClient();
            auto transaction = co_await dbClient->newTransactionCoro();

            // 先锁项目，再锁目标用户，最后做原子插入。
            // 这样“项目仍允许成员管理”“目标用户仍为启用状态”这些前置条件
            // 在本次写事务里不会被并发事务中途改掉。
            const auto projectCheckResult = co_await
                projectMemberRepository_.findAddProjectMemberProjectCheckResultForUpdate(
                    transaction,
                    input.projectId);
            if (!projectCheckResult) {
                error::throwNotFound(
                    error::ErrorCode::ProjectNotFound,
                    "项目不存在");
            }

            const auto targetUserStatus = co_await
                projectMemberRepository_.findAddProjectMemberTargetUserCheckResultForUpdate(
                    transaction,
                    input.userId);
            if (!targetUserStatus) {
                error::throwNotFound(
                    error::ErrorCode::UserNotFound,
                    "目标用户不存在");
            }

            const bool isAdmin = input.operatorUserRole == user_domain::SystemRole::Admin;
            if (!isAdmin &&
                (input.operatorUserRole != user_domain::SystemRole::ProjectManager ||
                 projectCheckResult->ownerUserId != input.operatorUserId)) {
                error::throwForbidden(
                    error::ErrorCode::Forbidden,
                    "当前操作者不是管理员且不是项目经理身份的项目负责人");
            }

            if (projectCheckResult->creatorUserRole == user_domain::SystemRole::Employee) {
                error::throwConflict(
                    error::ErrorCode::PersonalProjectMemberManageForbidden,
                    "普通员工创建的个人自用项目不允许添加成员");
            }

            if (projectCheckResult->status == project_domain::ProjectStatus::Completed) {
                error::throwConflict(
                    error::ErrorCode::ProjectCompletedReadonly,
                    "项目已完成，不允许添加成员");
            }

            if (*targetUserStatus != user_domain::UserStatus::Enabled) {
                error::throwConflict(
                    error::ErrorCode::UserDisabled,
                    "目标用户已被禁用");
            }

            const auto addedMember = co_await projectMemberRepository_.insertProjectMember(
                transaction,
                input);

            if (!addedMember) {
                error::throwConflict(
                    error::ErrorCode::ProjectMemberAlreadyExists,
                    "目标用户已是当前项目成员");
            }

            co_return *addedMember;
        } catch (const drogon::orm::DrogonDbException &) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "数据库事务创建失败");
        }
    }
} // namespace project_tracker::modules::project_member::service
