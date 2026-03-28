#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <drogon/utils/coroutine.h>

#include "modules/user/dto/view/SysUserView.h"

namespace project_tracker::modules::user::repository {
    namespace user_view = modules::user::dto::view;

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
        // 查询用户分页列表
        drogon::Task<UserListPage>
        listUsers(const UserListQuery &query) const;
    };
} // namespace project_tracker::modules::user::repository
