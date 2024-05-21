#ifndef KEYVAL_SOCKET_H
#define KEYVAL_SOCKET_H

#ifndef KEYVAL_SOCKET_LISTEN_MAX
#define KEYVAL_SOCKET_LISTEN_MAX  128
#endif

func i32 socket_server(u32 address, u16 port);
func i32 socket_accept(i32 server);

func i32 socket_client(u32 address, u16 port);

func b32 read_full(i32 sock, sze count, char *buffer);
func b32 write_all(i32 sock, sze count, char *buffer);

#endif //KEYVAL_SOCKET_H
