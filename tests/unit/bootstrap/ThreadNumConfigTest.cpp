#include <gtest/gtest.h>
#include "bootstrap/ThreadNumConfig.h"

namespace project_tracker::bootstrap {

    TEST(ThreadNumConfigTest, ReadConfiguredThreadNum_NullConfig_ReturnsNullopt) {
        Json::Value config = Json::Value::null;
        auto result = readConfiguredThreadNum(config);
        EXPECT_FALSE(result.has_value());
    }

    TEST(ThreadNumConfigTest, ReadConfiguredThreadNum_NotAnObject_ThrowsException) {
        Json::Value config = "not_an_object";
        EXPECT_THROW(readConfiguredThreadNum(config), std::runtime_error);
    }

    TEST(ThreadNumConfigTest, ReadConfiguredThreadNum_ThreadNumNull_ReturnsNullopt) {
        Json::Value config;
        config["some_other_key"] = 123;
        auto result = readConfiguredThreadNum(config);
        EXPECT_FALSE(result.has_value());
    }

    TEST(ThreadNumConfigTest, ReadConfiguredThreadNum_ThreadNumNotInt64_ThrowsException) {
        Json::Value config;
        config["threads_num"] = "four";
        EXPECT_THROW(readConfiguredThreadNum(config), std::runtime_error);
    }

    TEST(ThreadNumConfigTest, ReadConfiguredThreadNum_ThreadNumNegative_ThrowsException) {
        Json::Value config;
        config["threads_num"] = -5;
        EXPECT_THROW(readConfiguredThreadNum(config), std::runtime_error);
    }

    TEST(ThreadNumConfigTest, ReadConfiguredThreadNum_ValidThreadNum_ReturnsValue) {
        Json::Value config;
        config["threads_num"] = 4;
        auto result = readConfiguredThreadNum(config);
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, 4);
    }

} // namespace project_tracker::bootstrap
