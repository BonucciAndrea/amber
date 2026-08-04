// amber_wasm.c - GNU AGPLv3 - see LICENSE and NOTICE
//
// Browser-facing export functions for the wasm32 build, compiled only when
// `-Dwasm` is set (matching src/0.c's existing `#if defined(wasm)` syscall
// shims, which this file sits directly on top of). The ABI here matches
// amber-notepad's amber.js exactly (see that file's AmberVM class):
//
//   amber_init()   - called once after instantiate; kinit() + a dummy argv
//                     so `.z`-style env access doesn't dereference NULL.
//   amber_inbuf()  - returns a pointer JS writes the next NUL-terminated
//                     input line (or example filename) into before calling
//                     amber_eval() / amber_load().
//   amber_eval()   - evaluate the NUL-terminated line at amber_inbuf() via
//                     the same evs(S,B) the native REPL's rep() loop uses.
//                     Output streams out through the existing write(1,...)
//                     -> js_out(...) shim in 0.c -- no explicit return value.
//   amber_load()   - load an embedded example by filename via the same
//                     bsl(S) the native `\l file.k` command uses (opens it
//                     from the `s[]` virtual filesystem in o/w/fs.h, reads
//                     it, evs()'s it).
//
// __heap_base and `memory` are exported by the linker (see build flags in
// tools/build_wasm.sh), not from this file.
#if defined(wasm)
#include "a.h"

#define INBUF_SZ (1<<16)
static char g_inbuf[INBUF_SZ];

static const char *dummy_argv[2] = {"amber", 0};

__attribute__((export_name("amber_init")))
void amber_init(void) {
    kinit();
    kargs(1, (S*)dummy_argv);
}

__attribute__((export_name("amber_inbuf")))
void *amber_inbuf(void) {
    return g_inbuf;
}

__attribute__((export_name("amber_eval")))
void amber_eval(void) {
    evs(g_inbuf, 1);
}

__attribute__((export_name("amber_load")))
void amber_load(void) {
    bsl(g_inbuf);
}
#endif
