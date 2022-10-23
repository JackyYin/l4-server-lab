cmake_minimum_required(VERSION 3.0)
project(l4_server_lab)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g -DWATCH_STATIC_FILES")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

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
	UPDATE_COMMAND  ""
)

ExternalProject_Add(libmagic
	PREFIX file-5.42
	URL ftp://ftp.astron.com/pub/file/file-5.42.tar.gz
	CONFIGURE_COMMAND ./configure
		--enable-static=yes
		--enable-shared=yes
		--prefix=${CMAKE_CURRENT_BINARY_DIR}
	BUILD_COMMAND make all
	BUILD_IN_SOURCE TRUE
	INSTALL_COMMAND make install
	UPDATE_COMMAND  ""
)

ExternalProject_Add(elfutils
	PREFIX elfutils
	URL https://sourceware.org/elfutils/ftp/elfutils-latest.tar.bz2
	CONFIGURE_COMMAND ./configure
		--disable-debuginfod
		--disable-libdebuginfod
		--prefix=${CMAKE_CURRENT_BINARY_DIR}
	BUILD_COMMAND make all
	BUILD_IN_SOURCE TRUE
	INSTALL_COMMAND make install
	UPDATE_COMMAND  ""
)

set(LIBURING_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
set(LIBURING_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(LIBMAGIC_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
set(LIBMAGIC_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib)
set(LIBELF_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
set(LIBELF_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib)

include_directories(include)

add_subdirectory(lib)
add_subdirectory(tests)
