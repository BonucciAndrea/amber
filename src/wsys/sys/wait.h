#ifndef WSYS_SYS_WAIT_H
#define WSYS_SYS_WAIT_H
#include <sys/types.h>
struct rusage { long ru_utime_sec, ru_utime_usec, ru_stime_sec, ru_stime_usec; };
int wait4(int pid, int *status, int options, struct rusage *ru);
#define WNOHANG 1
#endif
