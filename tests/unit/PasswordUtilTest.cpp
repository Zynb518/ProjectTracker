#include <gtest/gtest.h>
#include "common/util/PasswordUtil.h"

namespace project_tracker::common::util {

    TEST(PasswordUtilTest, HashAndPasswordVerifySuccess) {
        const std::string password = "StrongPassword123!";
        
        auto hashOpt = hashPassword(password);
        ASSERT_TRUE(hashOpt.has_value());
        
        const std::string hash = *hashOpt;
        EXPECT_FALSE(hash.empty());
        EXPECT_EQ(hash.rfind("$argon2id$", 0), 0); // Should be Argon2id encoded string
        
        auto verifyResult = verifyPassword(password, hash);
        EXPECT_EQ(verifyResult, PasswordVerifyResult::Matched);
    }

    TEST(PasswordUtilTest, VerifyPasswordMismatched) {
        const std::string password = "StrongPassword123!";
        const std::string wrongPassword = "WrongPassword123!";
        
        auto hashOpt = hashPassword(password);
        ASSERT_TRUE(hashOpt.has_value());
        
        auto verifyResult = verifyPassword(wrongPassword, *hashOpt);
        EXPECT_EQ(verifyResult, PasswordVerifyResult::Mismatched);
    }

    TEST(PasswordUtilTest, VerifyInvalidHashFormat) {
        const std::string password = "StrongPassword123!";
        const std::string invalidHash = "invalid_hash_string";
        
        auto verifyResult = verifyPassword(password, invalidHash);
        EXPECT_EQ(verifyResult, PasswordVerifyResult::Failed);
    }

} // namespace project_tracker::common::util
