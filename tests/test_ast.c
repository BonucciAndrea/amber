/* test_ast.c  -  standalone test for src/ast.c's \ast tree formatter.
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * Unlike tests/test_simd.c and tests/test_parallel.c (each deliberately
 * dependency-free from the rest of the interpreter),
 * ast.c is inherently built on Amber's own parser and value representation
 * (pk(), su(), the tu/tv/tw/to/tS/... type tags) -- there is no meaningful
 * "AST of an expression" without Amber's real parser producing it. So this
 * test links against the WHOLE interpreter (every src object file, with
 * 0.c compiled -Dldstatic -- the guard 0.c already wraps main() in -- so it
 * still supplies `pg` and the non-wasm js_eval() stub but no main()) and calls kinit() itself before touching pk()/
 * ast_cmd(), the same startup 0.c's own main() does.
 *
 * It captures ast_cmd()'s stdout (it prints straight to stdout, like
 * \disasm/\hl) into a real temp file the same way ast_selftest() does
 * in-process, then asserts on substrings of the printed tree -- proving the
 * historical "<v-atom>"/"<w-atom>"/"<o-atom>"/"<I-atom>"/"<S-atom>"
 * placeholder bugs (see ast.c's file header) are gone, that hooks/forks/
 * lambdas/curried-projections/blank-argument-slots render with their own
 * explicit labels, that the literal-vector preview and the Symbol-Vector
 * packed-id read fix both work, and that ANSI color codes for all five
 * required categories (verb/adverb/number/symbol/connector) are present.
 *
 * Build (from the project root, after a normal `./build.sh` so src/\*.c is
 * known to compile cleanly against this checkout). Note: unlike build.sh's
 * own flags, no `-std=c99` on the interpreter's own object files below --
 * they rely on GNU extensions (e.g. MAP_ANON) build.sh itself gets for
 * free by NOT passing -std=c99; only this test file needs strict C99:
 *   mkdir -p /tmp/ast_test_o
 *   for f in src/\*.c; do
 *     [ "$(basename "$f")" = "0.c" ] && continue
 *     cc -w -O2 -Isrc -c "$f" -o "/tmp/ast_test_o/$(basename "${f%.c}").o"
 *   done
 *   # 0.c needs -Dldstatic to compile its shared globals (pg, the wasm
 *   # js_eval() stub) WITHOUT its main() -- see 0.c's own `#ifndef ldstatic`
 *   # guard around `I main(...)`.
 *   cc -w -O2 -Isrc -Dldstatic -c src/0.c -o /tmp/ast_test_o/0.o
 *   cc -std=c99 -Wall -Wextra -O2 -Isrc /tmp/ast_test_o/\*.o tests/test_ast.c \
 *      -o /tmp/test_ast -lm -lpthread -ldl
 *   /tmp/test_ast
 *
 * (`-w` matches build.sh's own convention of silencing the core interpreter
 * files' pre-existing warnings, e.g. the a.h TU()/LH() sign-compare note
 * documented in ast.c -- this test file itself compiles warning-clean under
 * -Wall -Wextra and does not need -w for its own code.)
 */
#include "a.h"
#include "ast.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int fails = 0;

/* Runs `\ast src` with stdout redirected to a temp file and copies whatever it
 * printed into `buf`. Factored out of expect_contains()/expect_no_placeholder()
 * (which had two byte-identical copies of it) so the 1.9.5 checks that need to
 * inspect the raw output themselves -- banner column alignment, "this must NOT
 * appear" assertions -- can reuse it. Writes an empty string on capture
 * failure, so every caller can just read `buf` unconditionally. */
static void capture_ast(const char *src, char *buf, size_t cap) {
    CO C *path = "/tmp/.amber_test_ast_capture.out";
    buf[0] = 0;
    fflush(stdout);
    int saved = dup(1);
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (saved < 0 || fd < 0) { if (saved >= 0) close(saved); if (fd >= 0) close(fd); return; }

    dup2(fd, 1);
    ast_cmd((S)src);
    fflush(stdout);
    dup2(saved, 1);

    lseek(fd, 0, SEEK_SET);
    long n = read(fd, buf, cap - 1);
    if (n < 0) n = 0;
    buf[n] = 0;
    close(fd);
    close(saved);
    remove(path);
}

