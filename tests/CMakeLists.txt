add_executable(
    Project_Tracker_ThreadNumConfig_Test
    bootstrap/ThreadNumConfigTest.cpp
)

target_include_directories(
    Project_Tracker_ThreadNumConfig_Test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(
    Project_Tracker_ThreadNumConfig_Test
    PRIVATE
        Drogon::Drogon
)

add_test(
    NAME Project_Tracker_ThreadNumConfig_Test
    COMMAND Project_Tracker_ThreadNumConfig_Test
)

add_executable(
    Project_Tracker_RequestLogging_Test
    http/RequestLoggingTest.cpp
    ${PROJECT_SOURCE_DIR}/src/http/RequestLogging.cpp
)

target_include_directories(
    Project_Tracker_RequestLogging_Test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(
    Project_Tracker_RequestLogging_Test
    PRIVATE
        Drogon::Drogon
)

add_test(
    NAME Project_Tracker_RequestLogging_Test
    COMMAND Project_Tracker_RequestLogging_Test
)

add_executable(
    Project_Tracker_HealthController_Test
    http/HealthControllerTest.cpp
    ${PROJECT_SOURCE_DIR}/src/modules/health/controller/HealthController.cpp
    ${PROJECT_SOURCE_DIR}/src/modules/health/service/HealthService.cpp
)

target_include_directories(
    Project_Tracker_HealthController_Test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_compile_definitions(
    Project_Tracker_HealthController_Test
    PRIVATE
        PROJECT_TRACKER_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

target_link_libraries(
    Project_Tracker_HealthController_Test
    PRIVATE
        Drogon::Drogon
)

add_test(
    NAME Project_Tracker_HealthController_Test
    COMMAND Project_Tracker_HealthController_Test
)

add_executable(
    Project_Tracker_HealthService_Test
    http/HealthServiceTest.cpp
    ${PROJECT_SOURCE_DIR}/src/modules/health/service/HealthService.cpp
)

target_include_directories(
    Project_Tracker_HealthService_Test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_compile_definitions(
    Project_Tracker_HealthService_Test
    PRIVATE
        PROJECT_TRACKER_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

target_link_libraries(
    Project_Tracker_HealthService_Test
    PRIVATE
        Drogon::Drogon
)
