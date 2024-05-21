
func i32 socket_open(void)
{
    i32 result = socket(AF_INET, SOCK_STREAM, 0);
    if (result >= 0)
    {
        i32 reuse = 1;
        if (setsockopt(result, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
            keyval_error("Could not set REUSEADDR socket option");
        }
        i32 nodly = 1;
        if (setsockopt(result, IPPROTO_TCP, TCP_NODELAY, &nodly, sizeof(nodly)) < 0) {
            keyval_error("Could not set TCP NODELAY socket option");
        }
    }
    return result;
}

func b32 socket_bind(i32 socket, u32 address, u16 port)
{
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(address);
    b32 result = (bind(socket, (struct sockaddr *)&addr, sizeof(addr)) >= 0);
    return result;
}

func i32 socket_server(u32 address, u16 port)
{
    i32 sock = socket_open();
    if (sock < 0) {
        keyval_fatal("Could not open socket");
    }
    if (!socket_bind(sock, address, port)) {
        keyval_fatal("Could not bind socket to %d.%d.%d.%d:%d",
                     (address >> 24) & 0xFF, (address >> 16) & 0xFF,
                     (address >>  8) & 0xFF, (address >>  0) & 0xFF, port);
    }
    if (listen(sock, KEYVAL_SOCKET_LISTEN_MAX) < 0) {
        keyval_fatal("Could not start listening on socket");
    }

    return sock;
}

func i32 socket_accept(i32 server)
{
    struct sockaddr_in client = {0};
    socklen_t addrLen = sizeof(client);
    i32 result = accept(server, (struct sockaddr *)&client, &addrLen);
    return result;
}

func b32 socket_connect(i32 socket, u32 address, u16 port)
{
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(address);
    b32 result = (connect(socket, (struct sockaddr *)&addr, sizeof(addr)) >= 0);
    return result;
}

func i32 socket_client(u32 address, u16 port)
{
    i32 sock = socket_open();
    if (sock < 0) {
        keyval_fatal("Could not open socket");
    }
    if (!socket_connect(sock, address, port)) {
        keyval_fatal("Could not connect socket to %d.%d.%d.%d:%d",
                     (address >> 24) & 0xFF, (address >> 16) & 0xFF,
                     (address >>  8) & 0xFF, (address >>  0) & 0xFF, port);
    }
    return sock;
}

func b32 read_full(i32 sock, sze count, char *buffer)
{
    b32 result = true;
    while (count > 0)
    {
        ssize_t readCount = read(sock, buffer, (size_t)count);
        if (readCount <= 0) {
            result = false;
            break;
        }
        count -= readCount;
        buffer += readCount;
    }
    return result;
}

func b32 write_all(i32 sock, sze count, char *buffer)
{
    b32 result = true;
    while (count > 0)
    {
        ssize_t writeCount = write(sock, buffer, (size_t)count);
        if (writeCount <= 0) {
            result = false;
            break;
        }
        count -= writeCount;
        buffer += writeCount;
    }
    return result;
}
