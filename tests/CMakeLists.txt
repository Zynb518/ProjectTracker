if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)
# For Windows MSVC support (optional but recommended)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# --- 单元测试 Target ---
add_executable(
    Project_Tracker_UnitTests
    unit/HealthServiceUnitTest.cpp
    unit/common/PasswordUtilTest.cpp
    unit/bootstrap/ThreadNumConfigTest.cpp
    unit/common/JsonUtilTest.cpp
    unit/common/QueryParamUtilTest.cpp
    unit/common/ErrorTest.cpp
    unit/filters/AuthHelperTest.cpp
    unit/filters/RateLimitHelperTest.cpp
    ${PROJECT_SOURCE_DIR}/src/modules/health/service/HealthService.cpp
    ${PROJECT_SOURCE_DIR}/src/common/error/BusinessException.cpp
    ${PROJECT_SOURCE_DIR}/src/common/error/Throw.cpp
    ${PROJECT_SOURCE_DIR}/src/filters/RateLimitHelper.cpp
)

target_include_directories(
    Project_Tracker_UnitTests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(
    Project_Tracker_UnitTests
    PRIVATE
        GTest::gtest_main
        Drogon::Drogon
        PkgConfig::ARGON2
)

# 自动发现 GTest 测试用例
include(GoogleTest)
gtest_discover_tests(Project_Tracker_UnitTests)


# --- 集成测试 Target ---
add_executable(
    Project_Tracker_IntegrationTests
    integration/HealthServiceTest.cpp
    ${PROJECT_SOURCE_DIR}/src/modules/health/service/HealthService.cpp
)

target_include_directories(
    Project_Tracker_IntegrationTests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_compile_definitions(
    Project_Tracker_IntegrationTests
    PRIVATE
        PROJECT_TRACKER_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

target_link_libraries(
    Project_Tracker_IntegrationTests
    PRIVATE
        GTest::gtest_main
        Drogon::Drogon
)

# 自动发现 GTest 测试用例
gtest_discover_tests(Project_Tracker_IntegrationTests)
