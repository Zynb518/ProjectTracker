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
