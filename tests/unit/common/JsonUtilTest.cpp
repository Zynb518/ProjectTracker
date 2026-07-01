#include <gtest/gtest.h>
#include "common/util/JsonUtil.h"

namespace project_tracker::common::util {

    // --- readRequiredString Tests ---
    TEST(JsonUtilTest, ReadRequiredString_ValidString_ReturnsTrue) {
        Json::Value json;
        json["name"] = "Alice";
        std::string value;
        EXPECT_TRUE(readRequiredString(json, "name", value));
        EXPECT_EQ(value, "Alice");
    }

    TEST(JsonUtilTest, ReadRequiredString_EmptyString_ReturnsFalse) {
        Json::Value json;
        json["name"] = "";
        std::string value;
        EXPECT_FALSE(readRequiredString(json, "name", value));
    }

    TEST(JsonUtilTest, ReadRequiredString_NotAnObject_ReturnsFalse) {
        Json::Value json = "not_an_object";
        std::string value;
        EXPECT_FALSE(readRequiredString(json, "name", value));
    }

    TEST(JsonUtilTest, ReadRequiredString_KeyMissing_ReturnsFalse) {
        Json::Value json;
        json["age"] = 30;
        std::string value;
        EXPECT_FALSE(readRequiredString(json, "name", value));
    }

    TEST(JsonUtilTest, ReadRequiredString_NotAString_ReturnsFalse) {
        Json::Value json;
        json["name"] = 123;
        std::string value;
        EXPECT_FALSE(readRequiredString(json, "name", value));
    }

    // --- readRequiredInt Tests ---
    TEST(JsonUtilTest, ReadRequiredInt_ValidInt_ReturnsTrue) {
        Json::Value json;
        json["age"] = 25;
        int value = 0;
        EXPECT_TRUE(readRequiredInt(json, "age", value));
        EXPECT_EQ(value, 25);
    }

    TEST(JsonUtilTest, ReadRequiredInt_NotAnObject_ReturnsFalse) {
        Json::Value json = 123;
        int value = 0;
        EXPECT_FALSE(readRequiredInt(json, "age", value));
    }

    TEST(JsonUtilTest, ReadRequiredInt_KeyMissing_ReturnsFalse) {
        Json::Value json;
        json["name"] = "Alice";
        int value = 0;
        EXPECT_FALSE(readRequiredInt(json, "age", value));
    }

    TEST(JsonUtilTest, ReadRequiredInt_NotAnInt_ReturnsFalse) {
        Json::Value json;
        json["age"] = "twenty-five";
        int value = 0;
        EXPECT_FALSE(readRequiredInt(json, "age", value));
    }

    // --- readRequiredInt64 Tests ---
    TEST(JsonUtilTest, ReadRequiredInt64_ValidInt64_ReturnsTrue) {
        Json::Value json;
        json["id"] = static_cast<Json::Int64>(9223372036854775807LL);
        std::int64_t value = 0;
        EXPECT_TRUE(readRequiredInt64(json, "id", value));
        EXPECT_EQ(value, 9223372036854775807LL);
    }

    TEST(JsonUtilTest, ReadRequiredInt64_NotAnInt64_ReturnsFalse) {
        Json::Value json;
        json["id"] = "large_number";
        std::int64_t value = 0;
        EXPECT_FALSE(readRequiredInt64(json, "id", value));
    }

    // --- readOptionalString (std::string&) Tests ---
    TEST(JsonUtilTest, ReadOptionalStringStdString_ValidString_ReturnsTrue) {
        Json::Value json;
        json["desc"] = "some description";
        std::string value = "default";
        EXPECT_TRUE(readOptionalString(json, "desc", value));
        EXPECT_EQ(value, "some description");
    }

    TEST(JsonUtilTest, ReadOptionalStringStdString_KeyMissing_ReturnsTrueAndKeepsDefault) {
        Json::Value json;
        std::string value = "default";
        EXPECT_TRUE(readOptionalString(json, "desc", value));
        EXPECT_EQ(value, "default"); // remains unchanged
    }

