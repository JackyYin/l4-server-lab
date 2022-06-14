include_directories(${LIBURING_INCLUDE_DIR})
link_directories(${LIBURING_LIB_DIR})

find_package(Threads)
add_library(l4-shared SHARED
    coro.c
    kv.c
    murmurhash3.c
    hashtable.c
    strbuf.c
    socket.c
    http_parser.c
    io_loop.c
    server.c)
target_link_libraries(l4-shared ${CMAKE_THREAD_LIBS_INIT} uring)
add_library(l4-static STATIC
    coro.c
    kv.c
    murmurhash3.c
    hashtable.c
    strbuf.c
    socket.c
    http_parser.c
    io_loop.c
    server.c)
target_link_libraries(l4-static ${CMAKE_THREAD_LIBS_INIT} uring)
