#include <gtest/gtest.h>
#include <random>
#include <iostream>
#include "common/util/PasswordUtil.h"

namespace project_tracker::common::util {

    TEST(PasswordUtilTest, HashAndPasswordVerifySuccess) {
        const std::string password = "StrongPassword123!";
        
        auto hashOpt = hashPassword(password);
        ASSERT_TRUE(hashOpt.has_value());
        
        const std::string hash = *hashOpt;
        EXPECT_FALSE(hash.empty());
        EXPECT_EQ(hash.rfind("$argon2id$", 0), 0); // 应该为 Argon2id 编码格式的字符串
        
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

    // --- 新增：边界条件测试 ---
    TEST(PasswordUtilTest, BoundaryAndSpecialPasswords) {
        // 1. 空密码测试
        const std::string emptyPassword = "";
        auto emptyHashOpt = hashPassword(emptyPassword);
        ASSERT_TRUE(emptyHashOpt.has_value());
        EXPECT_EQ(verifyPassword(emptyPassword, *emptyHashOpt), PasswordVerifyResult::Matched);

        // 2. Unicode / 中文 / Emoji 密码测试
        const std::string unicodePassword = "密码123!🌟🔒";
        auto unicodeHashOpt = hashPassword(unicodePassword);
        ASSERT_TRUE(unicodeHashOpt.has_value());
        EXPECT_EQ(verifyPassword(unicodePassword, *unicodeHashOpt), PasswordVerifyResult::Matched);

        // 3. 较长密码测试 (1024 字符)
        const std::string longPassword(1024, 'x');
        auto longHashOpt = hashPassword(longPassword);
        ASSERT_TRUE(longHashOpt.has_value());
        EXPECT_EQ(verifyPassword(longPassword, *longHashOpt), PasswordVerifyResult::Matched);
    }

    // --- 新增：基于随机种子的可复现哈希与校验测试 ---
    // 辅助函数：根据指定的种子和长度生成随机密码
    static std::string generateRandomPassword(std::uint32_t seed, std::size_t length) {
        std::mt19937 gen(seed);
        const std::string chars = 
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789"
            "!@#$%^&*()_+-=[]{}|;':\",./<>?\\";
        
        std::uniform_int_distribution<std::size_t> dist(0, chars.size() - 1);
        std::string password;
        password.reserve(length);
        for (std::size_t i = 0; i < length; ++i) {
            password += chars[dist(gen)];
        }
        return password;
    }

    TEST(PasswordUtilTest, RandomizedPasswordHashAndVerify) {
        std::random_device rd;
        const std::uint32_t seed = rd(); 
        
        // 在标准输出中打印随机种子，若测试失败可在日志中找到该值以用于本地复现
        std::cout << "[信息] PasswordUtilTest 随机数种子: " << seed << std::endl;

        // 限制在 5 次以内，防止因 Argon2 密集计算导致测试运行时间过长
        for (std::size_t i = 1; i <= 5; ++i) {
            std::size_t passwordLength = 8 + (i * 10); // 长度从 18 递增到 58
            std::string randomPassword = generateRandomPassword(seed + i, passwordLength);
            
            auto hashOpt = hashPassword(randomPassword);
            ASSERT_TRUE(hashOpt.has_value()) << "对长度为 " << passwordLength << " 的密码计算哈希失败";
            
            // 验证匹配
            EXPECT_EQ(verifyPassword(randomPassword, *hashOpt), PasswordVerifyResult::Matched);
            
            // 验证不匹配（微调最后一个字符）
            std::string wrongPassword = randomPassword;
            wrongPassword.back() = (wrongPassword.back() == 'a' ? 'b' : 'a');
            EXPECT_EQ(verifyPassword(wrongPassword, *hashOpt), PasswordVerifyResult::Mismatched);
        }
    }

} // namespace project_tracker::common::util
