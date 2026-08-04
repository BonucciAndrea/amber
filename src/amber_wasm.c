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
//   amber_eval()   - evaluate the NUL-terminated line at amber_inbuf(). Runs
//                     the line through qsql.k's bare-qSQL rewriter (`qrw`)
//                     first -- see the comment above amber_eval() below for
//                     why that's necessary here but not in bsl()/amber_load().
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
    // repl.k (the native REPL's own startup script) loads these four core
    // libraries before dropping you at a prompt -- do the same here so a
    // fresh page has `sum`/`qby`/`gentq`/`select`/... available immediately,
    // matching native ./a's behavior. Individual example files `\l` any
    // further libraries themselves (hdb.k, ipc.k, temporal.k, sys.k) exactly
    // like they do natively.
    A r;
    r = bsl("amber.k"); if (r) mr(r);
    r = bsl("fin.k");   if (r) mr(r);
    r = bsl("std.k");   if (r) mr(r);
    r = bsl("qsql.k");  if (r) mr(r);
    // wprint: auto-render tables/keyed-tables (types `M`/`m) via amber.k's
    // own box-drawing formatter (amfmt -- the same one the native REPL's
    // `fmt` uses, minus its `upd[]` step, which shells out to `tput` for
    // terminal size and has no meaning in a browser; amtab/amkeyed/amdict
    // don't depend on that, only on the CROWS row cap, so this is safe to
    // call unconditionally). Anything that ISN'T a table is returned
    // unchanged so amber_eval()'s own evs(...,1) auto-print handles it
    // exactly as it always has -- this only changes what tables look like.
    r = evs("wprint:{$[|/(@x)=`m`M;[`0:\"\\n\"/amfmt x;(::)];x]}", 0);
    if (r) mr(r);
}

__attribute__((export_name("amber_inbuf")))
void *amber_inbuf(void) {
    return g_inbuf;
}

// qsql.k defines a bare-qSQL rewriter, `qrw`, that turns an unwrapped
// `select ... [by ...] from ... [where ...]` (and exec/update/delete) into
// the matching `sel"..."`/`exq"..."`/`upd"..."`/`del"..."` string-call --
// see qsql.k's own header comment on qrw. The *native* interactive REPL
// (repl.k's `line1`) already runs every typed line through `qrw` before
// evaluating it, which is why bare `select ... by ... from ...` works when
// you type it at the `amber>` prompt -- but that rewrite is REPL-only: it's
// never part of evs() itself, so it's invisible to bsl()/script execution,
// and (until this fix) to the browser IDE's per-line eval, which called
// evs() directly. Reproducing repl.k's own `. qrw x` pattern here (as a K
// source string, not a hand-rolled C-level function call) is the smallest
// correct fix: it's exactly what the REPL does, applied where the REPL
// itself can't reach. `qrw` is a no-op passthrough for anything that isn't
// bare qSQL (plain expressions, `\`-commands, ...), so this doesn't change
// behavior for anything else -- see amber_wasm.c's test notes for the
// `\ast`/backslash-command case, which still works because `.` (val) on a
// string re-enters evs() and rechecks the leading `\` exactly like a fresh
// top-level line would.
#define REWRITE_SZ (2 * INBUF_SZ + 16)
static char g_rewritebuf[REWRITE_SZ];

static void qsql_rewrite_and_eval(const char *line) {
    char *o = g_rewritebuf;
    // wprint(. qrw "<escaped line>") -- rewrite bare qSQL if present, run
    // it, then let wprint (defined in amber_init) decide how to print the
    // result: box-drawn table if it's table-shaped, otherwise unchanged
    // (falls through to evs()'s normal auto-print below).
    const char *prefix = "wprint(. qrw \"";
    while (*prefix) *o++ = *prefix++;
    for (const char *p = line; *p; p++) {
        if (*p == '\\' || *p == '"') *o++ = '\\';
        *o++ = *p;
    }
    const char *suffix = "\")";
    while (*suffix) *o++ = *suffix++;
    *o = 0;
    evs(g_rewritebuf, 1);
}

__attribute__((export_name("amber_eval")))
void amber_eval(void) {
    qsql_rewrite_and_eval(g_inbuf);
}

__attribute__((export_name("amber_load")))
void amber_load(void) {
    bsl(g_inbuf);
}

// amber_read(): return the raw source text of a file from the embedded
// virtual filesystem (o/w/fs.h) at the name written to amber_inbuf(),
// WITHOUT evaluating it -- unlike amber_load()/bsl(), which reads *and*
// runs it. This is what lets the browser IDE show an example's actual
// source in the editor when you pick it, instead of only its output.
// Every bundled example is a small hand-written .k file (well under this
// buffer), so a fixed-size static buffer is fine -- same pattern as
// g_inbuf/g_rewritebuf above.
#include <fcntl.h>
#define READBUF_SZ (1<<17)
static char g_readbuf[READBUF_SZ];

__attribute__((export_name("amber_read")))
void *amber_read(void) {
    int fd = open(g_inbuf, O_RDONLY, 0);
    if (fd < 0) { g_readbuf[0] = 0; return g_readbuf; }
    int n = read(fd, g_readbuf, READBUF_SZ - 1);
    close(fd);
    if (n < 0) n = 0;
    g_readbuf[n] = 0;
    return g_readbuf;
}
#endif
