/* ext.c  -  Amber 1.9.5 extension seam.  See src/ext.h for the contract.
 *
 * Deliberately dependency-free: <string.h> and nothing else.  Every symbol
 * here exists so that the rest of the engine can ask "is anything plugged in?"
 * with one pointer test, and so that a build with an empty ext/ directory is
 * byte-for-byte the engine it was before the seam existed.
 *
 * Amber - GNU AGPLv3 - see LICENSE and NOTICE.
 */
#include "ext.h"

#include <string.h>

/* ---- hooks --------------------------------------------------------------- */

void       (*am_ext_startup)(void)                                      = 0;
int        (*am_ext_hint)(const char *, size_t, char *, size_t)         = 0;
void       (*am_ext_complete)(const char *, void *)                    = 0;
void       (*am_ext_complete_late)(const char *, void *)               = 0;
unsigned long long (*am_ext_bs)(const char *)                          = 0;
const char  *am_ext_usage                                              = 0;
const char  *am_ext_banner                                             = 0;

void am_ext_startup_once(void) {
    static int done;
    if (done) return;
    done = 1;
    if (am_ext_startup) am_ext_startup();
}

/* ---- verb registry ------------------------------------------------------- */
/*
 * Amber interns a symbol of up to four characters as those four bytes read as
 * an `int` (src/f.c fI() compares the caller's value against a table of
 * char[4]).  Packing a name here with the same memcpy therefore yields exactly
 * the key sym1() is holding, on either endianness, with no dependency on the
 * interpreter's symbol table.
 */
#define AM_EXT_MAX_VERBS 32

static struct { int key; void *fn; } g_verb[AM_EXT_MAX_VERBS];
static int g_verb_n;

static int pack(const char *name) {
    int key = 0;
    size_t n;
    if (!name) return 0;
    n = strlen(name);
    if (n == 0 || n > 4) return 0;
    memcpy(&key, name, n);
    return key;
}

int am_ext_verb(const char *name, void *fn) {
    int key = pack(name), i;
    if (!key || !fn) return -1;
    for (i = 0; i < g_verb_n; i++)
        if (g_verb[i].key == key) { g_verb[i].fn = fn; return 0; }  /* re-bind */
    if (g_verb_n == AM_EXT_MAX_VERBS) return -1;
    g_verb[g_verb_n].key = key;
    g_verb[g_verb_n].fn  = fn;
    g_verb_n++;
    return 0;
}

void *am_ext_verb_lookup(int packed) {
    int i;
    if (!g_verb_n) return 0;                 /* the overwhelmingly common case */
    for (i = 0; i < g_verb_n; i++)
        if (g_verb[i].key == packed) return g_verb[i].fn;
    return 0;
}

int am_ext_verb_count(void) { return g_verb_n; }

/* ---- accepted-suggestion channel ----------------------------------------- */

static char g_accepted[512];

void am_ext_set_accepted(const char *line) {
    if (!line) { g_accepted[0] = 0; return; }
    strncpy(g_accepted, line, sizeof g_accepted - 1);
    g_accepted[sizeof g_accepted - 1] = 0;
}

const char *am_repl_take_accepted(void) {
    static char out[512];
    memcpy(out, g_accepted, sizeof out);
    g_accepted[0] = 0;
    return out;
}

/* ===========================================================================
 * 6. DYNAMIC C API  --  the libamber.so implementation.  Contract: ext.h §6.
 * ===========================================================================
 *
 * WHY THIS LIVES HERE, next to the in-process seam, rather than in a new file:
 * both halves answer the same question -- "how does something OUTSIDE the
 * engine reach in without the engine reaching back out?" -- and keeping them in
 * one translation unit means the rule is stated once and enforced once.  The
 * file's original half is deliberately dependency-free; this half needs the
 * interpreter's own value type, so it pulls in src/a.h (an INTERNAL header, not
 * a satellite's).  Nothing here includes Python.h, arrow/c/abi.h, grpc, or any
 * other consumer's header, and nothing here is compiled differently depending
 * on which satellites exist.  That invariant is the whole architecture:
 *
 *     ./build.sh              -> ./amber          (unchanged, zero new cost)
 *     ./build.sh --shared     -> libamber.so      (the same objects, -fPIC)
 *
 * Every satellite -- python-amber, amber-arrow, amber-jupyter's kernel,
 * vscode-amber's LSP, grafana-amber-datasource, amber-flame -- consumes ONE of
 * these two artefacts.  None of them is mentioned anywhere in src/.
 *
 * A NOTE ON STYLE: everything below is written in plain C99 rather than in the
 * dense K-idiom the rest of src/ uses.  That is on purpose.  This is the file a
 * binding author reads while writing FFI declarations in another language, and
 * it is the one place in the tree where clarity to an outsider is worth more
 * than density.  a.h's single-letter macro vocabulary (I, C, S, F, W, P, Q, X,
 * Y, ...) is still in scope here, so the identifiers used below are chosen to
 * stay clear of it.
 */

