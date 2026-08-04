#ifndef WSYS_SYS_TIME_H
#define WSYS_SYS_TIME_H
#include <sys/types.h>
struct timeval { int tv_sec; int tv_usec; };
struct timespec { long tv_sec; long tv_nsec; };
#define CLOCK_MONOTONIC 1
#define CLOCK_REALTIME 0
int gettimeofday(struct timeval *tv, void *tz);
int clock_gettime(int clk, struct timespec *ts);
#endif
