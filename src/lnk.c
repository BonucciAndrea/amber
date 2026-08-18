/* lnk.c  -  Amber 1.9.5: the interpreter-facing surface of the line editor.
 *
 * One verb, and nothing else:
 *
 *   `rdl "prompt "   -> one console line, INCLUDING the trailing "\n"
 *                       ("" at end of input)
 *
 * repl.k calls it instead of reading fd 0 directly, which is what gives the
 * REPL editing, history and Tab completion without rlwrap.  When stdin is not
 * a terminal it degrades to an ordinary line read with the prompt still
 * printed, so `echo '2+2' | ./amber repl.k` and here-doc driven CI behave
 * exactly as they did before the editor existed.
 *
 * Kept in its own translation unit rather than folded into src/ln.c because
 * ln.c is deliberately free of the interpreter's headers: it is plain
 * C99 + POSIX and can be lifted into another project as-is.
 *
 * Amber - GNU AGPLv3 - see LICENSE and NOTICE.
 */
#include "a.h"
#include "ln.h"

#include <stdlib.h>
#include <string.h>

/* Copy a K char vector (or symbol) into a fresh NUL-terminated C string. */
static char *lnk_cstr(A v) {
    char *s;
    U n;
    if (!v) return NULL;
    if (_ts(v)) {                       /* symbol atom */
        const char *p = su(_v(v));
        n = (U)strlen(p);
        s = (char *)malloc(n + 1);
        if (!s) return NULL;
        memcpy(s, p, n + 1);
        return s;
    }
    if (_t(v) != tC) return NULL;
    n = _n(v);
    s = (char *)malloc((size_t)n + 1);
    if (!s) return NULL;
    if (n) memcpy(s, _C(v), n);
    s[n] = 0;
    return s;
}

A rdlC(A x) {
    char *p = lnk_cstr(x);
    char *line;
    size_t n = 0;
    A r;
    mr(x);
    line = am_repl_getline(p ? p : "", &n);
    free(p);
    if (!line) return emp(tC);
    r = aCn((S)line, (U)n);
    free(line);
    return r;
}
