#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include <stdint.h>

#define DEFAULT_PORT            8888
#define DEFAULT_ADDR            INADDR_LOOPBACK // localhost
#define DEFAULT_ROOT_PATH       "./root"
#define DEFAULT_INDEX           "index.html"
#define RECV_IDLE_TIMEOUT_DELAY 60 // seconds

typedef struct
{
        char method [8];
        char path   [512];
        char version[32];
} http_header_info_t;

typedef struct
{
        struct sockaddr_in info;
        int fd;
} sock;

typedef struct
{
        sock  *socket;
        char   buf[4096];
        int    flags;
} sock_data;

void init_server(sock *server, uint32_t address, uint16_t port);
void serve      (sock_data *client_data);
int  run_server (sock *server);


#endif // SERVER_H