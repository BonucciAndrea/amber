/* tests/test_capi.c  -  exercise the dynamic C API seam (src/ext.h section 6).
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * This is the ONLY consumer of libamber.so that lives inside the core
 * repository, and it is deliberately written the way a satellite would write
 * it: it includes src/ext.h and NOTHING else from src/, it never dereferences
 * an amber_value, and it links against the shared library rather than against
 * the object files.  If this file needs an internal header to do its job, the
 * API is wrong.
 *
 * Every case that produces a value releases it, so the whole run is expected to
 * be clean under -fsanitize=address with detect_leaks=1.  That is the point:
 * a C API whose ownership rules are only documented is a C API whose ownership
 * rules are wrong, and LeakSanitizer is the thing that checks the prose.
 *
 * Build + run:  tests/test_capi.sh          (plain, then ASan+UBSan)
 */
#include "ext.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static int failures, checks;

static void ck(int cond, const char *what) {
    checks++;
    if (!cond) { failures++; printf("  FAIL  %s   [%s]\n", what, amber_last_error()); }
}

#define CK(cond) ck((cond), #cond)

/* ---- 1. identity and boot ------------------------------------------------ */
static void t_boot(const char *home) {
    CK(amber_abi_version() == AMBER_CAPI_ABI);
    CK(amber_version_string() != 0 && amber_version_string()[0] == '1');
    CK(amber_init(home) == 0);
    CK(amber_init(home) == 0);          /* idempotent */
    amber_set_diagnostics(0);           /* library mode: errors via the API */
}

/* ---- 2. evaluation, errors, ownership ----------------------------------- */
static void t_eval(void) {
    amber_value v = amber_eval_str("2+3");
    int ok = 0;
    CK(v != 0);
    CK(amber_type(v) == AMBER_T_LONG_ATOM || amber_type(v) == AMBER_T_INT_ATOM);
    CK(amber_to_int(v, &ok) == 5 && ok);
    amber_release(v);

    /* a genuine error must return 0 AND leave a readable message */
    v = amber_eval_str("nosuchvariable+1");
    CK(v == 0);
    CK(strlen(amber_last_error()) > 0);
    CK(strstr(amber_last_error(), "value") != 0);

    /* ... and the next successful call must clear it */
    v = amber_eval_str("1");
    CK(v != 0 && amber_last_error()[0] == 0);
    amber_release(v);

    /* multi-statement source returns the LAST value */
    v = amber_eval_str("a:10\nb:32\na+b");
    CK(v != 0);
    CK(amber_to_int(v, &ok) == 42 && ok);
    amber_release(v);

    /* releasing a tagged immediate twice is a documented no-op, not a crash */
    v = amber_eval_str("7");
    amber_release(v);
    amber_release(0);
}

/* ---- 3. flat vectors: the zero-copy seam -------------------------------- */
static void t_vectors(void) {
    amber_value v;
    const void *p;
    int tp = 0, bits = 0;
    long long n = 0;

    /* An integer vector: Amber stores integers in the NARROWEST width that
     * holds every element (see tZ() in src/m.c), so `1 2 3 4 5` is physically
     * an 8-bit vector even though it is logically a long vector.  A binding
     * must switch on the reported type, never assume 64-bit -- which is exactly
     * what this case pins down. */
    v = amber_eval_str("1 2 3 4 5");
    CK(v != 0);
    CK(amber_count(v) == 5);
    p = amber_get_vector_ptr(v, &tp, &n, &bits);
    CK(p != 0 && n == 5);
    CK(tp == AMBER_T_BYTE || tp == AMBER_T_SHORT ||
       tp == AMBER_T_INT  || tp == AMBER_T_LONG);
    CK(bits == 8 || bits == 16 || bits == 32 || bits == 64);
    if (p) switch (bits) {
        case 8:  CK(((const signed char *)p)[4] == 5); break;
        case 16: CK(((const short *)p)[4] == 5); break;
        case 32: CK(((const int *)p)[4] == 5); break;
        default: CK(((const long long *)p)[4] == 5); break;
    }
    amber_release(v);

    /* ... and a value that does not fit 32 bits really is stored 64-bit wide */
    v = amber_eval_str("1 2 3000000000");
    p = amber_get_vector_ptr(v, &tp, &n, &bits);
    CK(p != 0 && tp == AMBER_T_LONG && n == 3 && bits == 64);
    if (p) CK(((const long long *)p)[2] == 3000000000LL);
    amber_release(v);

    /* float64 vector */
    v = amber_eval_str("1.5 2.5 3.5");
    p = amber_get_vector_ptr(v, &tp, &n, &bits);
    CK(p != 0 && tp == AMBER_T_FLOAT && n == 3 && bits == 64);
    if (p) CK(fabs(((const double *)p)[1] - 2.5) < 1e-12);
    amber_release(v);

    /* char vector, and the size-probe protocol */
    v = amber_eval_str("\"hello\"");
    CK(amber_to_string(v, 0, 0) == 5);
    {
        char buf[16];
        CK(amber_to_string(v, buf, sizeof buf) == 5);
        CK(strcmp(buf, "hello") == 0);
    }
    amber_release(v);

    /* a lazy range has no payload until it is flattened */
    v = amber_eval_str("!10");
    p = amber_get_vector_ptr(v, &tp, &n, &bits);
    if (tp == AMBER_T_RANGE) {
        amber_value dense;
        CK(p == 0);                       /* nothing to alias yet */
        dense = amber_flatten(v);
        CK(dense != 0);
        p = amber_get_vector_ptr(dense, &tp, &n, &bits);
        CK(p != 0 && n == 10);
        amber_release(dense);
    } else {
        CK(p != 0 && n == 10);            /* already materialised: also fine */
    }
    amber_release(v);

    /* an atom has no payload and MUST NOT be dereferenced for one */
    v = amber_eval_str("42");
    p = amber_get_vector_ptr(v, &tp, &n, &bits);
    CK(p == 0);
    amber_release(v);

    /* symbol vector: int32 ids resolvable back to text */
    v = amber_eval_str("`aapl`msft`goog");
    p = amber_get_vector_ptr(v, &tp, &n, &bits);
    CK(p != 0 && tp == AMBER_T_SYM && n == 3 && bits == 32);
    if (p) CK(strcmp(amber_symbol_name(((const int *)p)[1]), "msft") == 0);
    amber_release(v);
}

