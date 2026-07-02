#include <gtest/gtest.h>
#include "http/ExceptionHandling.h"
#include "common/error/BusinessException.h"
#include <stdexcept>

namespace project_tracker::http {

    TEST(ExceptionHandlingTest, ProcessException_BusinessException_Error_Level) {
        common::error::BusinessException ex(
            drogon::k500InternalServerError,
            common::error::ErrorCode::InternalError,
            "Database connection failed"
        );

        auto result = processException(ex);
        EXPECT_EQ(result.statusCode, drogon::k500InternalServerError);
        EXPECT_EQ(result.errorCode, common::error::ErrorCode::InternalError);
        EXPECT_EQ(result.message, "Database connection failed");
        EXPECT_TRUE(result.logAsError);
    }

    TEST(ExceptionHandlingTest, ProcessException_BusinessException_Warn_Level) {
        common::error::BusinessException ex(
            drogon::k400BadRequest,
            common::error::ErrorCode::InvalidParameter,
            "Invalid argument format"
        );

        auto result = processException(ex);
        EXPECT_EQ(result.statusCode, drogon::k400BadRequest);
        EXPECT_EQ(result.errorCode, common::error::ErrorCode::InvalidParameter);
        EXPECT_EQ(result.message, "Invalid argument format");
        EXPECT_FALSE(result.logAsError);
    }

    TEST(ExceptionHandlingTest, ProcessException_StandardException_GenericError) {
        std::runtime_error ex("Disk write failure");

        auto result = processException(ex);
        EXPECT_EQ(result.statusCode, drogon::k500InternalServerError);
        EXPECT_EQ(result.errorCode, common::error::ErrorCode::InternalError);
        EXPECT_EQ(result.message, "服务器内部错误");
        EXPECT_TRUE(result.logAsError);
    }

} // namespace project_tracker::http
