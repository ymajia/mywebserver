#ifndef ERR_HANDLER_H
#define ERR_HANDLER_H

#include <stdio.h>
#include <stdlib.h>

static void err_handle(const char *message)
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}

#endif