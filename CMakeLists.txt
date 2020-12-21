cmake_minimum_required(VERSION 3.13)
project(non_blocking_server C)

set(CMAKE_C_STANDARD 99)

add_executable(non_blocking_server main.c)
add_executable(simple_non_blocking_server simple.c)