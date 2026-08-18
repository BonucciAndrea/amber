/* tests/ext_probe.c  -  a minimal, self-contained extension.
 *
 * Not part of the engine.  tests/test_ext_seam.sh copies this file into ext/,
 * rebuilds, checks it works and then removes it again -- which is exactly the
 * lifecycle of a real out-of-tree package such as `amber-ai`, in miniature.
 *
 * It exercises every hook in src/ext.h:
 *
 *   `pr7 x      a verb registered at runtime (returns 7 for any argument)
 *   \probe ...  a REPL command claimed ahead of the /bin/sh fallback
 *   am_ext_hint an editor hint (ghost text) with no network anywhere in sight
 *   am_ext_startup / am_ext_usage / am_ext_banner
 *
 * Build: nothing to do.  Dropping a .c file in ext/ and running ./build.sh is
 * the entire installation protocol; src/ is never patched.
 *
 * Amber - GNU AGPLv3 - see LICENSE and NOTICE.
 */
#include "a.h"
#include "ext.h"
#include "ln.h"

#include <stdio.h>
#include <string.h>

static int g_started;

/* ---- a verb: `pr7 x -> 7 ------------------------------------------------- */
static A probe_verb(A x) {
    mr(x);
    return ai(7);
}

/* ---- a REPL command: \probe ---------------------------------------------- */
static unsigned long long probe_bs(const char *line) {
    if (strncmp(line, "probe", 5)) return 0;          /* not ours */
    if (line[5] && line[5] != ' ') return 0;
    printf("probe: ok started=%d verbs=%d\n", g_started, am_ext_verb_count());
    fflush(stdout);
    return (unsigned long long)au;                    /* handled */
}

/* ---- an editor hint ------------------------------------------------------ */
static int probe_hint(const char *line, size_t len, char *dst, size_t cap) {
    (void)len;
    if (strncmp(line, "probeh", 6)) return 0;
    snprintf(dst, cap, "INT");
    return 1;
}

/* ---- extra completion sources (early and late) ---------------------------- */
static void probe_complete(const char *line, void *lc) {
    if (strncmp(line, "\\probe", 6)) return;
    am_ln_add_completion((amCompletions *)lc, "\\probe now");
}

static void probe_complete_late(const char *line, void *lc) {
    if (strncmp(line, "probec", 6)) return;
    am_ln_add_completion((amCompletions *)lc, "probecompleted");
}

static void probe_startup(void) { g_started = 1; }

__attribute__((constructor))
static void probe_register(void) {
    am_ext_verb("pr7", (void *)probe_verb);
    am_ext_bs       = probe_bs;
    am_ext_hint     = probe_hint;
    am_ext_complete = probe_complete;
    am_ext_complete_late = probe_complete_late;
    am_ext_startup  = probe_startup;
    am_ext_usage    = "\next probe: \\probe, `pr7\n";
    am_ext_banner   = " [ext-probe]";
}
