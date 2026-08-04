/* wasmlibc.c - GNU AGPLv3 - see LICENSE and NOTICE
 *
 * A minimal freestanding libc for the wasm32 build (`-Dwasm`), covering
 * exactly what src/*.c actually calls beyond what src/0.c's own wasm branch
 * already provides (open/read/write/close/lseek/fstat/mmap/gettimeofday/
 * exit/memcpy/memmove/memset/memchr/memmem/memcmp/strlen/strchr/strstr/
 * strcmp). This file adds: a printf-family formatter (snprintf/vsnprintf/
 * sprintf/printf/fprintf/vfprintf), FILE*-based stdio wrapping the existing
 * VFS open/read/write/close, a malloc/calloc/realloc/free built on the
 * js_alloc/js_free imports (which already implement page-rounded free-list
 * reuse on the JS side -- see amber.js), getenv (always NULL: there's no
 * real process environment in a browser), strncmp/strcat/strtod, a
 * clock_gettime() built on the same js_time import gettimeofday() uses, and
 * a single-threaded pthread_create/pthread_join emulation (see wsys/
 * pthread.h's header comment for why: no SharedArrayBuffer-based worker
 * pool here, so parallel.c's threaded kernels just run inline instead).
 *
 * Only compiled for the wasm build -- see build flags in the wasm build
 * script, which is the only place `-Dwasm` is defined.
 */
#if defined(wasm)
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

/* ---- allocator: thin wrapper over js_alloc/js_free (env imports) ------- */
extern void *js_alloc(unsigned long n);
extern void js_free(void *p, unsigned long n);

void *malloc(size_t n) {
    size_t total = n + sizeof(size_t);
    unsigned char *base = (unsigned char *)js_alloc(total);
    if (!base) return 0;
    *(size_t *)base = total;
    return base + sizeof(size_t);
}
void free(void *p) {
    if (!p) return;
    unsigned char *base = (unsigned char *)p - sizeof(size_t);
    size_t total = *(size_t *)base;
    js_free(base, total);
}
void *calloc(size_t n, size_t sz) {
    size_t total = n * sz;
    void *p = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}
void *realloc(void *p, size_t n) {
    if (!p) return malloc(n);
    unsigned char *base = (unsigned char *)p - sizeof(size_t);
    size_t oldtotal = *(size_t *)base;
    size_t oldn = oldtotal - sizeof(size_t);
    void *np = malloc(n);
    if (np) { size_t c = oldn < n ? oldn : n; memcpy(np, p, c); free(p); }
    return np;
}

/* ---- env: no real process environment in a browser --------------------- */
char *getenv(const char *name) { (void)name; return 0; }

/* ---- a few string.h odds and ends not already in 0.c -------------------- */
int strncmp(const char *a, const char *b, size_t n) {
    if (!n) return 0;
    while (--n && *a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}
char *strcat(char *restrict d, const char *restrict s) {
    char *p = d + strlen(d);
    while ((*p++ = *s++)) {}
    return d;
}
char *strdup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}
double strtod(const char *restrict s, char **restrict endp) {
    const char *p = s;
    while (*p == ' ' || *p == '\t' || *p == '\n') p++;
    int neg = 0;
    if (*p == '-') { neg = 1; p++; } else if (*p == '+') p++;
    double v = 0;
    while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; }
    if (*p == '.') {
        p++;
        double f = 0.1;
        while (*p >= '0' && *p <= '9') { v += (*p - '0') * f; f *= 0.1; p++; }
    }
    if (*p == 'e' || *p == 'E') {
        const char *q = p + 1;
        int eneg = 0;
        if (*q == '-') { eneg = 1; q++; } else if (*q == '+') q++;
        int ev = 0; int any = 0;
        while (*q >= '0' && *q <= '9') { ev = ev * 10 + (*q - '0'); q++; any = 1; }
        if (any) {
            p = q;
            double scale = 1;
            for (int i = 0; i < ev; i++) scale *= 10;
            v = eneg ? v / scale : v * scale;
        }
    }
    if (endp) *endp = (char *)p;
    return neg ? -v : v;
}