/* amber 2.0.0: the libamber.so C API (section 6) is excluded from the freestanding
 * wasm build -- its amber_init(const char*) would collide with the browser's
 * amber_init(void) in src/amber_wasm.c, and the wasm never links libamber.so. */
#ifndef wasm
#include "a.h"      /* the interpreter's own value type A, and its verbs */
#include "arena.h"  /* arena_free() for amber_shutdown()                 */
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* ---- 6.0 shared state ----------------------------------------------------
 * Thread-local, because e.c's own error buffer is: a peach worker that raises
 * formats into its own storage, and a host that drives the API from two
 * threads must see two independent error slots or neither is trustworthy. */

#define CAPI_ERRMAX 4096
static AM_TLS char capi_err[CAPI_ERRMAX];
static int capi_booted;

static void capi_err_clear(void) { capi_err[0] = 0; }

/* Drain e.c's compact caret text into capi_err.  err(au) is the interpreter's
 * OWN accessor for that buffer (it is what the `` `err `` verb returns and what
 * .[f;args;handler] hands a trap handler), so this reads exactly the string an
 * Amber-level handler would have received -- no second, drifting formatting
 * path.  e.c re-points its buffer at the start of every NEW error, so there is
 * nothing to reset here. */
static void capi_err_grab(void) {
    A msg = err(au);
    unsigned long long len;
    if (!msg) { capi_err[0] = 0; return; }
    len = (unsigned long long)_n(msg);
    if (len >= CAPI_ERRMAX) len = CAPI_ERRMAX - 1;
    memcpy(capi_err, _C(msg), (size_t)len);
    /* the caret block ends in a newline; trim it -- a host turning this into an
     * exception message does not want a trailing blank line. */
    while (len && (capi_err[len - 1] == '\n' || capi_err[len - 1] == '\r')) len--;
    capi_err[len] = 0;
    mr(msg);
}

static void capi_err_set(const char *text) {
    size_t len = strlen(text);
    if (len >= CAPI_ERRMAX) len = CAPI_ERRMAX - 1;
    memcpy(capi_err, text, len);
    capi_err[len] = 0;
}

/* ---- 6.1 identity -------------------------------------------------------- */

int         amber_abi_version(void)   { return AMBER_CAPI_ABI; }
const char *amber_version_string(void){ return AMBER_VERSION; }

/* ---- 6.2 lifecycle ------------------------------------------------------- */

/* The standard library, in the order repl.k loads it.  The order matters:
 * std -> qsql -> temporal -> sys -> hdb -> ipc (sys.k's plot/candle need
 * temporal.k's tsms/tsdate; hdb.k and ipc.k need the rest). */
static const char *CAPI_MODULES[] = {
    "amber.k", "fin.k", "std.k", "qsql.k",
    "temporal.k", "sys.k", "hdb.k", "ipc.k"
};

/* Load one module by absolute path through bsl(), the SAME entry point the
 * REPL's `\l file.k` uses -- so a module either loads here exactly as it loads
 * there, or it fails identically.  Returns 1 if the file was read. */
static int capi_load_module(const char *home, const char *leaf) {
    char path[4096];
    size_t hn = strlen(home), ln = strlen(leaf);
    A res;
    if (hn + ln + 2 >= sizeof path) return 0;
    memcpy(path, home, hn);
    if (hn && path[hn - 1] != '/') path[hn++] = '/';
    memcpy(path + hn, leaf, ln);
    path[hn + ln] = 0;
    res = bsl(path);
    if (!res) return 0;
    mr(res);
    return 1;
}

