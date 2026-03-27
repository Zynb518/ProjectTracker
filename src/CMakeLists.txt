# 生成后端可执行文件
add_executable(
    Project_Tracker
    bootstrap/main.cc
)

# 链接 Drogon 和 Argon2
target_link_libraries(
    Project_Tracker
    PRIVATE
        Drogon::Drogon
        PkgConfig::ARGON2
)
