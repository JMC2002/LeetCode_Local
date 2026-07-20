cmake_minimum_required(VERSION 3.25)

project(
    LeetCode_Local
    VERSION 0.1.0
    DESCRIPTION "Local LeetCode workspace built with GCC 17 and C++29 in WSL"
    LANGUAGES CXX
)

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
   OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 17)
    message(FATAL_ERROR
        "This preset requires GCC 17 or newer; found "
        "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}"
    )
endif()

add_executable(lc_probe
    src/main.cpp
    "problems/1. 两数之和/solution.cpp"
    include/lc/header.h
)

target_include_directories(lc_probe PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# CMake 4.4 still models CXX_STANDARD only through C++26. GCC 17 already
# accepts -std=c++29, so request the current C++29 working draft directly.
target_compile_options(lc_probe PRIVATE
    -std=c++29
    -freflection
)

