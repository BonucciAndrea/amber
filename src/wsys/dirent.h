#ifndef WSYS_DIRENT_H
#define WSYS_DIRENT_H
typedef struct { int _unused; } DIR;
struct dirent { char d_name[256]; };
DIR *fdopendir(int fd);
DIR *opendir(const char *path);
struct dirent *readdir(DIR *d);
int closedir(DIR *d);
#endif
