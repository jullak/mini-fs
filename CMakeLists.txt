cmake_minimum_required(VERSION 3.15)
project(mini_fs C)

set(CMAKE_C_STANDARD 11)

add_executable(mini_fs main.c mini_fs.c mjfs_cli.c)