/* ---- clock_gettime: reuse the same js_time import gettimeofday() uses -- */
extern void js_time(int *sec, int *usec);
int clock_gettime(int clk, struct timespec *ts) {
    (void)clk;
    int s = 0, u = 0;
    js_time(&s, &u);
    ts->tv_sec = s;
    ts->tv_nsec = (long)u * 1000;
    return 0;
}

/* ---- a few more libc odds and ends -------------------------------------- */
long long strtoll(const char *restrict s, char **restrict endp, int base) {
    (void)base; /* only base-10 is ever used in this codebase */
    const char *p = s;
    while (*p == ' ' || *p == '\t') p++;
    int neg = 0;
    if (*p == '-') { neg = 1; p++; } else if (*p == '+') p++;
    long long v = 0;
    while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; }
    if (endp) *endp = (char *)p;
    return neg ? -v : v;
}
char *strpbrk(const char *s, const char *accept) {
    for (; *s; s++) for (const char *a = accept; *a; a++) if (*s == *a) return (char *)s;
    return 0;
}
int posix_memalign(void **memptr, size_t alignment, size_t size) {
    /* our malloc() always hands back js_alloc()'d memory, which amber.js
     * page-rounds to 4096 bytes -- always well past any alignment this
     * codebase asks for (32 bytes, for AVX2/NEON-width arena blocks), so a
     * plain malloc() already satisfies the alignment contract here. */
    void *p = malloc(size ? size : 1);
    if (!p) return 12 /* ENOMEM */;
    *memptr = p;
    return 0;
}
int fflush(FILE *f) { (void)f; return 0; /* write() is unbuffered here */ }
int fseek(FILE *f, long off, int whence) {
    return lseek(f->fd, off, whence) < 0 ? -1 : 0;
}
long ftell(FILE *f) { return lseek(f->fd, 0, SEEK_CUR); }
void _exit(int code) { exit(code); }

/* ---- __int128 compiler-rt helpers --------------------------------------
 * src/s.c's hash function uses `__uint128_t` for a 128x64-bit multiply plus
 * a variable-amount shift, which clang lowers to calls to __multi3 (128x128
 * multiply, truncated) and __lshrti3 (128-bit logical shift right) on any
 * target -- like wasm32 -- with no native i128 ops. wasi-sdk normally pulls
 * these from compiler-rt; this freestanding build doesn't link it, so they
 * live here instead. Both use a union to pick apart the hi/lo 64-bit halves
 * by *memory layout* rather than by shifting the i128 itself, specifically
 * to avoid recursing back into __lshrti3 while implementing it. */
typedef union { unsigned __int128 v; struct { unsigned long long lo, hi; } w; } u128parts;

unsigned __int128 __multi3(unsigned __int128 a, unsigned __int128 b) {
    u128parts ua = {.v = a}, ub = {.v = b};
    unsigned long long alo = ua.w.lo, ahi = ua.w.hi, blo = ub.w.lo, bhi = ub.w.hi;
    /* 64x64->128 via 32-bit limbs, avoiding any i128 multiply of our own */
    unsigned long long a0 = alo & 0xffffffffu, a1 = alo >> 32;
    unsigned long long b0 = blo & 0xffffffffu, b1 = blo >> 32;
    unsigned long long t0 = a0 * b0;
    unsigned long long t1 = a1 * b0 + (t0 >> 32);
    unsigned long long t2 = a0 * b1 + (t1 & 0xffffffffu);
    unsigned long long lo_lo = (t2 << 32) | (t0 & 0xffffffffu);
    unsigned long long lo_hi = a1 * b1 + (t1 >> 32) + (t2 >> 32);
    u128parts r; r.w.lo = lo_lo; r.w.hi = lo_hi + ahi * blo + alo * bhi;
    return r.v;
}
unsigned __int128 __lshrti3(unsigned __int128 a, int n) {
    u128parts ua = {.v = a}, r = {.v = 0};
    if (n <= 0) return a;
    if (n >= 128) return 0;
    if (n >= 64) {
        r.w.lo = ua.w.hi >> (n - 64);
        r.w.hi = 0;
    } else {
        r.w.lo = (ua.w.lo >> n) | (n ? (ua.w.hi << (64 - n)) : 0);
        r.w.hi = ua.w.hi >> n;
    }
    return r.v;
}

