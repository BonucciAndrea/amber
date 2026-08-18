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
