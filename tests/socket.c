#include <stdio.h>
#include <stdlib.h>

#include "server.h"

ROUTER(GET, /fib)
{
    static long a = 0;
    static long b = 1;
    char content[100];

    long res = a + b;

    sprintf(content, "%ld\n", res);
    APPEND_RES_BODY(content, strlen(content));
    SET_RES_MIME("text/plain");
    response->status = 200;
    a = b;
    b = res;
    return 0;
}

ROUTER(GET, /simple/y)
{
    char *content =  "Something interesting y...";
    APPEND_RES_BODY(content, strlen(content));
    SET_RES_MIME("text/plain");
    SET_RES_HEADER("Header1", "Value1");
    SET_RES_HEADER("Header2", "Value2");
    SET_RES_HEADER("Header3", "Value3");
    response->status = 200;
    return 0;
}

ROUTER_STATIC(./tests/fib.html, /simple/fib)

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
