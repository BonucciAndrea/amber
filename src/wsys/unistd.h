#ifndef WSYS_UNISTD_H
#define WSYS_UNISTD_H
#include <sys/types.h>
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#define _SC_PAGESIZE 30
#define _SC_NPROCESSORS_ONLN 84
int open(const char *path, int flags, ...);
int close(int fd);
int read(int fd, void *buf, size_t n);
int write(int fd, const void *buf, size_t n);
off_t lseek(int fd, off_t off, int whence);
long sysconf(int name);
char *getcwd(char *buf, size_t n);
int chdir(const char *path);
int dup(int fd);
int dup2(int a, int b);
void _exit(int code);
int execve(const char *path, char *const argv[], char *const envp[]);
int fork(void);
int pipe(int fdv[2]);
int ftruncate(int fd, off_t len);
void _exit(int code);
extern char **environ;
#endif
