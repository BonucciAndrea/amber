#ifndef WSYS_SYS_MMAN_H
#define WSYS_SYS_MMAN_H
#include <sys/types.h>
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_NONE 0
#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_FIXED 0x10
#define MAP_ANON 0x20
#define MAP_ANONYMOUS 0x20
#define MAP_NORESERVE 0x4000
#define MAP_FAILED ((void *)-1)
void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int munmap(void *addr, size_t len);
#endif
