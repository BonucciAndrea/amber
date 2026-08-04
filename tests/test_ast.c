/* test_ast.c  -  standalone test for src/ast.c's \ast tree formatter.
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * Unlike tests/test_simd.c and tests/test_parallel.c (each deliberately
 * dependency-free from the rest of the interpreter),
 * ast.c is inherently built on Amber's own parser and value representation
 * (pk(), su(), the tu/tv/tw/to/tS/... type tags) -- there is no meaningful
 * "AST of an expression" without Amber's real parser producing it. So this
 * test links against the WHOLE interpreter (every src object file except
 * 0.c, which owns main()) and calls kinit() itself before touching pk()/
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

/* Runs `\ast src`, captures its stdout, and asserts every string in
 * `must[]` (NULL-terminated) appears somewhere in the printed tree. */
static void expect_contains(const char *src, const char *const must[], const char *what) {
    CO C *path = "/tmp/.amber_test_ast_capture.out";
    fflush(stdout);
    int saved = dup(1);
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (saved < 0 || fd < 0) { fprintf(stderr, "FAIL %s: could not capture stdout\n", what); fails++; if (saved >= 0) close(saved); if (fd >= 0) close(fd); return; }

    dup2(fd, 1);
    ast_cmd((S)src);
    fflush(stdout);
    dup2(saved, 1);

    lseek(fd, 0, SEEK_SET);
    char buf[8192];
    long n = read(fd, buf, sizeof buf - 1);
    if (n < 0) n = 0;
    buf[n] = 0;
    close(fd);
    close(saved);
    remove(path);

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
    CO C *path = "/tmp/.amber_test_ast_capture.out";
    fflush(stdout);
    int saved = dup(1);
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0600);
    if (saved < 0 || fd < 0) { fprintf(stderr, "FAIL %s: could not capture stdout\n", what); fails++; if (saved >= 0) close(saved); if (fd >= 0) close(fd); return; }

    dup2(fd, 1);
    ast_cmd((S)src);
    fflush(stdout);
    dup2(saved, 1);

    lseek(fd, 0, SEEK_SET);
    char buf[8192];
    long n = read(fd, buf, sizeof buf - 1);
    if (n < 0) n = 0;
    buf[n] = 0;
    close(fd);
    close(saved);
    remove(path);

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
        const char *m[] = { "\x1b[1;36m", 0 }; /* bold cyan: verbs */
        expect_contains("1+2", m, "verbs render bold cyan");
    }
    {
        const char *m[] = { "\x1b[1;35m", 0 }; /* bold magenta: adverbs */
        expect_contains("+/x", m, "adverbs render bold magenta");
    }
    {
        const char *m[] = { "\x1b[92m", 0 }; /* bright green: literals */
        expect_contains("42", m, "numeric literals render bright green");
    }
    {
        const char *m[] = { "\x1b[33m", 0 }; /* yellow: vars/symbols */
        expect_contains("myvar", m, "variables render yellow");
    }
    {
        const char *m[] = { "\x1b[2m", 0 }; /* dim gray: connectors */
        expect_contains("1+2", m, "tree connectors render dim");
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
