cmake_minimum_required(VERSION 3.25)

project(
    LeetCode_Local
    VERSION 0.2.0
    DESCRIPTION "由静态反射驱动的本地力扣运行器"
    LANGUAGES CXX
)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
   OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17)
    message(FATAL_ERROR
        "本项目要求 GCC 17 或更高版本；当前检测到 "
        "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
    )
endif()

set(LC_SELECTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/current_problem.txt")
if(NOT EXISTS "${LC_SELECTION_FILE}")
    message(FATAL_ERROR "缺少题目选择文件：${LC_SELECTION_FILE}")
endif()

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${LC_SELECTION_FILE}"
)
file(READ "${LC_SELECTION_FILE}" LC_ID)
string(STRIP "${LC_ID}" LC_ID)

# LC_ID 曾经是缓存变量，因此需要清除旧构建目录中的残留值。
# 现在所有 IDE 都只以 current_problem.txt 为题号来源。
unset(LC_ID CACHE)

include(cmake/SelectProblem.cmake)

file(RELATIVE_PATH LC_SOLUTION_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${LC_SOLUTION_FILE}"
)
file(RELATIVE_PATH LC_CASES_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${LC_CASES_FILE}"
)

set(LC_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${LC_GENERATED_DIR}/lc")

configure_file(
    cmake/selected_problem.hpp.in
    "${LC_GENERATED_DIR}/lc/selected_problem.hpp"
    @ONLY
)

add_executable(lc src/main.cpp)

set_target_properties(lc PROPERTIES
    CXX_STANDARD 26
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(lc PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${LC_GENERATED_DIR}"
)

# CMake 当前的 CXX_STANDARD 只支持到 C++26。GCC 17 可以直接接受仍在演进的
# C++29 草案，而静态反射和契约仍需显式启用。
target_compile_options(lc PRIVATE
    -std=c++29
    -freflection
    -fcontracts
    "--embed-dir=${CMAKE_CURRENT_SOURCE_DIR}"
    -Wall
    -Wextra
    -Wpedantic
)

message(STATUS "力扣题目 ${LC_ID}：${LC_PROBLEM_DIR}")
