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
    modules/project/controller/ProjectController.cpp
    modules/project/repository/ProjectRepository.cpp
    modules/project/service/ProjectService.cpp
    modules/project_node/controller/ProjectNodeController.cpp
    modules/project_node/repository/ProjectNodeRepository.cpp
    modules/project_node/service/ProjectNodeService.cpp
    modules/project_member/controller/ProjectMemberController.cpp
    modules/project_member/repository/ProjectMemberRepository.cpp
    modules/project_member/service/ProjectMemberService.cpp
    modules/project_template/controller/ProjectTemplateController.cpp
    modules/project_template/repository/ProjectTemplateRepository.cpp
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
