# 生成后端可执行文件
add_executable(
    Project_Tracker
    bootstrap/main.cpp
    common/error/BusinessException.cpp
    common/error/Throw.cpp
    filters/AdminRequiredFilter.cpp
    filters/LoginRequiredFilter.cpp
    modules/auth/controller/AuthController.cpp
    modules/auth/repository/AuthRepository.cpp
    modules/auth/service/AuthService.cpp
    modules/user/controller/UserController.cpp
    modules/user/repository/UserRepository.cpp
    modules/user/service/UserService.cpp
)

# 把当前 src 目录加入头文件搜索路径
# 这样 src 下的源文件可以直接写：
# #include "common/error/ErrorCode.h"
target_include_directories(
    Project_Tracker
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接 Drogon 和 Argon2
target_link_libraries(
    Project_Tracker
    PRIVATE
        Drogon::Drogon
        PkgConfig::ARGON2
)