/* ---- pthread: single-threaded emulation (see wsys/pthread.h) ----------- */
int pthread_create(pthread_t *th, const void *attr, void *(*start)(void *), void *arg) {
    (void)attr;
    if (th) *th = 0;
    start(arg);
    return 0;
}
int pthread_join(pthread_t th, void **retval) {
    (void)th;
    if (retval) *retval = 0;
    return 0;
}

/* ---- printf family -------------------------------------------------------
 * Supports exactly the conversions actually used in src/*.c: %s (with -N/N
 * width), %d, %ld, %lld, %u, %x (with 0-padding), %c, %f/%.Nf, %g, %%. Good
 * enough for a REPL's own diagnostic/table output; not a general libc
 * printf (no %e, no positional args, no long double). */
static char *fmt_uint(char *end, unsigned long long v, int base, int upper) {
    static const char *lo = "0123456789abcdef", *hi = "0123456789ABCDEF";
    const char *digits = upper ? hi : lo;
    char *p = end;
    if (!v) { *--p = '0'; return p; }
    while (v) { *--p = digits[v % (unsigned)base]; v /= (unsigned)base; }
    return p;
}
static void out_pad(char *restrict *o, size_t *left, char c, int n) {
    while (n-- > 0) { if (*left > 1) { **o = c; (*o)++; (*left)--; } }
}
static void out_str(char *restrict *o, size_t *left, const char *s, int n) {
    while (n-- > 0 && *s) { if (*left > 1) { **o = *s; (*o)++; (*left)--; } s++; }
}

int vsnprintf(char *restrict buf, size_t size, const char *restrict fmt, va_list ap) {
    char *o = buf;
    size_t left = size;
    char numbuf[64];
    for (const char *f = fmt; *f; f++) {
        if (*f != '%') { out_pad(&o, &left, *f, 1); continue; }
        f++;
        int leftalign = 0, zero = 0;
        while (*f == '-' || *f == '0') { if (*f == '-') leftalign = 1; else zero = 1; f++; }
        int width = 0;
        if (*f == '*') { width = va_arg(ap, int); f++; }
        else while (*f >= '0' && *f <= '9') { width = width * 10 + (*f - '0'); f++; }
        int prec = -1;
        if (*f == '.') {
            f++;
            if (*f == '*') { prec = va_arg(ap, int); f++; }
            else { prec = 0; while (*f >= '0' && *f <= '9') { prec = prec * 10 + (*f - '0'); f++; } }
        }
        int longlong = 0, longmod = 0;
        while (*f == 'l') { if (longmod) longlong = 1; longmod = 1; f++; }
        char c = *f;
        if (c == 's') {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            int n = (int)strlen(s);
            if (prec >= 0 && prec < n) n = prec;
            int pad = width - n;
            if (!leftalign) out_pad(&o, &left, ' ', pad);
            out_str(&o, &left, s, n);
            if (leftalign) out_pad(&o, &left, ' ', pad);
        } else if (c == 'c') {
            int ch = va_arg(ap, int);
            out_pad(&o, &left, (char)ch, 1);
        } else if (c == 'd' || c == 'i') {
            long long v = longlong ? va_arg(ap, long long) : (longmod ? va_arg(ap, long) : va_arg(ap, int));
            int neg = v < 0; unsigned long long uv = neg ? (unsigned long long)(-v) : (unsigned long long)v;
            char *p = fmt_uint(numbuf + sizeof numbuf, uv, 10, 0);
            int n = (int)(numbuf + sizeof numbuf - p);
            int total = n + neg;
            int pad = width - total;
            if (!leftalign) out_pad(&o, &left, zero ? '0' : ' ', pad > 0 ? pad : 0);
            if (neg) out_pad(&o, &left, '-', 1);
            out_str(&o, &left, p, n);
            if (leftalign) out_pad(&o, &left, ' ', pad > 0 ? pad : 0);
        } else if (c == 'u' || c == 'x' || c == 'X') {
            unsigned long long v = longlong ? va_arg(ap, unsigned long long) : (longmod ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int));
            char *p = fmt_uint(numbuf + sizeof numbuf, v, c == 'u' ? 10 : 16, c == 'X');
            int n = (int)(numbuf + sizeof numbuf - p);
            int pad = width - n;
            if (!leftalign) out_pad(&o, &left, zero ? '0' : ' ', pad > 0 ? pad : 0);
            out_str(&o, &left, p, n);
            if (leftalign) out_pad(&o, &left, ' ', pad > 0 ? pad : 0);
        } else if (c == 'f' || c == 'g') {
            double v = va_arg(ap, double);
            int neg = v < 0; if (neg) v = -v;
            int p = prec < 0 ? (c == 'g' ? 6 : 6) : prec;
            /* round */
            double scale = 1; for (int i = 0; i < p; i++) scale *= 10;
            unsigned long long scaled = (unsigned long long)(v * scale + 0.5);
            unsigned long long ip = p ? scaled / (unsigned long long)scale : scaled;
            unsigned long long fp = p ? scaled % (unsigned long long)scale : 0;
            char *ipp = fmt_uint(numbuf + 32, ip, 10, 0);
            int ipn = (int)(numbuf + 32 - ipp);
            if (neg) out_pad(&o, &left, '-', 1);
            out_str(&o, &left, ipp, ipn);
            if (p > 0) {
                out_pad(&o, &left, '.', 1);
                char fbuf[32]; char *fpp = fmt_uint(fbuf + sizeof fbuf, fp, 10, 0);
                int fpn = (int)(fbuf + sizeof fbuf - fpp);
                out_pad(&o, &left, '0', p - fpn);
                out_str(&o, &left, fpp, fpn);
            }
        } else if (c == '%') {
            out_pad(&o, &left, '%', 1);
        } else {
            out_pad(&o, &left, '%', 1);
            out_pad(&o, &left, c, 1);
        }
    }
    if (left > 0) *o = 0; else if (size) buf[size - 1] = 0;
    return (int)(o - buf);
}
int snprintf(char *restrict buf, size_t size, const char *restrict fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return r;
}
int sprintf(char *restrict buf, const char *restrict fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vsnprintf(buf, (size_t)-1, fmt, ap);
    va_end(ap);
    return r;
}

