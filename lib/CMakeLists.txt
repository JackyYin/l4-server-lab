find_package(Threads)
add_library(l4-shared SHARED
    coro.c
    kv.c
    hashtable.c
    strbuf.c
    socket.c
    http_parser.c
    epoll_handler.c
    io_uring_handler.c
    server.c)
target_link_libraries(l4-shared ${CMAKE_THREAD_LIBS_INIT} )
target_link_libraries(l4-shared uring)
add_library(l4-static STATIC
    coro.c
    kv.c
    hashtable.c
    strbuf.c
    socket.c
    http_parser.c
    epoll_handler.c
    io_uring_handler.c
    server.c)
target_link_libraries(l4-static ${CMAKE_THREAD_LIBS_INIT} )
target_link_libraries(l4-static uring)

