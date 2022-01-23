#include <stdio.h>
#include <stdlib.h>

#include "server.h"

int main()
{
    char addr[] = "0.0.0.0";
    int port = 8080;
    /* int fd; */
    /* fd = create_listening_socket((const char*)addr, (uint16_t)port); */

    /* if (fd > 0) { */
    /*     printf("socket successfully created: %d\n", fd); */
    /* } */

    /* printf("%lu\n", sizeof(struct server_info)); */

    struct server_info *server =
        create_server((const char *)addr, (uint16_t)port);

    if (server != NULL) {
        printf("%d\n", server->epoll_fd);
        printf("%d\n", server->listen_fd);

        start_listening(server, 4);
    }
}
