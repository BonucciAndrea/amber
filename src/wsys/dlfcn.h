#ifndef WSYS_DLFCN_H
#define WSYS_DLFCN_H
#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_GLOBAL 0x100   /* amber 2.0.0: ext.c's plugin loader passes it; dlopen is a wasm stub, so the value is inert */
#define RTLD_LOCAL 0
#define RTLD_DEFAULT ((void*)0)
void *dlopen(const char *path, int flags);
void *dlsym(void *handle, const char *sym);
int dlclose(void *handle);
char *dlerror(void);
#endif
