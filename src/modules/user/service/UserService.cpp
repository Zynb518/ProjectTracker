#include "modules/user/service/UserService.h"

#include "common/error/ErrorCode.h"
#include "common/error/Throw.h"
#include "common/util/PasswordUtil.h"

namespace project_tracker::modules::user::service {
    namespace error = project_tracker::common::error;
    namespace util = project_tracker::common::util;

    drogon::Task<dto::view::SysUserView>
    UserService::createUser(const CreateUserCommand &command) const {
        // 创建用户前先完成密码哈希和默认值处理。
        const auto passwordHash = util::hashPassword(command.password);
        if (!passwordHash) {
            error::throwInternalError(
                error::ErrorCode::InternalError,
                "密码哈希生成失败");
        }

        repository::CreateUserRecord record{
            .username = command.username,
            .passwordHash = *passwordHash,
            .realName = command.realName,
            .systemRole = command.systemRole.value_or(domain::toInt(domain::SystemRole::Employee)),
            .email = command.email,
            .phone = command.phone
        };

        co_return co_await userRepository_.createUser(record);
    }
} // namespace project_tracker::modules::user::service
