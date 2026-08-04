#ifndef WSYS_TIME_H
#define WSYS_TIME_H
#include <sys/time.h>
#include <sys/types.h>
struct tm { int tm_sec,tm_min,tm_hour,tm_mday,tm_mon,tm_year,tm_wday,tm_yday,tm_isdst; };
time_t time(time_t *t);
#endif