int amber_init(const char *home) {
    static const char *boot_argv[2] = { "amber", 0 };
    int loaded = 0, want = 0;
    int saved_fd1 = -1, devnull = -1;
    unsigned mi;

    capi_err_clear();
    if (!capi_booted) {
        kinit();
        kargs(1, (S *)boot_argv);
        capi_booted = 1;
    }
    if (!home || !*home) return 0;

    /* The modules are quiet on a healthy load, but `bsl` runs them through the
     * auto-printing evaluator, so a stray expression in a user-supplied home
     * directory would write to the HOST's stdout -- which for a Jupyter kernel
     * or an LSP daemon speaking JSON-RPC on that very stream is not a cosmetic
     * problem but a protocol violation.  Redirect fd 1 itself for the duration
     * (the same technique vm_selftest() and ast_selftest() already use), so the
     * guarantee holds no matter what the modules do. */
    fflush(stdout);
    saved_fd1 = dup(1);
    devnull = open("/dev/null", O_WRONLY);
    if (saved_fd1 >= 0 && devnull >= 0) dup2(devnull, 1);

    want = (int)(sizeof CAPI_MODULES / sizeof *CAPI_MODULES);
    for (mi = 0; mi < sizeof CAPI_MODULES / sizeof *CAPI_MODULES; mi++)
        loaded += capi_load_module(home, CAPI_MODULES[mi]);

    fflush(stdout);
    if (saved_fd1 >= 0) { dup2(saved_fd1, 1); close(saved_fd1); }
    if (devnull >= 0) close(devnull);

    if (loaded == 0) {
        capi_err_set("'io: no Amber standard-library module could be read from the given home directory");
        return -1;
    }
    if (loaded < want) {
        /* partial load is survivable -- report it without failing the boot */
        capi_err_set("'io: some Amber standard-library modules were missing from the given home directory");
    }
    return 0;
}

void amber_shutdown(void) { arena_reset(); arena_free(); }

/* ---- 6.3 evaluation ------------------------------------------------------ */

/* Turn a C name into the symbol value the global table is keyed by.
 *
 * A plain name interns to a symbol ATOM. A DOTTED name does not: Amber's global
 * table keys a namespaced global on the pair (namespace, name), and the parser
 * hands it a symbol LIST -- (`amber;`export) for `arrow.export` -- not a single
 * symbol whose text happens to contain a dot. Interning "arrow.export" as one
 * symbol therefore produces a key that matches nothing, and every dotted global
 * in the standard library (arrow.*, hdb.*, ipc.*'s u.*, and any helper a
 * binding installs for itself) would be unreachable through amber_call() and
 * amber_get_global() -- with a plain 'value error and no hint as to why.
 *
 * Returns an OWNED value the caller passes on to gg()/gp(), which consume it. */
static A capi_name_sym(const char *name) {
    const char *scan = name, *part = name;
    unsigned parts = 1, i = 0;
    A vec;
    I *ids;
    char buf[256];

    if (!strchr(name, '.')) return sym((S)name);
    for (scan = name; *scan; scan++) if (*scan == '.') parts++;
    vec = aS(parts);
    if (!vec) return sym((S)name);
    ids = (I *)_V(vec);
    for (scan = name; ; scan++) {
        if (*scan == '.' || !*scan) {
            size_t len = (size_t)(scan - part);
            if (len >= sizeof buf) len = sizeof buf - 1;
            memcpy(buf, part, len);
            buf[len] = 0;
            if (i < parts) ids[i++] = (I)us(buf);
            part = scan + 1;
            if (!*scan) break;
        }
    }
    return vec;
}

static int capi_ready(void) {
    if (capi_booted) return 1;
    capi_err_set("'value: amber_init() has not been called");
    return 0;
}

amber_value amber_eval_str(const char *src) {
    A res;
    if (!capi_ready()) return 0;
    capi_err_clear();
    if (!src) { capi_err_set("'domain: null source"); return 0; }
    /* r=0: evaluate every statement, return the LAST one's value, print
     * nothing.  This is evs()'s library mode -- the REPL uses r=1. */
    res = evs((S)src, 0);
    if (!res) capi_err_grab();
    return (amber_value)res;
}

