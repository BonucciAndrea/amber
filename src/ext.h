/* ext.h  -  Amber 1.9.5 extension seam (ABI 1).
 *
 * The engine in this repository is complete on its own: it has no AI code, no
 * network code and no optional feature that is switched off.  What it does
 * have is one small, neutral, documented seam so that an out-of-tree package
 * (for example the separate `amber-ai` repository) can add verbs, REPL
 * commands and editor behaviour by dropping .c files into `ext/` and running
 * ./build.sh again -- WITHOUT patching a single line of src/.
 *
 * That property is the whole point.  A user who installs an extension and
 * later pulls a new Amber release does not get a merge conflict, and a user
 * who never installs one carries no cost: every hook below is a null pointer
 * and every call site is a predictable, correctly-predicted branch on a code
 * path that already touches the terminal or the parser.
 *
 * ---- how an extension registers -------------------------------------------
 *
 *   #include "a.h"        // the interpreter's value type A
 *   #include "ext.h"
 *
 *   static A my_verb(A x) { ... }
 *   static int my_hint(const char *line, size_t len, char *dst, size_t cap);
 *
 *   __attribute__((constructor))
 *   static void my_register(void) {
 *       am_ext_verb("foo", (void *)my_verb);   //  `foo x   in Amber
 *       am_ext_hint = my_hint;                 //  ghost text in the editor
 *   }
 *
 * The constructor runs before main(), because ext/*.o is linked directly into
 * the executable (never as an archive member), so there is no initialisation
 * order to arrange and no registration call to add to src/.
 *
 * ---- ABI ------------------------------------------------------------------
 *
 * AMBER_EXT_ABI is bumped whenever an existing hook changes shape.  An
 * installer should check it (`grep AMBER_EXT_ABI src/ext.h`) before copying
 * files in.  Adding a NEW hook does not bump it.
 *
 * Amber - GNU AGPLv3 - see LICENSE and NOTICE.
 */
#ifndef AMBER_EXT_H
#define AMBER_EXT_H

#include <stddef.h>

#define AMBER_EXT_ABI 1

