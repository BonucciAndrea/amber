/* minimal freestanding pthread.h -- single-threaded emulation.
 *
 * WASM has no real thread support without a JS Worker-based pthread shim
 * (what emscripten's -pthread flag provides via SharedArrayBuffer), which
 * this freestanding build doesn't have. Rather than fail to link, this
 * shim makes pthread_create() call the start routine *synchronously*, right
 * there, before returning -- so src/parallel.c's multithreaded vector
 * engine still produces byte-identical results, just without actual
 * parallelism (the same "runs sequentially in the browser sandbox"
 * tradeoff amber-notepad's README already documents for `peach`). See
 * src/wasmlibc.c for the implementation.
 */
#ifndef WSYS_PTHREAD_H
#define WSYS_PTHREAD_H
typedef unsigned int pthread_t;
int pthread_create(pthread_t *th, const void *attr, void *(*start)(void *), void *arg);
int pthread_join(pthread_t th, void **retval);
#endif