amber_value amber_call(const char *fname, const amber_value *args, int argc) {
    A fn, res;
    A slot[8];
    ArenaMark mark;
    int i;
    if (!capi_ready()) return 0;
    capi_err_clear();
    if (!fname)              { capi_err_set("'domain: null function name"); return 0; }
    if (argc < 0 || argc > 8){ capi_err_set("'rank: amber_call takes 0..8 arguments"); return 0; }
    fn = gg(capi_name_sym(fname));       /* consumes the name; owned or 0 */
    if (!fn) { capi_err_grab(); return 0; }
    /* amber_call() does not go through evs(), so nothing else rewinds the
     * scratchpad on this path: a host calling it in a loop -- which is the
     * whole point of the entry point -- would otherwise accumulate one
     * statement's arena per call. Mark/release rather than reset, for the same
     * re-entrancy reason as evs(). */
    mark = arena_mark();
    if (argc == 0) {
        /* f[] -- apply to the generic null, exactly as the compiler emits for
         * an empty argument list. */
        res = _1(fn, au);
    } else {
        for (i = 0; i < argc; i++) slot[i] = _R((A)args[i]);
        res = _8(fn, slot, (U)argc);     /* consumes fn and every slot[i] */
    }
    if (!res) capi_err_grab();           /* copies the text out; safe to rewind */
    arena_release(mark);
    return (amber_value)res;
}

amber_value amber_eval_qsql(const char *line) {
    A src, res;
    if (!capi_ready()) return 0;
    capi_err_clear();
    if (!line) { capi_err_set("'domain: null source"); return 0; }
    /* `. qrw x` is repl.k's own pattern, reproduced where the REPL cannot
     * reach: qrw turns bare `select ... from ...` into the equivalent
     * sel"..."/exq"..."/upd"..."/del"..." call and is an identity function for
     * anything that is not bare qSQL, so this is safe for ordinary
     * expressions too. */
    src = aCn((S)line, (U)strlen(line));
    if (!src) { capi_err_set("'limit: allocation failed"); return 0; }
    res = K1("{. qrw x}", src);          /* consumes src */
    if (res) return (amber_value)res;
    /* qrw undefined (bare engine, no standard library) -- fall back rather than
     * report a confusing 'value error about a rewriter the caller never asked
     * for.  A real syntax error in `line` still surfaces from evs(). */
    capi_err_clear();
    return amber_eval_str(line);
}

const char *amber_last_error(void) { return capi_err; }

int amber_set_diagnostics(int on) {
    int was = amdiag;
    amdiag = on ? 1 : 0;
    return was < 0 ? 1 : was;   /* -1 means "never resolved"; the default is on */
}

/* ---- 6.4 ownership ------------------------------------------------------- */

amber_value amber_retain(amber_value handle) {
    if (!handle) return 0;
    return (amber_value)_R((A)handle);
}

void amber_release(amber_value handle) {
    if (!handle) return;
    mr((A)handle);          /* mr() is a documented no-op on tagged immediates */
}

/* ---- 6.5 introspection --------------------------------------------------- */

int amber_type(amber_value handle) {
    if (!handle) return 0;
    return (int)(unsigned char)_t((A)handle);
}

const char *amber_type_name(amber_value handle) {
    switch (amber_type(handle)) {
        case tA:   return "list";
        case tE:   return "range";
        case tB:   return "bool vector";
        case tG:   return "byte vector";
        case tH:   return "short vector";
        case tI:   return "int vector";
        case tL:   return "long vector";
        case tF:   return "float vector";
        case tC:   return "char vector";
        case tS:   return "symbol vector";
        case tM:   return "table";
        case tm:   return "dict";
        case ti:   return "int";
        case tl:   return "long";
        case tf:   return "float";
        case tc:   return "char";
        case ts:   return "symbol";
        case tdt:  return "date";
        case ttm:  return "time";
        case tnp:  return "timestamp";
        default:   return "function";
    }
}

long long amber_count(amber_value handle) {
    if (!handle) return -1;
    return (long long)_N((A)handle);   /* v.c: rows for a table, keys for a dict */
}

int amber_is_table(amber_value handle) { return amber_type(handle) == tM; }
int amber_is_dict (amber_value handle) { return amber_type(handle) == tm; }
int amber_is_atom (amber_value handle) { return handle && _tt((A)handle) ? 1 : 0; }

/* ---- 6.6 flat vectors ---------------------------------------------------- */

