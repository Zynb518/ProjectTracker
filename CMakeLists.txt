cmake_minimum_required(VERSION 3.28)

# 定义项目名称
project(Project_Tracker)

# 使用 C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 Drogon
find_package(Drogon CONFIG REQUIRED)

# 通过 pkg-config 查找 Argon2，后续登录/密码模块会用到
find_package(PkgConfig REQUIRED)
pkg_check_modules(ARGON2 REQUIRED IMPORTED_TARGET libargon2)

# 进入 src 子目录
add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