/* Runs `\ast src`, captures its stdout, and asserts every string in
 * `must[]` (NULL-terminated) appears somewhere in the printed tree. */
static void expect_contains(const char *src, const char *const must[], const char *what) {
    char buf[131072];
    capture_ast(src, buf, sizeof buf);
    for (int i = 0; must[i]; i++) {
        if (!strstr(buf, must[i])) {
            fprintf(stderr, "FAIL %s: \\ast %s\n  expected to contain: %s\n  got:\n%s\n", what, src, must[i], buf);
            fails++;
        }
    }
}

/* Asserts NONE of the historical placeholder bugs appear anywhere in the
 * tree -- a stronger, more direct regression check than expecting the
 * *correct* label, since it would also catch some *new*, differently-named
 * placeholder this revision didn't anticipate. */
static void expect_no_placeholder(const char *src, const char *what) {
    char buf[131072];
    capture_ast(src, buf, sizeof buf);
    static const char *BAD[] = { "-atom>", "<v-atom", "<w-atom", "<o-atom", "<I-atom", "<S-atom", 0 };
    for (int i = 0; BAD[i]; i++) {
        if (strstr(buf, BAD[i])) {
            fprintf(stderr, "FAIL %s: \\ast %s printed a placeholder (%s)\n  got:\n%s\n", what, src, BAD[i], buf);
            fails++;
        }
    }
}

