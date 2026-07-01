#include <gtest/gtest.h>
#include "common/error/BusinessException.h"
#include "common/error/Throw.h"

namespace project_tracker::common::error {

    TEST(ErrorTest, BusinessException_PropertiesSetCorrectly) {
        BusinessException ex(drogon::k400BadRequest, ErrorCode::InvalidParameter, "提供的参数无效");
        
        EXPECT_EQ(ex.status(), drogon::k400BadRequest);
        EXPECT_EQ(ex.code(), ErrorCode::InvalidParameter);
        EXPECT_EQ(ex.message(), "提供的参数无效");
        EXPECT_STREQ(ex.what(), "提供的参数无效");
    }

    TEST(ErrorTest, ThrowBusinessError_ThrowsExceptionAndSetsFields) {
        try {
            throwBusinessError(drogon::k403Forbidden, ErrorCode::Forbidden, "自定义禁止访问消息");
            FAIL() << "预期应该抛出 BusinessException 异常";
        } catch (const BusinessException& ex) {
            EXPECT_EQ(ex.status(), drogon::k403Forbidden);
            EXPECT_EQ(ex.code(), ErrorCode::Forbidden);
            EXPECT_EQ(ex.message(), "自定义禁止访问消息");
        } catch (...) {
            FAIL() << "预期应该抛出 BusinessException 异常，但捕获到了其他异常";
        }
    }

    TEST(ErrorTest, ThrowBadRequest_ThrowsExceptionAndSetsFields) {
        try {
            throwBadRequest(ErrorCode::InvalidParameter, "错误请求消息");
            FAIL() << "预期应该抛出 BusinessException 异常";
        } catch (const BusinessException& ex) {
            EXPECT_EQ(ex.status(), drogon::k400BadRequest);
            EXPECT_EQ(ex.code(), ErrorCode::InvalidParameter);
            EXPECT_EQ(ex.message(), "错误请求消息");
        }
    }

    TEST(ErrorTest, ThrowUnauthorized_ThrowsExceptionAndSetsFields) {
        try {
            throwUnauthorized(ErrorCode::Unauthorized, "未授权消息");
            FAIL() << "预期应该抛出 BusinessException 异常";
        } catch (const BusinessException& ex) {
            EXPECT_EQ(ex.status(), drogon::k401Unauthorized);
            EXPECT_EQ(ex.code(), ErrorCode::Unauthorized);
            EXPECT_EQ(ex.message(), "未授权消息");
        }
    }

    TEST(ErrorTest, ThrowForbidden_ThrowsExceptionAndSetsFields) {
        try {
            throwForbidden(ErrorCode::Forbidden, "禁止访问消息");
            FAIL() << "预期应该抛出 BusinessException 异常";
        } catch (const BusinessException& ex) {
            EXPECT_EQ(ex.status(), drogon::k403Forbidden);
            EXPECT_EQ(ex.code(), ErrorCode::Forbidden);
            EXPECT_EQ(ex.message(), "禁止访问消息");
        }
    }

    TEST(ErrorTest, ThrowNotFound_ThrowsExceptionAndSetsFields) {
        try {
            throwNotFound(ErrorCode::UserNotFound, "用户不存在消息");
            FAIL() << "预期应该抛出 BusinessException 异常";
        } catch (const BusinessException& ex) {
            EXPECT_EQ(ex.status(), drogon::k404NotFound);
            EXPECT_EQ(ex.code(), ErrorCode::UserNotFound);
            EXPECT_EQ(ex.message(), "用户不存在消息");
        }
    }

    TEST(ErrorTest, ThrowConflict_ThrowsExceptionAndSetsFields) {
        try {
            throwConflict(ErrorCode::UsernameAlreadyExists, "冲突消息");
            FAIL() << "预期应该抛出 BusinessException 异常";
        } catch (const BusinessException& ex) {
            EXPECT_EQ(ex.status(), drogon::k409Conflict);
            EXPECT_EQ(ex.code(), ErrorCode::UsernameAlreadyExists);
            EXPECT_EQ(ex.message(), "冲突消息");
        }
    }

    TEST(ErrorTest, ThrowInternalError_ThrowsExceptionAndSetsFields) {
        try {
            throwInternalError(ErrorCode::InternalError, "内部服务器错误消息");
            FAIL() << "预期应该抛出 BusinessException 异常";
        } catch (const BusinessException& ex) {
            EXPECT_EQ(ex.status(), drogon::k500InternalServerError);
            EXPECT_EQ(ex.code(), ErrorCode::InternalError);
            EXPECT_EQ(ex.message(), "内部服务器错误消息");
        }
    }

} // namespace project_tracker::common::error
