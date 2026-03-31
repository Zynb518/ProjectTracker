#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <drogon/utils/coroutine.h>

#include "common/db/SqlExecutor.h"
#include "modules/user/dto/command/UpdateUserBasicInfoInput.h"
#include "modules/user/dto/command/UpdateUserStatusInput.h"
#include "modules/user/dto/view/SysUserView.h"

namespace project_tracker::modules::user::repository {
    namespace user_view = modules::user::dto::view;

    // 创建用户写入参数
    struct CreateUserRecord {
        std::string username;
        std::string passwordHash;
        std::string realName;
        int systemRole;
        std::string email;
        std::string phone;
    };

    // 用户列表查询条件
    struct UserListQuery {
        std::string keyword;
        std::optional<int> systemRole;
        std::optional<int> status;
        std::int64_t page = 1;
        std::int64_t pageSize = 10;
    };

    // 用户列表分页结果
    struct UserListPage {
        std::vector<user_view::SysUserView> list;
        std::int64_t total;
        std::int64_t page;
        std::int64_t pageSize;
    };

    class UserRepository {
    public:
        // 创建用户
        drogon::Task<user_view::SysUserView>
        insertUser(const common::db::SqlExecutorPtr &executor,
                   const CreateUserRecord &record) const;

        // 查询单个用户详情
        drogon::Task<std::optional<user_view::SysUserView>>
        findUserById(const common::db::SqlExecutorPtr &executor,
                     std::int64_t userId) const;

        // 修改用户基础信息
        drogon::Task<std::optional<user_view::SysUserView>>
        updateUserBasicInfo(const common::db::SqlExecutorPtr &executor,
                            const dto::command::UpdateUserBasicInfoInput &input) const;

        // 修改用户状态
        drogon::Task<std::optional<user_view::SysUserView>>
        updateUserStatus(const common::db::SqlExecutorPtr &executor,
                         const dto::command::UpdateUserStatusInput &input) const;

        // 查询用户分页列表
        drogon::Task<UserListPage>
        listUsers(const common::db::SqlExecutorPtr &executor,
                  const UserListQuery &query) const;
    };
} // namespace project_tracker::modules::user::repository
