cmake_minimum_required(VERSION 3.3)
project(multi_thread_computation)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

set(SOURCE_FILES main.c threadpool.c threadpool.h main.h)
add_executable(multi_thread_computation ${SOURCE_FILES})