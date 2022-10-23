add_executable(coro
    coro.c)
target_link_libraries(coro l4-static)

add_executable(socket
    socket.c)
target_link_libraries(socket l4-shared)
target_link_libraries(socket ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(socket ${LIBURING_LIB_DIR}/liburing.so)
target_link_libraries(socket ${LIBMAGIC_LIB_DIR}/libmagic.so)
target_link_libraries(socket ${LIBELF_LIB_DIR}/libelf.so)