/* ---- 4. tables ----------------------------------------------------------- */
static void t_tables(void) {
    amber_value t = amber_eval_str("([]sym:`a`b`c; px:1.5 2.5 3.5; sz:10 20 30)");
    amber_value col;
    const void *p;
    int tp = 0, bits = 0;
    long long n = 0;

    CK(t != 0);
    CK(amber_is_table(t));
    CK(amber_count(t) == 3);
    CK(amber_table_ncols(t) == 3);
    CK(amber_table_colname(t, 0) && strcmp(amber_table_colname(t, 0), "sym") == 0);
    CK(amber_table_colname(t, 2) && strcmp(amber_table_colname(t, 2), "sz") == 0);
    CK(amber_table_colname(t, 9) == 0);

    col = amber_table_column(t, 1);
    CK(col != 0);
    p = amber_get_vector_ptr(col, &tp, &n, &bits);
    CK(p != 0 && tp == AMBER_T_FLOAT && n == 3);
    if (p) CK(fabs(((const double *)p)[2] - 3.5) < 1e-12);
    amber_release(col);

    CK(amber_table_column(t, 42) == 0);
    amber_release(t);
}

/* ---- 5. pushing data back in -------------------------------------------- */
static void t_push(void) {
    static const long long src[4] = { 11, 22, 33, 44 };
    static const double px[4] = { 1.0, 2.0, 3.0, 4.0 };
    static const char *const names[2] = { "qty", "px" };
    amber_value cols[2], tab, res;
    int ok = 0;

    cols[0] = amber_from_int64(src, 4);
    cols[1] = amber_from_float64(px, 4);
    CK(cols[0] != 0 && cols[1] != 0);

    tab = amber_make_table(names, cols, 2);
    CK(tab != 0);
    CK(amber_is_table(tab));
    CK(amber_count(tab) == 4);
    amber_release(cols[0]);
    amber_release(cols[1]);

    CK(amber_set_global("ttab", tab) == 0);
    amber_release(tab);

    res = amber_eval_str("sum ttab`qty");
    CK(res != 0);
    CK(amber_to_int(res, &ok) == 110 && ok);
    amber_release(res);

    res = amber_get_global("ttab");
    CK(res != 0 && amber_is_table(res));
    amber_release(res);

    res = amber_get_global("definitelyNotDefined");
    CK(res == 0);
}

/* ---- 6. calling a function by name -------------------------------------- */
static void t_call(void) {
    amber_value arg = amber_eval_str("3 1 2");
    amber_value res = amber_call("asc", &arg, 1);
    const void *p;
    int tp = 0, bits = 0;
    long long n = 0;
    CK(arg != 0);
    /* `asc` is a core verb, not a global, so this may legitimately be a 'value
     * error -- what must NOT happen is a crash or a lost reference. */
    if (res) {
        p = amber_get_vector_ptr(res, &tp, &n, &bits);
        CK(n == 3);
        amber_release(res);
    } else {
        CK(strlen(amber_last_error()) > 0);
    }
    amber_release(arg);

    /* a user-defined global function is the real target of amber_call */
    res = amber_eval_str("dbl:{2*x}");
    if (res) amber_release(res);
    {
        amber_value a2 = amber_eval_str("21");
        amber_value r2 = amber_call("dbl", &a2, 1);
        int ok = 0;
        CK(r2 != 0);
        CK(amber_to_int(r2, &ok) == 42 && ok);
        amber_release(r2);
        amber_release(a2);
    }
    /* out-of-range arity is rejected, not undefined behaviour */
    CK(amber_call("dbl", 0, 99) == 0);
    CK(amber_call(0, 0, 0) == 0);
}

