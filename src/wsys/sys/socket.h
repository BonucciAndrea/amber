#ifndef WSYS_SYS_SOCKET_H
#define WSYS_SYS_SOCKET_H
#include <sys/types.h>
struct sockaddr { unsigned short sa_family; char sa_data[14]; };
#define SOCK_STREAM 1
#define AF_INET 2
#define SOL_SOCKET 1
#define SO_REUSEADDR 2
#define IPPROTO_TCP 6
int socket(int, int, int);
int setsockopt(int, int, int, const void *, socklen_t);
int connect(int, const struct sockaddr *, socklen_t);
int bind(int, const struct sockaddr *, socklen_t);
int listen(int, int);
int accept(int, struct sockaddr *, socklen_t *);
long send(int, const void *, size_t, int);
long recv(int, void *, size_t, int);
#endif
