#include <gtest/gtest.h>
#include <drogon/HttpRequest.h>
#include "common/util/QueryParamUtil.h"

namespace project_tracker::common::util {

    // Helper 函数：创建一个带指定 query 参数的 mock 请求
    static drogon::HttpRequestPtr createMockRequest(const std::unordered_map<std::string, std::string>& params) {
        auto req = drogon::HttpRequest::newHttpRequest();
        for (const auto& [key, value] : params) {
            req->setParameter(key, value);
        }
        return req;
    }

    // --- readQueryString Tests ---
    TEST(QueryParamUtilTest, ReadQueryString_ParamExists_ReturnsValue) {
        auto req = createMockRequest({{"keyword", "alice"}});
        auto result = readQueryString(req, "keyword");
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(*result, "alice");
    }

    TEST(QueryParamUtilTest, ReadQueryString_ParamMissing_ReturnsNullopt) {
        auto req = createMockRequest({});
        auto result = readQueryString(req, "keyword");
        EXPECT_FALSE(result.has_value());
    }

    // --- readQueryInt Tests ---
    TEST(QueryParamUtilTest, ReadQueryInt_ValidInt_ReturnsTrue) {
        auto req = createMockRequest({{"system_role", "2"}});
        std::optional<int> value;
        EXPECT_TRUE(readQueryInt(req, "system_role", value));
        ASSERT_TRUE(value.has_value());
        EXPECT_EQ(*value, 2);
    }

    TEST(QueryParamUtilTest, ReadQueryInt_ParamMissing_ReturnsTrueAndSetsNullopt) {
        auto req = createMockRequest({});
        std::optional<int> value = 10;
        EXPECT_TRUE(readQueryInt(req, "system_role", value));
        EXPECT_FALSE(value.has_value());
    }

    TEST(QueryParamUtilTest, ReadQueryInt_InvalidInt_ReturnsFalseAndSetsNullopt) {
        auto req = createMockRequest({{"system_role", "not_a_number"}});
        std::optional<int> value = 10;
        EXPECT_FALSE(readQueryInt(req, "system_role", value));
        EXPECT_FALSE(value.has_value());
    }

    // --- readPositiveQueryInt Tests ---
    TEST(QueryParamUtilTest, ReadPositiveQueryInt_PositiveInt_ReturnsTrue) {
        auto req = createMockRequest({{"page", "2"}});
        std::optional<int> value;
        EXPECT_TRUE(readPositiveQueryInt(req, "page", value));
        ASSERT_TRUE(value.has_value());
        EXPECT_EQ(*value, 2);
    }

    TEST(QueryParamUtilTest, ReadPositiveQueryInt_ParamMissing_ReturnsTrueAndSetsNullopt) {
        auto req = createMockRequest({});
        std::optional<int> value = 10;
        EXPECT_TRUE(readPositiveQueryInt(req, "page", value));
        EXPECT_FALSE(value.has_value());
    }

    TEST(QueryParamUtilTest, ReadPositiveQueryInt_ZeroOrNegativeInt_ReturnsFalse) {
        // 测试 0
        {
            auto req = createMockRequest({{"page", "0"}});
            std::optional<int> value = 10;
            EXPECT_FALSE(readPositiveQueryInt(req, "page", value));
        }
        // 测试负数
        {
            auto req = createMockRequest({{"page", "-5"}});
            std::optional<int> value = 10;
            EXPECT_FALSE(readPositiveQueryInt(req, "page", value));
        }
    }

    // --- readPositiveQueryInt64 Tests ---
    TEST(QueryParamUtilTest, ReadPositiveQueryInt64_PositiveInt64_ReturnsTrue) {
        auto req = createMockRequest({{"page_size", "123456789012"}});
        std::optional<std::int64_t> value;
        EXPECT_TRUE(readPositiveQueryInt64(req, "page_size", value));
        ASSERT_TRUE(value.has_value());
        EXPECT_EQ(*value, 123456789012LL);
    }

    TEST(QueryParamUtilTest, ReadPositiveQueryInt64_ParamMissing_ReturnsTrueAndSetsNullopt) {
        auto req = createMockRequest({});
        std::optional<std::int64_t> value = 10LL;
        EXPECT_TRUE(readPositiveQueryInt64(req, "page_size", value));
        EXPECT_FALSE(value.has_value());
    }

    TEST(QueryParamUtilTest, ReadPositiveQueryInt64_ZeroOrNegativeInt64_ReturnsFalse) {
        // 测试 0
        {
            auto req = createMockRequest({{"page_size", "0"}});
            std::optional<std::int64_t> value;
            EXPECT_FALSE(readPositiveQueryInt64(req, "page_size", value));
        }
        // 测试负数
        {
            auto req = createMockRequest({{"page_size", "-100000000000"}});
            std::optional<std::int64_t> value;
            EXPECT_FALSE(readPositiveQueryInt64(req, "page_size", value));
        }
    }

} // namespace project_tracker::common::util
