cmake_minimum_required(VERSION 3.10)
project(mini-fs C)

set(CMAKE_C_STANDARD 11)

add_executable(mjfs
        main.c
        mjfs_cli.c)

add_executable(mjfs_client
        client_main.c)

add_subdirectory(
        ${mini-fs_SOURCE_DIR}/components
)

target_include_directories(mjfs PUBLIC
        ${mini-fs_SOURCE_DIR}/components)