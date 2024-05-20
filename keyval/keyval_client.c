#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h> // hton*
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "keyval_types.h"
#include "keyval_log.h"
#include "keyval_socket.h"

#include "keyval_socket.c"

i32 main(i32 argCount, char **arguments)
{
    i32 result = 0;

    i32 client = socket_client(INADDR_LOOPBACK, 1234);
    char msg[] = "Hello";
    write(client, msg, sizeof(msg) - 1);

    char readBuf[64];
    ssize_t readCount = read(client, readBuf, sizeof(readBuf) - 1);
    if (readCount >= 0)
    {
        readBuf[readCount] = 0;
        keyval_log("server msg: '%s'", readBuf);
    }
    else
    {
        keyval_log("read() error");
    }
    close(client);

    return result;
}