/* ---- FILE*-based stdio, wrapping 0.c's existing VFS open/read/write ---- */
FILE _wsys_stdout = {1}, _wsys_stderr = {2}, _wsys_stdin = {0};

FILE *fopen(const char *path, const char *mode) {
    int flags = O_RDONLY;
    if (mode[0] == 'w') flags = O_WRONLY | O_CREAT | O_TRUNC;
    else if (mode[0] == 'a') flags = O_WRONLY | O_CREAT | O_APPEND;
    int fd = open(path, flags, 0);
    if (fd < 0) return 0;
    FILE *f = (FILE *)malloc(sizeof *f);
    if (!f) { close(fd); return 0; }
    f->fd = fd;
    return f;
}
int fclose(FILE *f) {
    if (!f || f == &_wsys_stdout || f == &_wsys_stderr || f == &_wsys_stdin) return 0;
    close(f->fd);
    free(f);
    return 0;
}
size_t fread(void *ptr, size_t sz, size_t n, FILE *f) {
    long got = read(f->fd, ptr, sz * n);
    return got > 0 ? (size_t)got / (sz ? sz : 1) : 0;
}
size_t fwrite(const void *ptr, size_t sz, size_t n, FILE *f) {
    long want = (long)(sz * n);
    long got = write(f->fd, ptr, want);
    return got > 0 ? (size_t)got / (sz ? sz : 1) : 0;
}
int fputs(const char *s, FILE *f) { write(f->fd, s, strlen(s)); return 0; }
int fputc(int c, FILE *f) { char ch = (char)c; write(f->fd, &ch, 1); return c; }
int puts(const char *s) { fputs(s, &_wsys_stdout); fputc('\n', &_wsys_stdout); return 0; }
int putchar(int c) { return fputc(c, &_wsys_stdout); }
int remove(const char *path) { (void)path; return 0; /* best-effort: VFS entries are short-lived per session */ }

int vfprintf(FILE *restrict f, const char *restrict fmt, va_list ap) {
    char buf[4096];
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    write(f->fd, buf, n < (int)sizeof buf ? n : (int)sizeof buf - 1);
    return n;
}
int fprintf(FILE *restrict f, const char *restrict fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vfprintf(f, fmt, ap);
    va_end(ap);
    return r;
}
int printf(const char *restrict fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int r = vfprintf(&_wsys_stdout, fmt, ap);
    va_end(ap);
    return r;
}
#endif
