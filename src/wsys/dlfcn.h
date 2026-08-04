#ifndef WSYS_DLFCN_H
#define WSYS_DLFCN_H
#define RTLD_LAZY 1
#define RTLD_NOW 2
void *dlopen(const char *path, int flags);
void *dlsym(void *handle, const char *sym);
int dlclose(void *handle);
char *dlerror(void);
#endif
