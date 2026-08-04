#ifndef WSYS_SYS_STAT_H
#define WSYS_SYS_STAT_H
#include <sys/types.h>
#define S_IFCHR 0020000
#define S_IFDIR 0040000
#define S_IFREG 0100000
#define S_IFMT  0170000
#define S_ISDIR(m) (((m)&S_IFMT)==S_IFDIR)
#define S_ISREG(m) (((m)&S_IFMT)==S_IFREG)
struct stat {
    long st_ino;
    unsigned int st_mode;
    unsigned int st_nlink;
    off_t st_size;
    long st_blksize;
    long st_blocks;
};
int fstat(int fd, struct stat *buf);
int stat(const char *path, struct stat *buf);
#endif
