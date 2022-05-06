cmake_minimum_required(VERSION 3.0)
project(l4_server_lab)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

include_directories(include)

add_subdirectory(lib)
add_subdirectory(tests)
