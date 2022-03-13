#include <stdio.h>
#include <stdlib.h>

#include "server.h"

int main(int argc, char **argv)
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
        printf("listen fd: %d\n", server->listen_fd);

        int additional_thrs = 0;

        if (argc > 1)
            additional_thrs = atoi(argv[1]);

        start_listening(server, additional_thrs);
    }
}
