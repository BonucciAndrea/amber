#ifndef WSYS_NETINET_IN_H
#define WSYS_NETINET_IN_H
struct in_addr { unsigned int s_addr; };
struct sockaddr_in { unsigned short sin_family; unsigned short sin_port; struct in_addr sin_addr; char sin_zero[8]; };
#endif
