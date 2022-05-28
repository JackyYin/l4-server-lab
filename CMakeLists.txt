cmake_minimum_required(VERSION 3.0)
project(l4_server_lab)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g")
set(CMAKE_C_FLAGS_RELEASE "-O3")

message(WARNING "${CMAKE_CURRENT_BINARY_DIR}")
include(ExternalProject)
ExternalProject_Add(liburing
    PREFIX liburing
    GIT_REPOSITORY https://github.com/axboe/liburing.git
    GIT_TAG ecf66cc2a49c69e9c9d224ef54b95082844a7b7a
    CONFIGURE_COMMAND ./configure
        --prefix=${CMAKE_CURRENT_BINARY_DIR}
    BUILD_COMMAND make all
    BUILD_IN_SOURCE TRUE
    INSTALL_COMMAND make install
)

set(LIBURING_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
set(LIBURING_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib)

include_directories(include)

add_subdirectory(lib)
add_subdirectory(tests)
