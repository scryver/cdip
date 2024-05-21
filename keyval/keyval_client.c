#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> // hton*
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "keyval_types.h"
#include "keyval_log.h"
#include "keyval_socket.h"
#include "keyval_proto.h"

#include "keyval_socket.c"

func b32 query(i32 client, char *text)
{
    b32 result = false;
    u32 len = (u32)strlen(text);
    if (len <= KEYVAL_MAX_MSG)
    {
        char writeBuf[4 + KEYVAL_MAX_MSG];
        memcpy(writeBuf, &len, 4);
        memcpy(writeBuf + 4, text, len);
        if (write_all(client, 4 + len, writeBuf))
        {
            char readBuf[4 + KEYVAL_MAX_MSG + 1];
            errno = 0;
            if (read_full(client, 4, readBuf))
            {
                memcpy(&len, readBuf, 4);
                if (len <= KEYVAL_MAX_MSG)
                {
                    if (read_full(client, len, readBuf + 4))
                    {
                        readBuf[4 + len] = 0;
                        keyval_print("server: '%s'", readBuf + 4);
                        result = true;
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
        }
    }
    return result;
}

i32 main(i32 argCount, char **arguments)
{
    i32 result = 0;

    i32 client = socket_client(INADDR_LOOPBACK, 1234);

    if (query(client, "Hello1") &&
        query(client, "Hello 2") &&
        query(client, "Hello  3"))
    {
        keyval_log("All done");
    }

    close(client);

    return result;
}
