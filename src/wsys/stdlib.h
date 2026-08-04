/* minimal freestanding stdlib.h. malloc/calloc/realloc/free/getenv are
 * defined in src/wasmlibc.c (compiled only for -Dwasm); exit() is already
 * defined in src/0.c's wasm branch (routes to the js_exit import). */
#ifndef WSYS_STDLIB_H
#define WSYS_STDLIB_H
#include <stddef.h>
void *malloc(size_t);
void *calloc(size_t, size_t);
void *realloc(void *, size_t);
void free(void *);
char *getenv(const char *);
void exit(int);
double strtod(const char *restrict, char **restrict);
long long strtoll(const char *restrict, char **restrict, int base);
int posix_memalign(void **memptr, size_t alignment, size_t size);
#endif