const void *amber_get_vector_ptr(amber_value handle,
                                 int *out_type,
                                 long long *out_len,
                                 int *out_elem_bits) {
    A val;
    unsigned char tp;
    if (out_type) *out_type = 0;
    if (out_len) *out_len = 0;
    if (out_elem_bits) *out_elem_bits = 0;
    if (!handle) return 0;
    val = (A)handle;
    tp = (unsigned char)_t(val);
    if (out_type) *out_type = (int)tp;
    if (out_elem_bits) *out_elem_bits = 1 << Tw[tp];
    /* A tagged immediate has no heap object at all: reading _n(val) would
     * dereference (pointer - 4) on a value that is not a pointer.  Bail before
     * touching the header. */
    if (_t0(val)) return 0;
    /* tA (general list), tM (table), tm (dict) and the function types have a
     * payload, but it is an array of A handles, not of scalars -- exposing it
     * as a "vector pointer" would be an invitation to read it wrong.  tE is
     * lazy and has no materialised payload at all. */
    if (tp == tA || tp == tE || tp >= tM) return 0;
    if (out_len) *out_len = (long long)_n(val);
    return (const void *)_V(val);
}

amber_value amber_flatten(amber_value handle) {
    A val;
    if (!handle) return 0;
    val = (A)handle;
    if (!_t0(val) && _t(val) == tE) return (amber_value)gZ(_R(val));
    return (amber_value)_R(val);
}

const char *amber_symbol_name(int symbol_id) { return su((U)symbol_id); }

/* ---- 6.7 scalars --------------------------------------------------------- */

long long amber_to_int(amber_value handle, int *ok) {
    A val = (A)handle;
    if (ok) *ok = 1;
    if (handle) switch ((unsigned char)_t(val)) {
        case ti: case tc: case tdt: case ttm: return (long long)_v(val);
        case ts:                              return (long long)(U)_v(val);
        case tl: case tnp:                    return (long long)*_L(val);
        case tf:                              return (long long)*_F(val);
    }
    if (ok) *ok = 0;
    return 0;
}

double amber_to_float(amber_value handle, int *ok) {
    A val = (A)handle;
    if (ok) *ok = 1;
    if (handle) switch ((unsigned char)_t(val)) {
        case tf:                              return *_F(val);
        case tl: case tnp:                    return (double)*_L(val);
        case ti: case tc: case tdt: case ttm: return (double)_v(val);
    }
    if (ok) *ok = 0;
    return 0.0;
}

int amber_to_symbol_id(amber_value handle, int *ok) {
    if (handle && (unsigned char)_t((A)handle) == ts) { if (ok) *ok = 1; return _v((A)handle); }
    if (ok) *ok = 0;
    return 0;
}

long long amber_to_string(amber_value handle, char *dst, long long cap) {
    A val = (A)handle;
    long long len;
    if (!handle) return -1;
    if ((unsigned char)_t(val) == tc) {           /* char atom */
        if (dst && cap > 1) { dst[0] = (char)_v(val); dst[1] = 0; }
        else if (dst && cap > 0) dst[0] = 0;
        return 1;
    }
    if ((unsigned char)_t(val) != tC) return -1;  /* not char-shaped */
    len = (long long)_n(val);
    if (dst && cap > 0) {
        long long copy = len < cap - 1 ? len : cap - 1;
        if (copy > 0) memcpy(dst, _C(val), (size_t)copy);
        dst[copy] = 0;
    }
    return len;
}

/* ---- 6.8 tables and dictionaries ----------------------------------------- */

/* A table (type `M) and a dictionary (type `m) share a shape: field 0 is the
 * key/name vector, field 1 the value/column list.  See src/a.c's iM() and
 * src/ar.c's arrowExport(), which read the same two fields. */

int amber_table_ncols(amber_value table) {
    A val = (A)table;
    if (!table || _t0(val) || _t(val) != tM) return -1;
    return (int)_n(_x(val));
}

const char *amber_table_colname(amber_value table, int col) {
    A val = (A)table, names;
    if (amber_table_ncols(table) <= col || col < 0) return 0;
    val = (A)table; names = _x(val);
    return su((U)((const I *)_V(names))[col]);
}

