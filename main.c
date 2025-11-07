#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include "server.h"

int main(int argc, char **argv)
{
        // dbg
        signal(SIGPIPE, SIG_IGN);
        sock server;
        init_server(&server, inet_addr("127.0.0.1"), DEFAULT_PORT);
        run_server(&server);
               
        return 0;
}