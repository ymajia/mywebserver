#include <unistd.h>
#include "server/web_server.h"

int main()
{
    WebServer webServer(9807, 8, 60000);
    printf("test...\n");
    fflush(stdout);
    webServer.eventLoop();

    return 0;
}