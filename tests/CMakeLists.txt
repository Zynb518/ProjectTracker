add_executable(
    Project_Tracker_ThreadNumConfig_Test
    bootstrap/ThreadNumConfigTest.cpp
)

add_executable(
    Project_Tracker_StartupLogMessage_Test
    bootstrap/StartupLogMessageTest.cpp
)

target_include_directories(
    Project_Tracker_ThreadNumConfig_Test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_include_directories(
    Project_Tracker_StartupLogMessage_Test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(
    Project_Tracker_ThreadNumConfig_Test
    PRIVATE
        Drogon::Drogon
)

target_link_libraries(
    Project_Tracker_StartupLogMessage_Test
    PRIVATE
        Drogon::Drogon
)

add_test(
    NAME Project_Tracker_ThreadNumConfig_Test
    COMMAND Project_Tracker_ThreadNumConfig_Test
)

add_test(
    NAME Project_Tracker_StartupLogMessage_Test
    COMMAND Project_Tracker_StartupLogMessage_Test
)