amber_value amber_table_column(amber_value table, int col) {
    A val, cols;
    if (amber_table_ncols(table) <= col || col < 0) {
        capi_err_set("'index: column index out of range");
        return 0;
    }
    val = (A)table; cols = _y(val);
    return amber_flatten((amber_value)((A *)_V(cols))[col]);
}

amber_value amber_dict_keys(amber_value dict) {
    A val = (A)dict;
    if (!dict || _t0(val) || _t(val) != tm) { capi_err_set("'type: not a dictionary"); return 0; }
    return (amber_value)_R(_x(val));
}

amber_value amber_dict_values(amber_value dict) {
    A val = (A)dict;
    if (!dict || _t0(val) || _t(val) != tm) { capi_err_set("'type: not a dictionary"); return 0; }
    return (amber_value)_R(_y(val));
}

/* ---- 6.9 pushing data back in -------------------------------------------- */

amber_value amber_int(long long value)   { return (amber_value)al((L)value); }
amber_value amber_float(double value)    { return (amber_value)af((F)value); }
amber_value amber_symbol(const char *nm) { return (amber_value)sym(nm ? (S)nm : ""); }
amber_value amber_null(void)             { return (amber_value)au; }

amber_value amber_make_list(const amber_value *items, long long n) {
    A vec;
    A *slot;
    long long i;
    if (n < 0 || (!items && n)) return 0;
    vec = aA((U)n);
    if (!vec) return 0;
    slot = (A *)_V(vec);
    for (i = 0; i < n; i++) slot[i] = _R((A)items[i]);
    return (amber_value)sqz(vec);
}

amber_value amber_from_int32  (const int *src, long long n)       { return n < 0 ? 0 : (amber_value)aV(tI, (U)n, src); }
amber_value amber_from_int64  (const long long *src, long long n) { return n < 0 ? 0 : (amber_value)aV(tL, (U)n, src); }
amber_value amber_from_float64(const double *src, long long n)    { return n < 0 ? 0 : (amber_value)aV(tF, (U)n, src); }
amber_value amber_from_bytes  (const char *src, long long n)      { return n < 0 ? 0 : (amber_value)aCn((S)src, (U)n); }

amber_value amber_from_symbols(const char *const *src, long long n) {
    A vec;
    I *ids;
    long long i;
    if (n < 0) return 0;
    vec = aS((U)n);
    if (!vec) return 0;
    ids = (I *)_V(vec);
    for (i = 0; i < n; i++) ids[i] = (I)us(src[i] ? (S)src[i] : "");
    return (amber_value)vec;
}

amber_value amber_make_table(const char *const *names,
                             const amber_value *cols,
                             int ncols) {
    A nm, cv, res;
    A *slot;
    int i;
    if (!capi_ready()) return 0;
    capi_err_clear();
    if (ncols < 0 || (!names && ncols) || (!cols && ncols)) {
        capi_err_set("'domain: amber_make_table got a null argument");
        return 0;
    }
    nm = (A)amber_from_symbols(names, ncols);
    cv = aA((U)(ncols ? ncols : 0));
    if (!nm || !cv) { capi_err_set("'limit: allocation failed"); return 0; }
    slot = (A *)_V(cv);
    for (i = 0; i < ncols; i++) slot[i] = _R((A)cols[i]);
    /* `+names!cols` -- flip a dictionary into a table.  Expressed in K rather
     * than assembled by hand so it stays correct if the internal table
     * representation ever changes; K2 compiles the lambda once and caches it. */
    res = K2("{+x!y}", nm, cv);          /* consumes nm and cv */
    if (!res) capi_err_grab();
    return (amber_value)res;
}

amber_value amber_make_dict(const char *const *keys,
                            const amber_value *vals,
                            int n) {
    A kv_, vv, res;
    A *slot;
    int i;
    if (!capi_ready()) return 0;
    capi_err_clear();
    if (n < 0 || (!keys && n) || (!vals && n)) {
        capi_err_set("'domain: amber_make_dict got a null argument");
        return 0;
    }
    kv_ = (A)amber_from_symbols(keys, n);
    vv  = aA((U)(n ? n : 0));
    if (!kv_ || !vv) { capi_err_set("'limit: allocation failed"); return 0; }
    slot = (A *)_V(vv);
    for (i = 0; i < n; i++) slot[i] = _R((A)vals[i]);
    res = K2("{x!y}", kv_, vv);          /* consumes both */
    if (!res) capi_err_grab();
    return (amber_value)res;
}