    TEST(JsonUtilTest, ReadOptionalStringStdString_NotAnObject_ReturnsTrueAndKeepsDefault) {
        Json::Value json = "not_an_object";
        std::string value = "default";
        EXPECT_TRUE(readOptionalString(json, "desc", value));
        EXPECT_EQ(value, "default");
    }

    TEST(JsonUtilTest, ReadOptionalStringStdString_NotAString_ReturnsFalse) {
        Json::Value json;
        json["desc"] = 123;
        std::string value = "default";
        EXPECT_FALSE(readOptionalString(json, "desc", value));
    }

    // --- readOptionalString (std::optional<std::string>&) Tests ---
    TEST(JsonUtilTest, ReadOptionalStringOptional_ValidString_ReturnsTrue) {
        Json::Value json;
        json["desc"] = "some description";
        std::optional<std::string> value;
        EXPECT_TRUE(readOptionalString(json, "desc", value));
        ASSERT_TRUE(value.has_value());
        EXPECT_EQ(*value, "some description");
    }

    TEST(JsonUtilTest, ReadOptionalStringOptional_KeyMissing_ReturnsTrueAndSetsNullopt) {
        Json::Value json;
        std::optional<std::string> value = "has_value";
        EXPECT_TRUE(readOptionalString(json, "desc", value));
        EXPECT_FALSE(value.has_value());
    }

    TEST(JsonUtilTest, ReadOptionalStringOptional_NotAString_ReturnsFalseAndSetsNullopt) {
        Json::Value json;
        json["desc"] = 123;
        std::optional<std::string> value = "has_value";
        EXPECT_FALSE(readOptionalString(json, "desc", value));
        EXPECT_FALSE(value.has_value());
    }

    // --- readOptionalInt Tests ---
    TEST(JsonUtilTest, ReadOptionalInt_ValidInt_ReturnsTrue) {
        Json::Value json;
        json["limit"] = 50;
        std::optional<int> value;
        EXPECT_TRUE(readOptionalInt(json, "limit", value));
        ASSERT_TRUE(value.has_value());
        EXPECT_EQ(*value, 50);
    }

    TEST(JsonUtilTest, ReadOptionalInt_KeyMissing_ReturnsTrueAndSetsNullopt) {
        Json::Value json;
        std::optional<int> value = 10;
        EXPECT_TRUE(readOptionalInt(json, "limit", value));
        EXPECT_FALSE(value.has_value());
    }

    TEST(JsonUtilTest, ReadOptionalInt_NotAnInt_ReturnsFalseAndSetsNullopt) {
        Json::Value json;
        json["limit"] = "fifty";
        std::optional<int> value = 10;
        EXPECT_FALSE(readOptionalInt(json, "limit", value));
        EXPECT_FALSE(value.has_value());
    }

    // --- readOptionalInt64 Tests ---
    TEST(JsonUtilTest, ReadOptionalInt64_ValidInt64_ReturnsTrue) {
        Json::Value json;
        json["id"] = static_cast<Json::Int64>(12345678901234LL);
        std::optional<std::int64_t> value;
        EXPECT_TRUE(readOptionalInt64(json, "id", value));
        ASSERT_TRUE(value.has_value());
        EXPECT_EQ(*value, 12345678901234LL);
    }

    TEST(JsonUtilTest, ReadOptionalInt64_KeyMissing_ReturnsTrueAndSetsNullopt) {
        Json::Value json;
        std::optional<std::int64_t> value = 10LL;
        EXPECT_TRUE(readOptionalInt64(json, "id", value));
        EXPECT_FALSE(value.has_value());
    }

    TEST(JsonUtilTest, ReadOptionalInt64_NotAnInt64_ReturnsFalseAndSetsNullopt) {
        Json::Value json;
        json["id"] = "not_a_number";
        std::optional<std::int64_t> value = 10LL;
        EXPECT_FALSE(readOptionalInt64(json, "id", value));
        EXPECT_FALSE(value.has_value());
    }

} // namespace project_tracker::common::util
