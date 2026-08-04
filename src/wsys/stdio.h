/* minimal freestanding stdio.h for the wasm32 build.
 *
 * FILE is just an int fd wrapper; fopen/fclose/fread/fwrite route through
 * the existing open/read/write/close VFS shims already defined in
 * src/0.c's `#if defined(wasm)` branch (same virtual filesystem `\l`/`bsl`
 * uses), so writing/reading a "file" from C code here behaves exactly like
 * any other VFS entry. printf/fprintf/snprintf/vsnprintf are implemented in
 * src/wasmlibc.c (compiled only for -Dwasm) since this project doesn't link
 * a real libc.
 *
 * stdout/stderr/stdin all map to the *same* inode (see 0.c: `d[8]={{.i=1},
 * {.i=1},{.i=1}}`), so both fd 1 and fd 2 already route to js_out() --
 * nothing here needs to special-case stderr separately.
 */
#ifndef WSYS_STDIO_H
#define WSYS_STDIO_H
#include <stddef.h>
#include <stdarg.h>

typedef struct { int fd; } FILE;
extern FILE _wsys_stdout, _wsys_stderr, _wsys_stdin;
#define stdout (&_wsys_stdout)
#define stderr (&_wsys_stderr)
#define stdin  (&_wsys_stdin)

FILE *fopen(const char *path, const char *mode);
int fclose(FILE *f);
size_t fread(void *ptr, size_t sz, size_t n, FILE *f);
size_t fwrite(const void *ptr, size_t sz, size_t n, FILE *f);
int fputs(const char *s, FILE *f);
int fputc(int c, FILE *f);
int puts(const char *s);
int putchar(int c);
int remove(const char *path);
int fflush(FILE *f);
int fseek(FILE *f, long off, int whence);
long ftell(FILE *f);

int printf(const char *restrict, ...);
int fprintf(FILE *restrict, const char *restrict, ...);
int vfprintf(FILE *restrict, const char *restrict, va_list);
int snprintf(char *restrict, size_t, const char *restrict, ...);
int vsnprintf(char *restrict, size_t, const char *restrict, va_list);
int sprintf(char *restrict, const char *restrict, ...);
#endif
