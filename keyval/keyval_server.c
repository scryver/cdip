#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h> // hton*
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "keyval_types.h"
#include "keyval_log.h"
#include "keyval_socket.h"

#include "keyval_socket.c"

global b32 gIsRunning = false;

func void closer(i32 sigId)
{
    if (sigId == SIGINT) {
        gIsRunning = false;
    }
}

func void do_something(i32 client)
{
    char readBuf[64];
    ssize_t readCount = read(client, readBuf, sizeof(readBuf) - 1);
    if (readCount >= 0)
    {
        readBuf[readCount] = 0;
        keyval_log("client msg: '%s'", readBuf);

        char writeBuf[] = "Welcome";
        write(client, writeBuf, sizeof(writeBuf) - 1);
    }
    else
    {
        keyval_log("read() error");
    }
}

i32 main(i32 argCount, char **arguments)
{
    i32 result = 0;

    i32 server = socket_server(0, 1234);
    gIsRunning = true;
    signal(SIGINT, closer);

    while (gIsRunning)
    {
        i32 client = socket_accept(server);
        if (client > 0)
        {
            do_something(client);
            close(client);
        }
    }

    return result;
}
