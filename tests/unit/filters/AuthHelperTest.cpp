#include <gtest/gtest.h>
#include "filters/AuthHelper.h"
#include "modules/user/domain/UserEnums.h"

namespace project_tracker::filters {

    TEST(AuthHelperTest, IsLoginAllowed_Nullopt_ReturnsFalse) {
        std::optional<std::int64_t> userId = std::nullopt;
        EXPECT_FALSE(isLoginAllowed(userId));
    }

    TEST(AuthHelperTest, IsLoginAllowed_ZeroOrNegative_ReturnsFalse) {
        EXPECT_FALSE(isLoginAllowed(0));
        EXPECT_FALSE(isLoginAllowed(-1));
        EXPECT_FALSE(isLoginAllowed(-100));
    }

    TEST(AuthHelperTest, IsLoginAllowed_Positive_ReturnsTrue) {
        EXPECT_TRUE(isLoginAllowed(1));
        EXPECT_TRUE(isLoginAllowed(99999));
    }

    TEST(AuthHelperTest, CheckAdmin_UserIdMissingOrInvalid_ReturnsUnauthorized) {
        EXPECT_EQ(checkAdmin(std::nullopt, modules::user::domain::SystemRole::Admin), AdminCheckResult::Unauthorized);
        EXPECT_EQ(checkAdmin(0, modules::user::domain::SystemRole::Admin), AdminCheckResult::Unauthorized);
        EXPECT_EQ(checkAdmin(-5, modules::user::domain::SystemRole::Admin), AdminCheckResult::Unauthorized);
    }

    TEST(AuthHelperTest, CheckAdmin_SystemRoleMissing_ReturnsUnauthorized) {
        EXPECT_EQ(checkAdmin(1, std::nullopt), AdminCheckResult::Unauthorized);
    }

    TEST(AuthHelperTest, CheckAdmin_NotAdmin_ReturnsForbidden) {
        EXPECT_EQ(checkAdmin(1, modules::user::domain::SystemRole::ProjectManager), AdminCheckResult::Forbidden);
        EXPECT_EQ(checkAdmin(1, modules::user::domain::SystemRole::Employee), AdminCheckResult::Forbidden);
    }

    TEST(AuthHelperTest, CheckAdmin_Admin_ReturnsSuccess) {
        EXPECT_EQ(checkAdmin(1, modules::user::domain::SystemRole::Admin), AdminCheckResult::Success);
    }

} // namespace project_tracker::filters
