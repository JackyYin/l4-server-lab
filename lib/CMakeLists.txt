include_directories(${LIBURING_INCLUDE_DIR})
include_directories(${LIBMAGIC_INCLUDE_DIR})
link_directories(${LIBURING_LIB_DIR} ${LIBMAGIC_LIB_DIR})

find_package(Threads)

file(GLOB_RECURSE L4_SOURCE_FILES
	${CMAKE_CURRENT_SOURCE_DIR}/*.c)

add_library(l4-shared SHARED ${L4_SOURCE_FILES})

add_library(l4-static STATIC ${L4_SOURCE_FILES})
target_link_libraries(l4-static ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(l4-static ${LIBURING_LIB_DIR}/liburing.a)
target_link_libraries(l4-static ${LIBMAGIC_LIB_DIR}/libmagic.so)
