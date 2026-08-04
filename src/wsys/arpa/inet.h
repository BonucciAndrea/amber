#ifndef WSYS_ARPA_INET_H
#define WSYS_ARPA_INET_H
#include <netinet/in.h>
unsigned int inet_addr(const char *);
char *inet_ntoa(struct in_addr);
#endif
