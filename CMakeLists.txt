cmake_minimum_required(VERSION 3.28)

project(LeetCode_Local LANGUAGES CXX)

add_executable(lc_probe
    src/main.cpp
)

target_compile_options(lc_probe PRIVATE
    -std=gnu++29
    -freflection
    -Wall
    -Wextra
    -Wpedantic
)