/* ---- 7. Arrow C Data Interface ------------------------------------------ */
/* Declared locally, exactly as a satellite would: the core never ships an
 * Arrow header and this test does not use one either. */
struct TestArrowSchema {
    const char *format, *name, *metadata;
    long long flags, n_children;
    struct TestArrowSchema **children, *dictionary;
    void (*release)(struct TestArrowSchema *);
    void *private_data;
};
struct TestArrowArray {
    long long length, null_count, offset, n_buffers, n_children;
    const void **buffers;
    struct TestArrowArray **children, *dictionary;
    void (*release)(struct TestArrowArray *);
    void *private_data;
};

static void t_arrow(void) {
    amber_value t = amber_eval_str("([]a:1 2 3; b:1.5 2.5 3.5)");
    void *schema = 0, *array = 0;
    CK(t != 0);
    CK(amber_arrow_export(t, &schema, &array) == 0);
    CK(schema != 0 && array != 0);
    if (schema && array) {
        struct TestArrowSchema *sc = (struct TestArrowSchema *)schema;
        struct TestArrowArray *ar = (struct TestArrowArray *)array;
        amber_value back;
        CK(sc->n_children == 2);
        CK(ar->length == 3);
        CK(sc->format && strcmp(sc->format, "+s") == 0);
        CK(sc->children[0]->name && strcmp(sc->children[0]->name, "a") == 0);
        /* zero-copy: the Arrow data buffer must BE the Amber column payload */
        {
            amber_value col = amber_table_column(t, 0);
            const void *p = amber_get_vector_ptr(col, 0, 0, 0);
            CK(p != 0 && p == ar->children[0]->buffers[1]);
            amber_release(col);
        }
        /* Round-trip. amber_arrow_import() MOVES the two structs: it takes their
         * contents and clears their release pointers, which leaves the two
         * CONTAINERS -- malloc'd by amber_arrow_export() -- as ours to free.
         *
         * This is exactly the leak LeakSanitizer caught the first time this test
         * was run under it: 152 bytes, once per export, invisible in any normal
         * run and fatal in a Flight server that exports a table per request. It
         * is why the sanitizer leg exists. */
        back = amber_arrow_import(schema, array);
        CK(back != 0);
        if (back) {
            CK(amber_is_table(back));
            CK(amber_count(back) == 3);
            CK(amber_table_ncols(back) == 2);
            amber_release(back);
        }
        amber_free(schema);
        amber_free(array);
    }
    /* The caller-allocates form: the engine MOVES its export into structs we own,
     * so there is nothing left to free and no way to get the ownership wrong.
     * This is the one a binding should use. */
    {
        struct TestArrowSchema sc;
        struct TestArrowArray ar;
        memset(&sc, 0, sizeof sc);
        memset(&ar, 0, sizeof ar);
        CK(amber_arrow_export_into(t, &sc, &ar) == 0);
        CK(sc.n_children == 2 && ar.length == 3 && sc.release != 0);
        if (sc.release) sc.release(&sc);
        if (ar.release) ar.release(&ar);
        CK(amber_arrow_export_into(t, 0, 0) == -1);
    }

    amber_release(t);
    CK(amber_arrow_export(0, &schema, &array) == -1);
    CK(amber_arrow_import(0, 0) == 0);
}

/* ---- 8. rendering -------------------------------------------------------- */
static void t_format(void) {
    amber_value v = amber_eval_str("1 2 3");
    char *text = amber_format(v);
    CK(text != 0);
    if (text) { CK(strchr(text, '1') != 0); amber_free(text); }
    amber_release(v);
    CK(amber_format(0) == 0);
}

/* ---- 9. plugin loading --------------------------------------------------- */
static void t_plugin(void) {
    /* A path that cannot resolve must report, not crash. */
    CK(amber_plugin_load("/nonexistent/definitely-not-a-plugin.so") == -1);
    CK(strlen(amber_last_error()) > 0);
    CK(amber_plugin_load(0) == -1);
}

int main(int argc, char **argv) {
    const char *home = argc > 1 ? argv[1] : ".";
    printf("== libamber.so C API (%s, ABI %d) ==\n",
           amber_version_string(), amber_abi_version());
    t_boot(home);
    t_eval();
    t_vectors();
    t_tables();
    t_push();
    t_call();
    t_arrow();
    t_format();
    t_plugin();
    amber_shutdown();
    printf("%d checks, %d failures\n", checks, failures);
    printf(failures ? "CAPI TESTS FAILED\n" : "ALL CAPI TESTS PASSED\n");
    return failures ? 1 : 0;
}