int amber_set_global(const char *name, amber_value v) {
    A *slot;
    if (!capi_ready()) return -1;
    capi_err_clear();
    if (!name || !*name) { capi_err_set("'domain: null global name"); return -1; }
    slot = gp(capi_name_sym(name));      /* consumes the name */
    if (!slot) { capi_err_set("'limit: global table full"); return -1; }
    if (*slot) mr(*slot);
    *slot = v ? _R((A)v) : 0;
    return 0;
}

amber_value amber_get_global(const char *name) {
    A res;
    if (!capi_ready()) return 0;
    capi_err_clear();
    if (!name || !*name) { capi_err_set("'domain: null global name"); return 0; }
    res = gg(capi_name_sym(name));
    if (!res) capi_err_grab();
    return (amber_value)res;
}

/* ---- 6.10 Apache Arrow C Data Interface ---------------------------------- */

/* Build the (names; cols) pair src/ar.c's arrowExport() wants, and run it.
 * Returns the two heap struct addresses (schema, array), or -1. */
static int capi_arrow_raw(amber_value table, struct ArrowSchema **sc, struct ArrowArray **ar) {
    A val = (A)table, pair, res;
    A slot[2];
    const L *addr;
    if (!capi_ready()) return -1;
    capi_err_clear();
    if (!table || _t0(val) || _t(val) != tM) {
        capi_err_set("'type: amber_arrow_export needs a table");
        return -1;
    }
    slot[0] = _R(_x(val));
    slot[1] = _R(_y(val));
    pair = aV(tA, 2, slot);
    if (!pair) { mr(slot[0]); mr(slot[1]); capi_err_set("'limit: allocation failed"); return -1; }
    res = arrowExport(pair);          /* consumes pair */
    if (!res) { capi_err_grab(); return -1; }
    addr = (const L *)_V(res);
    *sc = (struct ArrowSchema *)(size_t)addr[0];
    *ar = (struct ArrowArray *)(size_t)addr[1];
    mr(res);
    return 0;
}

int amber_arrow_export(amber_value table, void **out_schema, void **out_array) {
    struct ArrowSchema *sc = 0;
    struct ArrowArray *ar = 0;
    if (out_schema) *out_schema = 0;
    if (out_array) *out_array = 0;
    if (capi_arrow_raw(table, &sc, &ar) != 0) return -1;
    if (out_schema) *out_schema = sc;
    if (out_array) *out_array = ar;
    return 0;
}

int amber_arrow_export_into(amber_value table, void *out_schema, void *out_array) {
    struct ArrowSchema *sc = 0;
    struct ArrowArray *ar = 0;
    if (!out_schema || !out_array) { capi_err_set("'domain: null Arrow struct"); return -1; }
    if (capi_arrow_raw(table, &sc, &ar) != 0) return -1;
    /* A "move" in the Arrow C data interface is a shallow struct copy plus
     * clearing the source's release pointer: the CONTENTS transfer, the
     * containers do not.  Doing that here is what lets a caller allocate the
     * two structs however it likes -- on its stack, inside a ctypes buffer,
     * inside a Rust struct -- instead of being forced to malloc them because
     * that is what this library happens to free. */
    memcpy(out_schema, sc, sizeof *sc);
    memcpy(out_array, ar, sizeof *ar);
    free(sc);
    free(ar);
    return 0;
}

