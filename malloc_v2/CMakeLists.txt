cmake_minimum_required(VERSION 3.3)
project(Malloc)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

set(SOURCE_FILES malloc.c malloc.h t-test1.c)
add_executable(Malloc ${SOURCE_FILES})

target_link_libraries(Malloc pthread)