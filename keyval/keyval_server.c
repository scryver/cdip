#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h> // hton*
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "keyval_types.h"
#include "keyval_log.h"
#include "keyval_socket.h"
#include "keyval_proto.h"

#include "keyval_socket.c"

global b32 gIsRunning = false;

func void closer(i32 sigId)
{
    if (sigId == SIGINT) {
        gIsRunning = false;
    }
}

func b32 handle_request(i32 client)
{
    char readBuf[4 + KEYVAL_MAX_MSG + 1];
    errno = 0;

    b32 result = false;
    if (read_full(client, 4, readBuf))
    {
        u32 len = 0;
        memcpy(&len, readBuf, 4);
        if (len <= KEYVAL_MAX_MSG)
        {
            if (read_full(client, len, readBuf + 4))
            {
                readBuf[4 + len] = 0;
                keyval_print("client: '%s'", readBuf + 4);

                char reply[] = "okay";
                char writeBuf[4 + sizeof(reply) - 1];
                len = (u32)strlen(reply);
                memcpy(writeBuf, &len, 4);
                memcpy(writeBuf + 4, reply, len);
                result = write_all(client, 4 + len, writeBuf);
            }
            else
            {
                keyval_log("read() error");
            }
        }
        else
        {
            keyval_log("message too long");
        }
    }
    else
    {
        if (errno == 0) {
            keyval_log("EOF");
        } else {
            keyval_log("read() error");
        }
    }
    return result;
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
            while (true) {
                b32 success = handle_request(client);
                if (!success) {
                    break;
                }
            }
            close(client);
        }
    }

    return result;
}
