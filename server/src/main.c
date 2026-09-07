#include "epoll_server.h"

int main(int argc, char *argv[]) {
    const char *ip = "0.0.0.0";
    int port = 8888;

    if (argc >= 2) ip = argv[1];
    if (argc >= 3) port = atoi(argv[2]);

    printf("VideoServer starting...\n");
    printf("  IP: %s\n", ip);
    printf("  Port: %d\n", port);
    printf("  Epoll + Dynamic ThreadPool\n");
    printf("  JWT Dual Token Auth\n");
    printf("  4KB Chunked Upload with Resume\n");
    printf("  Collaborative Filtering Recommend\n");

    server_run(ip, port);

    return 0;
}