int main(void) {
    kinit(); /* same startup call 0.c's own main() makes before any pk()/eval */

    /* ---- exact-label checks, one per historical bug category ---------- */
    {
        const char *m[] = { "Binary Op", "+", "Add", "Int64", 0 };
        expect_contains("1+2", m, "dyadic application labels the real glyph and name");
    }
    {
        const char *m[] = { "Lambda", "{x+1}", 0 };
        expect_contains("{x+1}", m, "a lambda literal shows its source text, not <o-atom>");
    }
    {
        const char *m[] = { "Hook", "tacit 2-train", 0 };
        expect_contains("(+;-)", m, "a 2-verb train is an explicit Hook");
    }
    {
        const char *m[] = { "Fork", "tacit 3-train", 0 };
        expect_contains("(*;+;%)", m, "a 3-verb train is an explicit Fork");
    }
    {
        const char *m[] = { "Projection", "curried", "Blank", 0 };
        expect_contains("1+", m, "a right-curried dyad is an explicit Projection with a Blank slot");
    }
    {
        const char *m[] = { "Projection", "Blank", "curried", 0 };
        expect_contains("f:{x+y+z};f[1;;3]", m, "a partial application (blank middle arg) is an explicit Projection");
    }
    {
        const char *m[] = { "Symbol Vector[3]", "`a", "`b", "`c", 0 };
        expect_contains("`a`b`c", m, "a literal symbol vector previews its elements, not <S-atom>");
    }
    {
        /* the old bug misread tS's packed 32-bit ids as 8-byte A* strides
         * and printed "ns.ns" (duplicated first segment) instead of the
         * real second segment -- assert the REAL second segment is there
         * and the wrong duplicate is not. */
        const char *m[] = { "ns.sub", 0 };
        expect_contains(".ns.sub", m, "a namespaced identifier reads its real second segment, not a duplicated first one");
    }
    {
        const char *m[] = { "Vector", "Vector[3]", 0 };
        expect_contains("1 2 3", m, "a literal numeric vector previews with a Vector[len] annotation");
    }
    {
        const char *m[] = { "Float64", 0 };
        expect_contains("1.5", m, "a float atom is labeled Float64");
    }

    /* ---- broad regression sweep: no placeholder text anywhere ---------- */
    static const char *SWEEP[] = {
        "+", "-", "*", "%", "!", "&", "|", "<", ">", "=", "~", ",", "^", "#", "_", "$", "?", "@", ".",
        "+/", "-/", "*/", "'", "\\", "/:", "\\:", "':",
        "1+2", "1-2", "1*2", "1%2", "2#1", "a,b", "1+", "(1+)@3",
        "(+;-)", "(*;+;%)", "{x+1}", "{[a;b]a+b}", "f:{x+y+z};f[1;;3]",
        "1 2 3", "1.5 2.5", "`a`b`c", "\"hello\"", ".ns.sub", "ns.sub", "`a",
        "1", "1.0", "0N", "0n", "0w",
    };
    for (unsigned i = 0; i < sizeof(SWEEP) / sizeof(*SWEEP); i++)
        expect_no_placeholder(SWEEP[i], "regression sweep");

    /* ---- ANSI color coverage: all five required categories present ----- */
    {
        const char *m[] = { "\x1b[1;38;2;255;176;0m", 0 }; /* amber: verbs */
        expect_contains("1+2", m, "verbs render amber");
    }
    {
        const char *m[] = { "\x1b[1;38;2;240;185;95m", 0 }; /* marigold: adverbs */
        expect_contains("+/x", m, "adverbs render marigold");
    }
    {
        const char *m[] = { "\x1b[38;2;170;185;135m", 0 }; /* warm sage: literals */
        expect_contains("42", m, "numeric literals render warm sage");
    }
    {
        const char *m[] = { "\x1b[38;2;205;175;135m", 0 }; /* warm tan: vars/symbols */
        expect_contains("myvar", m, "variables render warm tan");
    }
    {
        const char *m[] = { "\x1b[2m", 0 }; /* dim gray: connectors */
        expect_contains("1+2", m, "tree connectors render dim");
    }

    /* ---- 1.9.5: framed banner ------------------------------------------
     * The banner is drawn by print_ast() only for the AST_ROOT carrier that
     * ast_cmd() builds, and it must always render as a closed box: a top rule
     * carrying the title, an "Expr:" row, and a bottom rule. */
    {
        const char *m[] = { "\xe2\x94\x8c", "AST Visualization", "Expr: ", "1+2",
                             "\xe2\x94\x94", "\xe2\x94\x98", 0 };
        expect_contains("1+2", m, "print_ast frames the tree in a titled ASCII banner");
    }
    /* Every banner row must occupy exactly the same number of display columns,
     * or the right-hand border visibly tears. Counting is done in UTF-8 lead
     * bytes with the ANSI SGR sequences stripped, which is what the terminal
     * itself renders. This is the check that would have caught the box-drawing
     * glyphs being counted as 3 columns each. */
    {
        char buf[131072];
        capture_ast("1+2", buf, sizeof buf);
        int width[3], row = 0, col = 0, esc = 0;
        for (const char *p = buf; *p && row < 3; p++) {
            if (esc)                 { if (*p == 'm') esc = 0; continue; }
            if (*p == 27)            { esc = 1; continue; }
            if (*p == '\n')          { width[row++] = col; col = 0; continue; }
            if ((*p & 0xC0) != 0x80) col++;
        }
        if (row < 3 || width[0] != width[1] || width[1] != width[2]) {
            fprintf(stderr, "FAIL banner alignment: rows are %d/%d/%d columns wide (must match)\n"
                             "  got:\n%s\n", row > 0 ? width[0] : -1, row > 1 ? width[1] : -1,
                             row > 2 ? width[2] : -1, buf);
            fails++;
        }
    }
    /* An over-long expression must be ellipsised INSIDE the frame, never
     * allowed to push the right border out. */
    {
        char lng[512] = "f[";
        for (int i = 0; i < 40; i++) strcat(lng, "abcdef;");
        strcat(lng, "z]");
        const char *m[] = { "\xe2\x80\xa6", 0 };   /* the ellipsis glyph */
        expect_contains(lng, m, "an over-long expression is ellipsised inside the banner");
    }

    /* ---- 1.9.5: time-series join badging -------------------------------- */
    {
        const char *m[] = { "As-Of Time-Series Join", "\x1b[1;38;2;255;176;0m", 0 };
        expect_contains("aj[`sym`time;tr;qu]", m, "`aj` gets an As-Of join badge");
    }
    {
        const char *m[] = { "Window Join", 0 };
        expect_contains("wj[w;`sym`time;tr;qu;ag]", m, "`wj` gets a Window Join badge");
    }
    {   /* a name that merely CONTAINS a join name must not be badged */
        char buf[131072];
        capture_ast("ajax[1;2]", buf, sizeof buf);
        if (strstr(buf, "As-Of")) {
            fprintf(stderr, "FAIL: `ajax` must not be mistaken for `aj`\n  got:\n%s\n", buf);
            fails++;
        }
    }

    /* ---- 1.9.5: qSQL clause specialization ------------------------------
     * tests/test_ast.c calls kinit() and nothing else, so qsql.k's `qrw` is
     * NOT loaded and try_rewrite() passes text through untouched. That is
     * exactly why these cases are written in the already-rewritten form
     * (`sel"..."`) that qrw would have produced: it tests the AST module's own
     * clause decomposition rather than the K-level rewriter, and it keeps the
     * test independent of the stdlib being present. */
    {
        const char *m[] = { "qSQL Select", "Columns: px,sz", "From: t", "Where: px>10", 0 };
        expect_contains("sel\"select px,sz from t where px>10\"", m,
                         "a select query decomposes into columns / from / where");
    }
    {
        const char *m[] = { "By: sym", "Columns: sum px", "Where: px>5", 0 };
        expect_contains("sel\"select sum px by sym from t where px>5\"", m,
                         "a by-clause is split out from the projection and the from-target");
    }
    {
        const char *m[] = { "qSQL Exec", "result expression", 0 };
        expect_contains("exq\"exec px from t\"", m, "an exec query names its result expression");
    }
    {
        const char *m[] = { "qSQL Update", "column assignments", "Where: sym=`a", 0 };
        expect_contains("upd\"update px:px*2 from t where sym=`a\"", m,
                         "an update query names its assignment list");
    }
    {
        const char *m[] = { "qSQL Delete", "columns to drop", 0 };
        expect_contains("del\"delete px from t\"", m, "a delete query names the columns it drops");
    }
    {   /* the functional form keeps its arguments as real subtrees */
        const char *m[] = { "qSQL Exec", "functional form", 0 };
        expect_contains("qexec[t;w;b;d]", m, "the functional form still renders as a query block");
    }
    {   /* a `where`/`by` appearing only inside the predicate must not split */
        const char *m[] = { "Columns: px", "Where: sym=`byte", 0 };
        expect_contains("sel\"select px from t where sym=`byte\"", m,
                         "a keyword substring inside a predicate is not a clause boundary");
    }
    {   /* a query verb applied to a non-string falls back, it does not crash */
        expect_no_placeholder("sel[1 2 3]", "query verb applied to a non-string");
    }

    /* ---- safety: moderately deep (but pk()-legal) nesting doesn't crash */
    {
        char deep[512] = "1";
        char tmp[512];
        for (int i = 0; i < 40; i++) { snprintf(tmp, sizeof tmp, "(%s+1)", deep); strcpy(deep, tmp); }
        const char *m[] = { "Int64", 0 };
        expect_contains(deep, m, "40-deep nested parens do not crash the AST walker");
    }

    /* ---- self-test builtin agrees ---------------------------------------- */
    if (!ast_selftest()) { fprintf(stderr, "FAIL: ast_selftest() itself reported failure\n"); fails++; }

    if (fails) { fprintf(stderr, "%d AST TEST(S) FAILED\n", fails); return 1; }
    printf("ALL AST TESTS PASSED\n");
    return 0;
}
