/* minimal freestanding string.h -- declarations only. The actual
 * definitions live in src/0.c's `#if defined(wasm)` branch (memcpy,
 * memmove, memset, memchr, memmem, memcmp, strlen, strchr, strstr, strcmp)
 * -- this header just lets every other translation unit that includes
 * <string.h> see matching prototypes. */
#ifndef WSYS_STRING_H
#define WSYS_STRING_H
#include <stddef.h>
void *memcpy(void *restrict, const void *restrict, size_t);
void *memmove(void *, const void *, size_t);
void *memset(void *, int, size_t);
void *memchr(const void *, int, size_t);
void *memmem(const void *, size_t, const void *, size_t);
int memcmp(const void *, const void *, size_t);
size_t strlen(const char *);
char *strchr(const char *, int);
char *strchrnul(const char *, int);
char *strstr(const char *, const char *);
int strcmp(const char *, const char *);
int strncmp(const char *, const char *, size_t);
char *strcpy(char *restrict, const char *restrict);
char *strncpy(char *restrict, const char *restrict, size_t);
char *strcat(char *restrict, const char *restrict);
char *strdup(const char *);
char *strrchr(const char *, int);
char *strpbrk(const char *, const char *);
#endif