#ifdef __cplusplus
extern "C" {
#endif

/* AMBER_API: default visibility for the symbols that make up the public
 * surface.  The shared build compiles with -fvisibility=hidden so that the
 * engine's ~1,500 terse internal globals never reach a host process's dynamic
 * namespace even by accident; that makes marking the intended exports
 * mandatory rather than optional, and marking them HERE means the declaration
 * and the definition can never disagree.  In the static build the macro expands
 * to nothing and ./amber is byte-for-byte what it was. */
#if defined(AMBER_SHARED) && (defined(__GNUC__) || defined(__clang__))
#define AMBER_API __attribute__((visibility("default")))
#else
#define AMBER_API
#endif


/* ---- 1. verb registry ----------------------------------------------------
 *
 * Amber's backtick verbs (`t, `hex, `simd, ...) live in a fixed table in
 * src/a.c that is indexed by the symbol's own 4-byte packed value.  An
 * extension cannot edit that table, so sym1() consults this registry FIRST and
 * falls through to the built-in table when nothing is registered.  Names are
 * therefore at most four characters, exactly like the built-ins, and a
 * registered name shadows a built-in of the same spelling (which is how an
 * extension may replace behaviour rather than only add to it).
 *
 * `fn` must have the interpreter's monadic signature, A (*)(A), i.e. it takes
 * one reference and consumes it.  It is passed as void* purely so that this
 * header stays free of src/a.h -- extensions include both anyway.
 */
AMBER_API int   am_ext_verb(const char *name, void *fn);   /* 0 = registered, -1 = no */
AMBER_API void *am_ext_verb_lookup(int packed);            /* used by src/a.c sym1()  */
AMBER_API int   am_ext_verb_count(void);

/* ---- 2. startup ----------------------------------------------------------
 * Called once, lazily, the first time the REPL reads a line (am_repl_init()).
 * An extension uses it for whatever it must not do from a constructor -- read
 * the environment, open a file, warm a cache.
 */
AMBER_API extern void (*am_ext_startup)(void);
AMBER_API void am_ext_startup_once(void);                  /* idempotent              */

/* ---- 3. line editor ------------------------------------------------------
 *
 * am_ext_hint: offered a line and asked for an inline continuation.  Write at
 *   most cap-1 bytes plus NUL into dst and return 1; return 0 for "nothing".
 *   The editor calls this ONLY when the cursor is at end of line and every
 *   lexical completion source has already come up empty, and it renders the
 *   result as dim ghost text that is never inserted until the user explicitly
 *   accepts it with Tab / Right / Ctrl-F.  It MUST return quickly: it runs on
 *   the keystroke path, so anything that can block belongs behind a deadline
 *   inside the extension.
 *
 * am_ext_complete / am_ext_complete_late: extra sources of ordinary Tab
 *   candidates.  Completion layers, so there are two of them and the order is
 *   the point:
 *
 *     am_ext_complete       runs BEFORE the built-in lexical sources, and is
 *                           for syntax the engine cannot know about -- an
 *                           extension's own `\` command and its sub-commands.
 *                           If it produces candidates, nothing else is asked.
 *     am_ext_complete_late  runs AFTER globals, columns, vocabulary and
 *                           history have all come up empty, and before the
 *                           hint below.  It is for wide, fuzzy sources (a
 *                           recall file, a project index) that must never
 *                           shadow the name of a variable actually in scope.
 *
 *   `completions` is an amCompletions* (src/ln.h); add to it with
 *   am_ln_add_completion().  void* again only to keep this header standalone.
 */
AMBER_API extern int  (*am_ext_hint)(const char *line, size_t len, char *dst, size_t cap);
AMBER_API extern void (*am_ext_complete)(const char *line, void *completions);
AMBER_API extern void (*am_ext_complete_late)(const char *line, void *completions);

/* The line as it stood when the user ACCEPTED a ghost suggestion, or "" when
 * there is none.  Reading it clears it, so an extension can drain it once per
 * line and treat the acceptance as feedback. */
AMBER_API void        am_ext_set_accepted(const char *line);
AMBER_API const char *am_repl_take_accepted(void);

/* ---- 4. REPL commands ----------------------------------------------------
 * Consulted by src/m.c's backslash dispatcher BEFORE the "run it as a shell
 * command" fallback, with the text after the backslash (e.g. "ai why x").
 * Return 0 for "not mine, carry on"; otherwise return an Amber value (an `A`,
 * typed here as unsigned long long so this header needs no src/a.h) -- use the
 * generic null `au` when the command has only printed something.
 */
AMBER_API extern unsigned long long (*am_ext_bs)(const char *line);

/* ---- 5. cosmetics --------------------------------------------------------
 * Appended to `amber --help` (am_ext_usage) and shown in the REPL banner
 * (am_ext_banner, e.g. " [amber-ai 1.0.0]").  Both NULL by default; both must
 * point at storage that outlives the process, i.e. a string literal.
 */
AMBER_API extern const char *am_ext_usage;
AMBER_API extern const char *am_ext_banner;


/* ===========================================================================
 * 6. DYNAMIC C API  --  the `libamber.so` seam (ABI AMBER_CAPI_ABI)
 * ===========================================================================
 *
 * Everything above this line is the IN-PROCESS seam: an out-of-tree .c file
 * dropped into ext/ and linked straight into ./amber.  Everything below is the
 * OUT-OF-PROCESS seam: a stable, plain-C surface that a satellite project
 * links against as a shared library and calls from another language.
 *
 *   ./build.sh --shared        ->  libamber.so  (libamber.dylib on macOS)
 *
 * The two seams exist for the same reason and obey the same rule: the core
 * engine must never grow a dependency on its consumers.  Nothing below
 * includes -- or requires the caller to have included -- Python.h, an Arrow
 * header, a gRPC header or anything else from a satellite's world.  Every type
 * that crosses this boundary is a C99 builtin or an opaque 64-bit handle, so
 * `libamber.so` compiled today keeps working with a satellite compiled against
 * a different toolchain, a different language runtime, or a different decade.
 *
 * ---- the handle ----------------------------------------------------------
 *
 * `amber_value` is the interpreter's own `A` (see src/a.h), re-typed here as an
 * unsigned 64-bit integer so this header stays free of a.h.  It is either a
 * tagged immediate (small int, char, symbol, ...) or a pointer to a
 * refcounted heap object.  Callers MUST treat it as opaque and MUST NOT
 * dereference it -- use amber_get_vector_ptr() and the accessors below.
 *
 * ---- ownership -----------------------------------------------------------
 *
 * Functions that RETURN an `amber_value` return a value the caller OWNS and
 * must eventually pass to amber_release() -- exactly once.  Functions that
 * TAKE an `amber_value` borrow it: they never consume the caller's reference.
 * amber_release() on a tagged immediate is a documented no-op, so a caller can
 * release unconditionally without first asking what it is holding.
 *
 * ---- threads -------------------------------------------------------------
 *
 * The interpreter is single-threaded at the API boundary (it manages its own
 * worker pool internally for `peach`).  Call this API from ONE thread, or
 * serialise access yourself.  amber_last_error() is thread-local.
 *
 * Amber - GNU AGPLv3 - see LICENSE and NOTICE.
 */

#define AMBER_CAPI_ABI 1

typedef unsigned long long amber_value;

/* Type codes: the interpreter's own type tags (src/a.h `enum{tA=1,...}`),
 * surfaced here as named constants so a satellite never has to hard-code an
 * integer.  A value's type never changes, and these numbers are part of the
 * ABI. */
enum {
    AMBER_T_LIST      = 1,   /* general list                (`A)  */
    AMBER_T_RANGE     = 2,   /* lazy integer range          (`I)  -- see amber_flatten */
    AMBER_T_BOOL      = 3,   /* bit-packed boolean vector   (`I)  -- 1 bit/element */
    AMBER_T_BYTE      = 4,   /* 8-bit vector                (`I)  */
    AMBER_T_SHORT     = 5,   /* 16-bit vector               (`I)  */
    AMBER_T_INT       = 6,   /* 32-bit vector               (`I)  */
    AMBER_T_LONG      = 7,   /* 64-bit vector               (`I)  */
    AMBER_T_FLOAT     = 8,   /* 64-bit IEEE-754 vector      (`F)  */
    AMBER_T_CHAR      = 9,   /* char vector / string        (`C)  */
    AMBER_T_SYM       = 10,  /* symbol vector (int32 ids)   (`S)  -- see amber_symbol_name */
    AMBER_T_TABLE     = 11,  /* table                       (`M)  */
    AMBER_T_DICT      = 12,  /* dictionary                  (`m)  */
    AMBER_T_INT_ATOM  = 13,
    AMBER_T_LONG_ATOM = 14,
    AMBER_T_FLOAT_ATOM= 15,
    AMBER_T_CHAR_ATOM = 16,
    AMBER_T_SYM_ATOM  = 17,
    AMBER_T_DATE      = 26,  /* days since 2000.01.01              */
    AMBER_T_TIME      = 27,  /* milliseconds of day                */
    AMBER_T_TIMESTAMP = 28   /* nanoseconds since 2000.01.01       */
};

/* ---- 6.1 identity -------------------------------------------------------- */

/* Version of THIS API surface.  Bumped only when an existing entry point
 * changes shape; adding a new one does not bump it.  A satellite should check
 * it once at load time. */
AMBER_API int         amber_abi_version(void);

/* The interpreter release, e.g. "1.9.5" -- the same AMBER_VERSION the REPL
 * banner and `amber --version` report.  Static storage; do not free. */
AMBER_API const char *amber_version_string(void);

/* ---- 6.2 lifecycle ------------------------------------------------------- */

/* Boot the interpreter.  Idempotent: the second and later calls are no-ops and
 * still return 0.
 *
 * `home` is the directory holding the Amber standard-library .k modules
 * (amber.k, fin.k, std.k, qsql.k, temporal.k, sys.k, hdb.k, ipc.k).  Pass NULL
 * to boot the BARE engine with no standard library -- primitives only, exactly
 * what `./amber` with no script gives you.  Pass a directory to get the full
 * vocabulary, exactly what `./a` gives you.  The modules are loaded with
 * stdout redirected to /dev/null for the duration, so a host process's own
 * output stream is never polluted by the load.
 *
 * Returns 0 on success, -1 if `home` was given but no module could be read
 * (amber_last_error() then says which). */
AMBER_API int  amber_init(const char *home);

/* Release the interpreter's scratch arena.  The object heap itself is mmap'd
 * for process lifetime by design and is NOT returned -- Amber has no teardown
 * path and never needed one.  Safe to call more than once; after it, calling
 * amber_init() again re-arms the arena. */
AMBER_API void amber_shutdown(void);

/* ---- 6.3 evaluation ------------------------------------------------------ */

/* Evaluate `src` -- one or more newline-separated Amber statements -- and
 * return the value of the LAST one.  Nothing is printed: this is the library
 * entry point, not the REPL's.
 *
 * Returns 0 (a value no other call can produce) on a parse, compile or runtime
 * error; amber_last_error() then holds the interpreter's own error text. */
AMBER_API amber_value amber_eval_str(const char *src);

/* Apply the global function named `fname` to `argc` arguments (0..8).  This is
 * the fast path a language binding wants for `am.gentq(10000)`: no string
 * formatting, no re-parse, and arguments cross as native values rather than as
 * printed text.  Falls back to an error if `fname` is not defined or is not
 * applicable.  Returns 0 on error. */
AMBER_API amber_value amber_call(const char *fname, const amber_value *args, int argc);

/* Evaluate ONE line after first running it through qsql.k's bare-qSQL rewriter
 * (`qrw`), so that
 *
 *     select vwap:wavg[sz;px] by sym from trades where px>0
 *
 * works verbatim -- with no sel"..." wrapper -- exactly as it does when typed
 * at the `amber>` prompt.
 *
 * This is a SEPARATE entry point rather than a change to amber_eval_str()
 * because the rewrite is a surface-syntax convenience, not part of the
 * language: repl.k applies it to interactive lines only, and script execution
 * (bsl) deliberately does not see it.  A satellite whose users type queries --
 * the Grafana datasource, the LSP, a notebook cell -- wants this one; a
 * satellite that evaluates generated Amber code wants amber_eval_str().
 *
 * Falls back to plain evaluation when the standard library (and therefore
 * `qrw`) was not loaded.  Returns 0 on error. */
AMBER_API amber_value amber_eval_qsql(const char *line);

/* The error text left by the most recent failing call, or "" if the most
 * recent call succeeded.  Thread-local, static storage, valid until the next
 * call on this thread; do not free. */
AMBER_API const char *amber_last_error(void);

/* Turn the interpreter's rich Rust-style stderr diagnostic on (1) or off (0),
 * returning the previous setting.  A library host normally wants it OFF -- the
 * error text is delivered through amber_last_error() and turning it into an
 * exception is the host's job, not stderr's.  Equivalent to the `` `diag ``
 * verb and to $AMBER_DIAG. */
AMBER_API int amber_set_diagnostics(int on);

/* ---- 6.4 ownership ------------------------------------------------------- */

AMBER_API amber_value amber_retain(amber_value handle);   /* +1 ref; returns `handle`   */
AMBER_API void        amber_release(amber_value handle);  /* -1 ref; no-op on immediates*/

/* ---- 6.5 introspection --------------------------------------------------- */

AMBER_API int         amber_type(amber_value handle);        /* one of AMBER_T_*        */
AMBER_API const char *amber_type_name(amber_value handle);   /* "long vector", ...      */
AMBER_API long long   amber_count(amber_value handle);       /* `#x` -- rows for a table*/
AMBER_API int         amber_is_table(amber_value handle);
AMBER_API int         amber_is_dict(amber_value handle);
AMBER_API int         amber_is_atom(amber_value handle);

/* ---- 6.6 flat vectors (the zero-copy seam) ------------------------------- */

/* Hand back the raw payload pointer of a flat vector so a consumer can wrap it
 * WITHOUT copying: this is what makes `python-amber` -> NumPy and
 * `amber-arrow` -> Arrow zero-copy.
 *
 *   out_type      receives the AMBER_T_* code
 *   out_len       receives the element count
 *   out_elem_bits receives the element width IN BITS (1 for AMBER_T_BOOL,
 *                 which is genuinely bit-packed; 8/16/32/64 otherwise)
 *
 * Any of the three out-parameters may be NULL.
 *
 * Returns NULL -- with the out-parameters still filled in -- when the value has
 * no flat payload to point at: an atom, a general list, a table, a dict, a
 * function, or a lazy AMBER_T_RANGE.  For a range, call amber_flatten() first.
 *
 * LIFETIME: the pointer is owned by the Amber value.  It stays valid exactly as
 * long as the caller holds a reference (see amber_retain/amber_release), and
 * becomes dangling the instant the last reference goes.  A consumer that
 * publishes this pointer into another runtime (a NumPy array, an ArrowArray)
 * MUST keep a retained handle alive for as long as that object can be read. */
AMBER_API const void *amber_get_vector_ptr(amber_value handle,
                                 int *out_type,
                                 long long *out_len,
                                 int *out_elem_bits);

/* Materialise a lazy AMBER_T_RANGE into a real vector so it has a payload to
 * point at.  Any other value is returned retained and unchanged, so this is
 * always safe to call.  Returns an OWNED handle. */
AMBER_API amber_value amber_flatten(amber_value handle);

/* The interned text of a symbol id -- an element of an AMBER_T_SYM vector's
 * int32 payload, or amber_to_symbol_id() of an AMBER_T_SYM_ATOM.  Static
 * storage owned by the interpreter's symbol table; never freed, do not free. */
AMBER_API const char *amber_symbol_name(int symbol_id);

/* ---- 6.7 scalars --------------------------------------------------------- */

/* Each sets *ok (may be NULL) to 1 on success, 0 if the value is not of a
 * convertible type, and returns a zero/empty result in that case. */
AMBER_API long long amber_to_int(amber_value handle, int *ok);
AMBER_API double    amber_to_float(amber_value handle, int *ok);
AMBER_API int       amber_to_symbol_id(amber_value handle, int *ok);

/* Copy an AMBER_T_CHAR vector (or AMBER_T_CHAR_ATOM) into `dst` as a
 * NUL-terminated string.  Returns the number of bytes the string needs
 * EXCLUDING the NUL, or -1 if the value is not char-shaped -- so a caller can
 * size a buffer by calling once with dst=NULL,cap=0. */
AMBER_API long long amber_to_string(amber_value handle, char *dst, long long cap);

/* ---- 6.8 tables and dictionaries ----------------------------------------- */

AMBER_API int         amber_table_ncols(amber_value table);              /* -1 if not a table */
AMBER_API const char *amber_table_colname(amber_value table, int col);   /* NULL if out of range */
AMBER_API amber_value amber_table_column(amber_value table, int col);    /* OWNED, flattened */
AMBER_API amber_value amber_dict_keys(amber_value dict);                 /* OWNED */
AMBER_API amber_value amber_dict_values(amber_value dict);               /* OWNED */

/* ---- 6.9 pushing data back in -------------------------------------------- */
/* Each copies `n` elements out of the caller's buffer into a fresh Amber
 * vector and returns an OWNED handle (0 on allocation failure). */
/* Scalar constructors.  These make ATOMS, not one-element vectors: `amber_int(5)`
 * is `5`, whereas `amber_from_int64(&five,1)` is `,5`.  The distinction is not
 * cosmetic in an array language -- `f[5]` and `f[,5]` can do different things --
 * so a binding that converts a Python `int` needs the atom, and one that
 * converts a Python `[5]` needs the vector. */
AMBER_API amber_value amber_int(long long value);
AMBER_API amber_value amber_float(double value);
AMBER_API amber_value amber_symbol(const char *name);
AMBER_API amber_value amber_null(void);          /* the generic null, `::` */

/* A general list of `n` borrowed values, squeezed: a list whose elements share
 * one atomic type collapses to the corresponding typed vector, exactly as the
 * parser would have produced for the same literal. */
AMBER_API amber_value amber_make_list(const amber_value *items, long long n);

AMBER_API amber_value amber_from_int32  (const int *src,       long long n);
AMBER_API amber_value amber_from_int64  (const long long *src, long long n);
AMBER_API amber_value amber_from_float64(const double *src,    long long n);
AMBER_API amber_value amber_from_bytes  (const char *src,      long long n);  /* -> char vector */
AMBER_API amber_value amber_from_symbols(const char *const *src, long long n);

/* Build a table from `ncols` named columns.  The columns are borrowed. */
AMBER_API amber_value amber_make_table(const char *const *names,
                             const amber_value *cols,
                             int ncols);

/* Build a dictionary from `n` symbol keys and `n` borrowed values. */
AMBER_API amber_value amber_make_dict(const char *const *keys,
                            const amber_value *vals,
                            int n);

/* Bind / read a global by name.  amber_set_global() borrows `v` and returns 0
 * on success; amber_get_global() returns an OWNED handle, or 0 if undefined. */
AMBER_API int         amber_set_global(const char *name, amber_value v);
AMBER_API amber_value amber_get_global(const char *name);

/* ---- 6.10 Apache Arrow C Data Interface ---------------------------------- */
/* Thin, header-free access to the exporter already in src/ar.c.  The out-
 * parameters receive `struct ArrowSchema *` and `struct ArrowArray *` as plain
 * void* so that NOTHING in this header, or in the core build, needs Arrow's
 * abi.h.  The satellite that consumes them supplies its own declarations --
 * that is the entire point of a stable C data interface.
 *
 * Numeric columns are exported ZERO-COPY: the ArrowArray's data buffer aliases
 * the Amber column's payload, and the release callback drops the Amber
 * refcount.  Symbol columns are materialised to utf8 (offsets + data), which
 * necessarily copies.
 *
 * Returns 0 on success, -1 on error. */
/* Export a table, returning two heap-allocated Arrow structs.
 *
 * OWNERSHIP, and this one has caught people out: the CONTENTS of the two structs
 * are released by whoever ends up owning them -- the consumer that imports them,
 * or their own `release` callback -- but the two CONTAINERS are malloc'd here and
 * become the caller's. After the contents have been released or moved away, the
 * caller must pass both pointers to amber_free().
 *
 * That is the Arrow C data interface's rule, not an Amber quirk: a "move"
 * transfers contents, never containers. It is still a rule that is easy to read
 * past, which is why amber_arrow_export_into() below exists and why it is the
 * one to prefer -- it leaves nothing for anyone to free. */
AMBER_API int         amber_arrow_export(amber_value table, void **out_schema, void **out_array);

/* As amber_arrow_export(), but MOVES the result into two structs the CALLER has
 * allocated (each must be at least the size of ArrowSchema / ArrowArray as
 * defined by the Arrow C data interface).  Prefer this one: it leaves nothing
 * for the caller to free, so a binding can allocate the structs on its stack,
 * in a ctypes buffer, or inside one of its own objects.
 *
 * Returns 0 on success, -1 on error. */
AMBER_API int         amber_arrow_export_into(amber_value table, void *out_schema, void *out_array);

/* Import an Arrow struct pair as an Amber table.  MOVES the two structs in the
 * Arrow sense -- their contents are taken and their `release` pointers cleared,
 * so the caller keeps ownership of the two containers and may free them (or let
 * them go out of scope) immediately afterwards.  Returns an OWNED handle, or 0
 * on error, including when either struct has already been moved or released. */
AMBER_API amber_value amber_arrow_import(void *schema, void *array);

/* ---- 6.11 rendering ------------------------------------------------------ */

/* Render `handle` the way the REPL would -- box-drawn for a table when the
 * standard library is loaded, `` `k `` notation otherwise.  Returns a
 * malloc'd NUL-terminated string the caller frees with amber_free(), or NULL
 * on error. */
AMBER_API char *amber_format(amber_value handle);

/* As amber_format(), with every ANSI SGR escape sequence removed.  Amber's
 * table formatter colours its output unconditionally (see CT/PAL in amber.k),
 * which is right for a terminal and wrong for a notebook cell, an LSP hover
 * tooltip, a JSON payload or a log file.  Rather than make every satellite
 * carry its own escape-stripping regex, strip it once, here. */
AMBER_API char *amber_format_plain(amber_value handle);
AMBER_API void  amber_free(void *p);

/* ---- 6.12 optional dynamic plugins --------------------------------------- */

/* dlopen() `path` and call its `int amber_plugin_init(void)` entry point, if
 * it has one.  This is how a satellite adds native verbs to a RUNNING
 * interpreter without being compiled into it and without the core build ever
 * seeing that satellite's headers: the plugin links against libamber.so, is
 * built entirely out of tree, and registers itself through am_ext_verb() above.
 *
 * Returns 0 on success, -1 on failure (amber_last_error() has the dlerror). */
AMBER_API int amber_plugin_load(const char *path);

#ifdef __cplusplus
}
#endif

#endif /* AMBER_EXT_H */