amber_value amber_arrow_import(void *schema, void *array) {
    struct ArrowSchema *sc_src = (struct ArrowSchema *)schema, *sc;
    struct ArrowArray *ar_src = (struct ArrowArray *)array, *ar;
    A arg, pair, res;
    L addr[2];
    if (!capi_ready()) return 0;
    capi_err_clear();
    if (!schema || !array) { capi_err_set("'domain: null Arrow struct"); return 0; }
    if (!sc_src->release || !ar_src->release) {
        capi_err_set("'domain: Arrow struct has already been released or moved");
        return 0;
    }
    /* Move the caller's structs into containers this library owns, because
     * src/ar.c's arrowImport() free()s the containers it is given.  Without
     * this, importing from a stack- or ctypes-allocated struct -- which is how
     * every sane binding allocates them -- would hand a non-malloc pointer to
     * free().  The caller's structs are left marked as moved, exactly as the
     * Arrow specification requires. */
    sc = (struct ArrowSchema *)malloc(sizeof *sc);
    ar = (struct ArrowArray *)malloc(sizeof *ar);
    if (!sc || !ar) { free(sc); free(ar); capi_err_set("'limit: allocation failed"); return 0; }
    memcpy(sc, sc_src, sizeof *sc);
    memcpy(ar, ar_src, sizeof *ar);
    sc_src->release = 0;
    ar_src->release = 0;

    addr[0] = (L)(size_t)sc;
    addr[1] = (L)(size_t)ar;
    arg = aV(tL, 2, addr);
    if (!arg) { free(sc); free(ar); capi_err_set("'limit: allocation failed"); return 0; }
    pair = arrowImport(arg);              /* consumes arg, the structs and their containers */
    if (!pair) { capi_err_grab(); return 0; }
    res = K1("{+(x 0)!x 1}", pair);       /* consumes pair */
    if (!res) capi_err_grab();
    return (amber_value)res;
}

/* ---- 6.11 rendering ------------------------------------------------------ */

static char *capi_dup_charvec(A text) {
    unsigned long long len;
    char *outp;
    if (!text) return 0;
    if (_t0(text) || _t(text) != tC) { mr(text); return 0; }
    len = (unsigned long long)_n(text);
    outp = (char *)malloc((size_t)len + 1);
    if (outp) { memcpy(outp, _C(text), (size_t)len); outp[len] = 0; }
    mr(text);
    return outp;
}

char *amber_format(amber_value handle) {
    A text;
    if (!capi_ready()) return 0;
    capi_err_clear();
    if (!handle) return 0;
    /* amfmt is amber.k's box-drawing table formatter -- the same one the REPL's
     * `fmt` uses.  When the standard library was not loaded it is simply
     * undefined, the lambda raises 'value, and we fall back to `` `k ``
     * notation, which is always available because it is a core verb. */
    text = K1("{$[|/(@x)=`m`M;\"\\n\"/amfmt x;`k x]}", _R((A)handle));
    if (!text) { capi_err_clear(); text = K1("{`k x}", _R((A)handle)); }
    if (!text) { capi_err_grab(); return 0; }
    return capi_dup_charvec(text);
}

/* Strip ANSI SGR sequences in place.  Amber emits only CSI ... m (the colour
 * and attribute subset); anything else is left alone rather than guessed at. */
static void capi_strip_ansi(char *text) {
    char *readp = text, *writep = text;
    while (*readp) {
        if (readp[0] == 0x1b && readp[1] == '[') {
            const char *scan = readp + 2;
            while (*scan && ((*scan >= '0' && *scan <= '9') || *scan == ';')) scan++;
            if (*scan == 'm') { readp = (char *)scan + 1; continue; }
        }
        *writep++ = *readp++;
    }
    *writep = 0;
}

char *amber_format_plain(amber_value handle) {
    char *text = amber_format(handle);
    if (text) capi_strip_ansi(text);
    return text;
}

void amber_free(void *p) { free(p); }

/* ---- 6.12 optional dynamic plugins --------------------------------------- */

int amber_plugin_load(const char *path) {
    void *lib;
    int (*entry)(void);
    const char *why;
    capi_err_clear();
    if (!path || !*path) { capi_err_set("'domain: null plugin path"); return -1; }
    /* RTLD_GLOBAL so a plugin that itself exposes symbols (a second plugin's
     * dependency, a codec) resolves without a second dlopen.  RTLD_NOW so a
     * missing symbol is reported HERE, by name, instead of crashing the host
     * the first time an Amber script happens to reach the verb. */
    lib = dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    if (!lib) {
        why = dlerror();
        capi_err_set(why ? why : "'io: dlopen failed");
        return -1;
    }
    /* An extension may do all of its work from a constructor (that is the
     * documented pattern for the in-process seam above), in which case it needs
     * no entry point and dlopen() has already run it.  An explicit
     * amber_plugin_init() is offered for anything that must run AFTER the
     * interpreter is booted. */
    *(void **)&entry = dlsym(lib, "amber_plugin_init");
    if (entry) return entry();
    return 0;
}

#endif /* !wasm : end of the libamber.so C API */
