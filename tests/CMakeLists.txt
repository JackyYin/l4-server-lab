add_executable(coro
    coro.c)
target_link_libraries(coro l4-static)

add_executable(socket
    socket.c)
target_link_libraries(socket l4-static)

