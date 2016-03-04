cmake_minimum_required(VERSION 3.3)
project(HW3)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

set(SOURCE_FILES main.c main.h)
add_executable(HW3 ${SOURCE_FILES})