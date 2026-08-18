/* ln.c  -  Amber 1.9.5 native line editor.  See ln.h for the rationale.
 *
 * Plain C99 + POSIX (termios, ioctl, read/write).  No curses, no readline, no
 * terminfo, no rlwrap, and no allocation on the keystroke path beyond the line
 * buffer itself.  The terminal is switched to raw mode for the duration of one
 * line and restored on every exit path, including atexit(), so a crash or a ^C
 * can never leave the user's shell without an echo.
 *
 * Raw mode is entered ONLY when this file is certain it owns the terminal.  A
 * pipe, a dumb TERM, AMBER_NO_EDIT -- and, since 1.9.5, a parent rlwrap/rlfe --
 * each fall back to a plain canonical line read.  See am_ln_under_rlwrap()
 * below for why that last one matters more than it looks.
 *
 * Amber - GNU AGPLv3 - see LICENSE and NOTICE.
 */
#if !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif
#if !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE 1
#endif

#include "ln.h"
#include "ext.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#if !defined(wasm)
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif
#endif

#define LN_MAX_LINE   4096
#define LN_MAX_HIST   500
#define LN_MAX_CAND   64

/* ---- history ------------------------------------------------------------- */

static char *g_hist[LN_MAX_HIST];
static int   g_hist_n;
static int   g_hist_max = LN_MAX_HIST;

void am_ln_history_set_max(int n) {
    if (n > 0 && n <= LN_MAX_HIST) g_hist_max = n;
}

void am_ln_history_add(const char *line) {
    char *copy;
    if (!line || !*line) return;
    if (g_hist_n && !strcmp(g_hist[g_hist_n - 1], line)) return;
    copy = (char *)malloc(strlen(line) + 1);
    if (!copy) return;
    strcpy(copy, line);
    if (g_hist_n == g_hist_max) {
        free(g_hist[0]);
        memmove(g_hist, g_hist + 1, sizeof(char *) * (size_t)(g_hist_max - 1));
        g_hist_n--;
    }
    g_hist[g_hist_n++] = copy;
}

int am_ln_history_load(const char *path) {
    FILE *f = path ? fopen(path, "r") : NULL;
    char buf[LN_MAX_LINE];
    if (!f) return -1;
    while (fgets(buf, (int)sizeof buf, f)) {
        size_t n = strlen(buf);
        while (n && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) buf[--n] = 0;
        am_ln_history_add(buf);
    }
    fclose(f);
    return 0;
}

const char *const *am_ln_history(int *n) {
    if (n) *n = g_hist_n;
    return (const char *const *)g_hist;
}

int am_ln_history_save(const char *path) {
    FILE *f = path ? fopen(path, "w") : NULL;
    int i;
    if (!f) return -1;
    for (i = 0; i < g_hist_n; i++) fprintf(f, "%s\n", g_hist[i]);
    fclose(f);
    return 0;
}

/* ---- completion registry ------------------------------------------------- */

static amCompletionCallback *g_completion;

void am_ln_set_completion_callback(amCompletionCallback *fn) { g_completion = fn; }

void am_ln_add_completion(amCompletions *lc, const char *str) {
    char **v;
    size_t i;
    if (!lc || !str || !*str) return;
    if (lc->len >= LN_MAX_CAND) return;
    for (i = 0; i < lc->len; i++) if (!strcmp(lc->cvec[i], str)) return; /* dedupe */
    v = (char **)realloc(lc->cvec, sizeof(char *) * (lc->len + 1));
    if (!v) return;
    lc->cvec = v;
    v[lc->len] = (char *)malloc(strlen(str) + 1);
    if (!v[lc->len]) return;
    strcpy(v[lc->len], str);
    lc->len++;
}

static void free_completions(amCompletions *lc) {
    size_t i;
    for (i = 0; i < lc->len; i++) free(lc->cvec[i]);
    free(lc->cvec);
    lc->cvec = NULL;
    lc->len = 0;
}

#if defined(wasm)
/* ------------------------------------------------------------------------- */
/* No terminal in the wasm sandbox: everything degrades to a plain read.      */
int   am_ln_interactive(void) { return 0; }
int   am_ln_under_rlwrap(void) { return 0; }
char *am_ln_readline(const char *p) { (void)p; return NULL; }
void  am_repl_init(void) {}

char *am_repl_getline(const char *prompt, size_t *len) {
    (void)prompt; if (len) *len = 0; return NULL;
}
#else

/* ---- raw mode ------------------------------------------------------------ */

static struct termios g_orig;
static int g_raw;
static int g_atexit;

static int is_dumb(void) {
    const char *t = getenv("TERM");
    return t && (!strcmp(t, "dumb") || !strcmp(t, "cons25") || !strcmp(t, "emacs"));
}

/* ======================================================================== */
/* Are we running inside rlwrap (or rlfe)?                                  */
/* ======================================================================== */
/*
 * WHY THIS EXISTS.
 *
 * 1.9.5 gave the REPL its own editor, and the ./a launcher stopped invoking
 * rlwrap.  That fixed the warning for anyone who starts Amber with ./a -- and
 * for nobody else.  A user who kept `alias amber='rlwrap amber'` from the 1.9.4
 * era, or a wrapper script, a tmux command, a .desktop entry, or simply the
 * `rlwrap ./amber repl.k` line the old docs recommended, still got
 *
 *     rlwrap: warning: rlwrap appears to do nothing for amber, which asks for
 *     single keypresses all the time ...
 *
 * on the first error, because the fix lived in the launcher rather than in the
 * program.  A launcher can only speak for the one way it is used; the engine
 * has to be able to defend itself however it is started.
 *
 * WHAT WE DO ABOUT IT.
 *
 * rlwrap's complaint is legitimate: it is a canonical-mode line editor, and two
 * line editors cannot share one cursor.  Since we cannot pass rlwrap its own -n
 * from in here, the honest resolution is to STAND DOWN -- when Amber notices it
 * was started under rlwrap it leaves the terminal in canonical mode and reads
 * whole lines, exactly as 1.9.4 did.  rlwrap then has nothing to warn about,
 * the two editors stop fighting over the cursor, and the user still gets full
 * readline editing and history: nothing is lost either way.
 *
 * HOW WE DETECT IT.
 *
 * rlwrap exports no environment variable to the program it wraps (verified), so
 * the only reliable signal is process lineage: rlwrap runs the wrapped command
 * as its direct child.  We walk up to four ancestors -- four, not one, because
 * an intervening wrapper script that does not exec() would otherwise hide it --
 * and stop at the first process named rlwrap or rlfe.
 *
 * The walk is Linux (/proc) and macOS (sysctl) natively, and falls back to one
 * `ps` on any other POSIX host.  It runs at most ONCE per process, and only
 * when stdin is already known to be a terminal, so it costs nothing in a script
 * or a pipeline.  If lineage cannot be determined at all we assume no wrapper
 * and behave exactly as before -- an unknown platform loses nothing.
 *
 * OVERRIDES.  AMBER_RLWRAP=0 keeps Amber's own editor even under rlwrap (the
 * old behaviour, warning included); AMBER_RLWRAP=1 forces the stand-down for a
 * wrapper we failed to recognise.
 */

/* Fill `name` with the command name of `pid` and `*ppid` with its parent.
 * Returns 0 on success, -1 when the platform will not tell us. */
static int proc_ident(long pid, char *name, size_t cap, long *ppid) {
#if defined(__linux__)
    char path[64], buf[1024];
    FILE *f;
    size_t n;

    snprintf(path, sizeof path, "/proc/%ld/comm", pid);
    if (!(f = fopen(path, "r"))) return -1;
    if (!fgets(name, (int)cap, f)) { fclose(f); return -1; }
    fclose(f);
    n = strlen(name);
    while (n && (name[n - 1] == '\n' || name[n - 1] == '\r')) name[--n] = 0;

    *ppid = 0;
    snprintf(path, sizeof path, "/proc/%ld/stat", pid);
    if ((f = fopen(path, "r"))) {
        if (fgets(buf, (int)sizeof buf, f)) {
            /* field 2 is the command in parentheses and MAY CONTAIN both spaces
             * and parentheses, so the only safe anchor is the LAST ')'. After
             * it come " <state> <ppid>". */
            char *p = strrchr(buf, ')');
            if (p) {
                long v = 0;
                if (sscanf(p + 1, " %*c %ld", &v) == 1) *ppid = v;
            }
        }
        fclose(f);
    }
    return 0;

#elif defined(__APPLE__)
    struct kinfo_proc kp;
    size_t len = sizeof kp;
    int mib[4];

    mib[0] = CTL_KERN; mib[1] = KERN_PROC; mib[2] = KERN_PROC_PID; mib[3] = (int)pid;
    memset(&kp, 0, sizeof kp);
    if (sysctl(mib, 4, &kp, &len, NULL, 0) != 0 || len == 0) return -1;
    strncpy(name, kp.kp_proc.p_comm, cap - 1);
    name[cap - 1] = 0;
    *ppid = (long)kp.kp_eproc.e_ppid;
    return 0;

#else
    /* Any other POSIX host: one `ps`, once per process.  Deliberately asks for
     * both fields in one call so this costs a single fork, not two. */
    char cmd[96], line[256];
    FILE *f;
    long v = 0;

    snprintf(cmd, sizeof cmd, "ps -o ppid=,comm= -p %ld 2>/dev/null", pid);
    if (!(f = popen(cmd, "r"))) return -1;
    line[0] = 0;
    if (!fgets(line, (int)sizeof line, f)) { pclose(f); return -1; }
    pclose(f);
    {
        char nm[128];
        nm[0] = 0;
        if (sscanf(line, " %ld %127s", &v, nm) != 2) return -1;
        strncpy(name, nm, cap - 1);
        name[cap - 1] = 0;
        *ppid = v;
    }
    return 0;
#endif
}

/* rlwrap and rlfe are the two canonical-mode wrappers in the wild.  `name` may
 * arrive as a full path on the `ps` fallback, so compare the basename. */
static int is_wrapper_name(const char *name) {
    const char *b = strrchr(name, '/');
    b = b ? b + 1 : name;
    return !strcmp(b, "rlwrap") || !strcmp(b, "rlfe");
}

int am_ln_under_rlwrap(void) {
    static int cached = -1;
    const char *e;
    long pid, ppid;
    char name[128];
    int hops;

    if (cached >= 0) return cached;
    cached = 0;

    if ((e = getenv("AMBER_RLWRAP")) && *e) {   /* explicit override wins */
        cached = (*e != '0');
        return cached;
    }

    pid = (long)getppid();
    for (hops = 0; hops < 4 && pid > 1; hops++) {
        name[0] = 0;
        ppid = 0;
        if (proc_ident(pid, name, sizeof name, &ppid) != 0) break;
        if (is_wrapper_name(name)) { cached = 1; break; }
        if (ppid <= 1 || ppid == pid) break;
        pid = ppid;
    }
    return cached;
}

/* One line, once, to stderr: silence would be worse than a note here, because
 * the user has explicitly asked for rlwrap and would otherwise never learn that
 * Amber has an editor of its own. */
static void rlwrap_note(void) {
    static int said;
    if (said) return;
    said = 1;
    fputs("amber: started under rlwrap -- leaving line editing to it.\n"
          "       amber has had its own editor since 1.9.5: start it with ./a "
          "(no rlwrap) to use that instead.\n", stderr);
    fflush(stderr);
}

int am_ln_interactive(void) {
    if (getenv("AMBER_NO_EDIT")) return 0;
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO) || is_dumb()) return 0;
    /* Checked LAST so the lineage walk never runs for a pipe, a here-doc or a
     * CI job -- only for a session that would actually have entered raw mode. */
    if (am_ln_under_rlwrap()) { rlwrap_note(); return 0; }
    return 1;
}

static void disable_raw(void) {
    if (g_raw) { tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig); g_raw = 0; }
}

static void on_exit_restore(void) { disable_raw(); }

static int enable_raw(void) {
    struct termios r;
    if (g_raw) return 0;
    if (!isatty(STDIN_FILENO)) return -1;
    if (tcgetattr(STDIN_FILENO, &g_orig) == -1) return -1;
    if (!g_atexit) { atexit(on_exit_restore); g_atexit = 1; }
    r = g_orig;
    r.c_iflag &= ~(unsigned)(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    r.c_oflag &= ~(unsigned)(OPOST);
    r.c_cflag |=  (unsigned)(CS8);
    r.c_lflag &= ~(unsigned)(ECHO | ICANON | IEXTEN | ISIG);
    r.c_cc[VMIN] = 1;
    r.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &r) < 0) return -1;
    g_raw = 1;
    return 0;
}

static int term_cols(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 4) return ws.ws_col;
    return 80;
}

static void wr(const char *s, size_t n) {
    while (n) {
        ssize_t k = write(STDOUT_FILENO, s, n);
        if (k <= 0) { if (k < 0 && errno == EINTR) continue; return; }
        s += k; n -= (size_t)k;
    }
}
static void ws_(const char *s) { wr(s, strlen(s)); }

/* ---- editor state -------------------------------------------------------- */

typedef struct {
    char        buf[LN_MAX_LINE];
    size_t      len, pos;
    const char *prompt;
    size_t      plen;
    int         cols;
    char        ghost[512];   /* pending extension hint, "" when none */
    int         hidx;         /* history browse index (g_hist_n = "current") */
    char        stash[LN_MAX_LINE];
} LnState;

static void refresh(LnState *l) {
    char seq[64];
    size_t start = 0, show, plen = l->plen;
    size_t cols = (size_t)(l->cols = term_cols());

    /* Horizontal scroll: keep the cursor visible on one physical line. */
    if (plen > cols - 2) plen = 0;              /* absurdly long prompt: drop it */
    show = l->len;
    while (plen + (l->pos - start) >= cols - 1) start++;
    if (plen + (show - start) >= cols - 1) show = start + (cols - 1 - plen);

    ws_("\r");
    if (plen) wr(l->prompt, plen);
    wr(l->buf + start, show - start);
    if (l->ghost[0] && l->pos == l->len) {
        size_t room = cols - 1 - plen - (show - start);
        size_t gl = strlen(l->ghost);
        if (gl > room) gl = room;
        if (gl) { ws_("\x1b[2m"); wr(l->ghost, gl); ws_("\x1b[0m"); }
    }
    ws_("\x1b[0K");
    sprintf(seq, "\r\x1b[%dC", (int)(plen + (l->pos - start)));
    ws_(seq);
}

static void ins(LnState *l, const char *s, size_t n) {
    if (l->len + n >= sizeof l->buf) return;
    memmove(l->buf + l->pos + n, l->buf + l->pos, l->len - l->pos);
    memcpy(l->buf + l->pos, s, n);
    l->len += n;
    l->pos += n;
    l->buf[l->len] = 0;
}

static void del_left(LnState *l) {
    if (!l->pos) return;
    memmove(l->buf + l->pos - 1, l->buf + l->pos, l->len - l->pos);
    l->pos--; l->len--;
    l->buf[l->len] = 0;
}

static void del_right(LnState *l) {
    if (l->pos >= l->len) return;
    memmove(l->buf + l->pos, l->buf + l->pos + 1, l->len - l->pos - 1);
    l->len--;
    l->buf[l->len] = 0;
}

static void kill_word(LnState *l) {
    size_t p = l->pos;
    while (p && l->buf[p - 1] == ' ') p--;
    while (p && l->buf[p - 1] != ' ') p--;
    memmove(l->buf + p, l->buf + l->pos, l->len - l->pos);
    l->len -= l->pos - p;
    l->pos = p;
    l->buf[l->len] = 0;
}

static void set_line(LnState *l, const char *s) {
    size_t n = strlen(s);
    if (n >= sizeof l->buf) n = sizeof l->buf - 1;
    memcpy(l->buf, s, n);
    l->buf[n] = 0;
    l->len = l->pos = n;
}

/* ---- Tab ----------------------------------------------------------------- */

/* Longest common prefix of the candidate set, written into `dst`. */
static void common_prefix(amCompletions *lc, char *dst, size_t cap) {
    size_t i, k = 0;
    if (!lc->len) { dst[0] = 0; return; }
    for (;;) {
        char c;
        if (k + 1 >= cap) break;
        c = lc->cvec[0][k];
        if (!c) break;
        for (i = 1; i < lc->len; i++) if (lc->cvec[i][k] != c) break;
        if (i < lc->len) break;
        dst[k++] = c;
    }
    dst[k] = 0;
}

static void list_candidates(LnState *l, amCompletions *lc) {
    size_t i;
    ws_("\r\n");
    for (i = 0; i < lc->len; i++) {
        ws_("  ");
        ws_(lc->cvec[i]);
        ws_("\r\n");
    }
    refresh(l);
}

/* Ask the installed extension (src/ext.h) for an inline continuation of the
 * current line.  Returns 1 when l->ghost was filled.  With no extension in the
 * build am_ext_hint is NULL and this costs one predicted branch.
 *
 * The guards belong to the editor, not to the extension: a hint is only ever
 * asked for at end of line, only for a line long enough to be worth
 * continuing, and never for a `\` REPL command -- those are lexical and
 * complete exactly. */
static int ext_ghost(LnState *l) {
    if (!am_ext_hint) return 0;
    if (l->len < 3 || l->pos != l->len) return 0;
    if (l->buf[0] == '\\') return 0;
    l->ghost[0] = 0;
    if (!am_ext_hint(l->buf, l->len, l->ghost, sizeof l->ghost)) { l->ghost[0] = 0; return 0; }
    l->ghost[sizeof l->ghost - 1] = 0;
    return l->ghost[0] != 0;
}

static void accept_ghost(LnState *l) {
    if (!l->ghost[0]) return;
    ins(l, l->ghost, strlen(l->ghost));
    am_ext_set_accepted(l->buf);   /* an extension may treat this as feedback */
    l->ghost[0] = 0;
}

static void do_tab(LnState *l) {
    amCompletions lc;
    char pfx[LN_MAX_LINE];

    if (l->ghost[0]) { accept_ghost(l); refresh(l); return; }

    lc.len = 0; lc.cvec = NULL;
    if (g_completion) g_completion(l->buf, &lc);

    if (lc.len == 1) {
        set_line(l, lc.cvec[0]);
        free_completions(&lc);
        refresh(l);
        return;
    }
    if (lc.len > 1) {
        common_prefix(&lc, pfx, sizeof pfx);
        if (strlen(pfx) > l->len) { set_line(l, pfx); refresh(l); }
        else list_candidates(l, &lc);
        free_completions(&lc);
        return;
    }
    free_completions(&lc);
    if (ext_ghost(l)) refresh(l);
}

/* ---- the editor loop ----------------------------------------------------- */

char *am_ln_readline(const char *prompt) {
    LnState l;
    char *out;

    memset(&l, 0, sizeof l);
    l.prompt = prompt ? prompt : "";
    l.plen   = strlen(l.prompt);
    l.cols   = term_cols();
    l.hidx   = g_hist_n;

    if (enable_raw() < 0) return NULL;
    refresh(&l);

    for (;;) {
        char c;
        ssize_t k = read(STDIN_FILENO, &c, 1);
        if (k < 0 && errno == EINTR) continue;
        if (k <= 0) { disable_raw(); if (l.len) break; return NULL; }

        /* Any keystroke other than an explicit accept discards the ghost. */
        if (l.ghost[0] && c != 9 && c != 6 && c != 27) l.ghost[0] = 0;

        switch (c) {
        case 9:                                  /* Tab */
            do_tab(&l);
            continue;
        case 13: case 10:                        /* Enter */
            l.ghost[0] = 0;
            goto done;
        case 3:                                  /* Ctrl-C: abandon this line */
            l.len = l.pos = 0; l.buf[0] = 0;
            ws_("^C\r\n");
            disable_raw();
            out = (char *)malloc(1);
            if (out) out[0] = 0;
            return out;
        case 4:                                  /* Ctrl-D */
            if (!l.len) { disable_raw(); ws_("\r\n"); return NULL; }
            del_right(&l); break;
        case 8: case 127:                        /* Backspace */
            del_left(&l); break;
        case 1:  l.pos = 0; break;               /* Ctrl-A */
        case 5:  l.pos = l.len; break;           /* Ctrl-E */
        case 2:  if (l.pos) l.pos--; break;      /* Ctrl-B */
        case 6:                                  /* Ctrl-F / accept ghost */
            if (l.ghost[0] && l.pos == l.len) accept_ghost(&l);
            else if (l.pos < l.len) l.pos++;
            break;
        case 11: l.buf[l.pos] = 0; l.len = l.pos; break;       /* Ctrl-K */
        case 21: memmove(l.buf, l.buf + l.pos, l.len - l.pos); /* Ctrl-U */
                 l.len -= l.pos; l.pos = 0; l.buf[l.len] = 0; break;
        case 23: kill_word(&l); break;                          /* Ctrl-W */
        case 12: ws_("\x1b[H\x1b[2J"); break;                   /* Ctrl-L */
        case 16: case 14: {                                     /* Ctrl-P/N */
            int up = (c == 16);
            if (!g_hist_n) break;
            if (l.hidx == g_hist_n) { memcpy(l.stash, l.buf, l.len + 1); }
            l.hidx += up ? -1 : 1;
            if (l.hidx < 0) l.hidx = 0;
            if (l.hidx > g_hist_n) l.hidx = g_hist_n;
            set_line(&l, l.hidx == g_hist_n ? l.stash : g_hist[l.hidx]);
            break;
        }
        case 27: {                                              /* escape seq */
            char s[3] = {0, 0, 0};
            if (read(STDIN_FILENO, s, 1) != 1) break;
            if (read(STDIN_FILENO, s + 1, 1) != 1) break;
            if (s[0] == '[') {
                if (s[1] >= '0' && s[1] <= '9') {
                    if (read(STDIN_FILENO, s + 2, 1) != 1) break;
                    if (s[2] == '~') {
                        if (s[1] == '3') del_right(&l);          /* Delete */
                        else if (s[1] == '1' || s[1] == '7') l.pos = 0;
                        else if (s[1] == '4' || s[1] == '8') l.pos = l.len;
                    }
                } else switch (s[1]) {
                    case 'A': case 'B': {
                        int up = (s[1] == 'A');
                        if (!g_hist_n) break;
                        if (l.hidx == g_hist_n) memcpy(l.stash, l.buf, l.len + 1);
                        l.hidx += up ? -1 : 1;
                        if (l.hidx < 0) l.hidx = 0;
                        if (l.hidx > g_hist_n) l.hidx = g_hist_n;
                        set_line(&l, l.hidx == g_hist_n ? l.stash : g_hist[l.hidx]);
                        break;
                    }
                    case 'C':
                        if (l.ghost[0] && l.pos == l.len) accept_ghost(&l);
                        else if (l.pos < l.len) l.pos++;
                        break;
                    case 'D': if (l.pos) l.pos--; break;
                    case 'H': l.pos = 0; break;
                    case 'F': l.pos = l.len; break;
                    default: break;
                }
            } else if (s[0] == 'O') {
                if (s[1] == 'H') l.pos = 0;
                else if (s[1] == 'F') l.pos = l.len;
            }
            l.ghost[0] = 0;
            break;
        }
        default:
            if ((unsigned char)c < 32) break;    /* ignore other controls */
            ins(&l, &c, 1);
            break;
        }
        refresh(&l);
    }

done:
    disable_raw();
    ws_("\r\n");
    out = (char *)malloc(l.len + 1);
    if (!out) return NULL;
    memcpy(out, l.buf, l.len);
    out[l.len] = 0;
    am_ln_history_add(out);
    return out;
}

/* ========================================================================== */
/* Completion sources                                                          */
/* ========================================================================== */

/* ---- lexical vocabulary ---------------------------------------------- */

static const char *const REPL_CMDS[] = {
    "\\l ", "\\v", "\\d ", "\\f", "\\m", "\\t ", "\\t:", "\\cd ",
    "\\ast ", "\\trace ", "\\disasm ", "\\grid ", "\\clear", "\\h", "\\q",
    "\\j", "\\z", "\\a", "\\\\", NULL
};

/* Amber / K vocabulary worth completing.  Kept short on purpose: the point is
 * to cover the names a user types constantly, not to mirror the manual. */
static const char *const WORDS[] = {
    "select", "from", "where", "by", "update", "delete", "exec", "insert",
    "sum", "avg", "min", "max", "count", "first", "last", "dev", "var", "med",
    "sums", "prds", "maxs", "mins", "deltas", "ratios", "til", "distinct",
    "asc", "desc", "xasc", "xdesc", "xkey", "xcol", "xgroup", "ungroup",
    "cols", "keys", "value", "flip", "unkey", "show", "meta", "type",
    "aj", "wj", "lj", "ij", "uj", "pj", "ej", "taq", "tsign", "signedvol",
    "vwap", "twap", "bars", "symstats", "mid", "spread", "spreadbps",
    "micro", "imbal", "ret", "logret", "rvol", "effspread", "notional",
    "movavg", "movsum", "movmax", "movmin", "ema", "rollstd",
    "msum", "mavg", "mdev", "mvar", "mmin", "mmax", "mcount", "mprd",
    "gentq", "genopt", "gidx", "bysym", "symrows", "sortcol", "partcol",
    "groupcol", "peach", "parse", "eval", "ser", "deser", "protect", "ts",
    "splay", "dload", "partsave", "partload", "dset", "dget", "parts",
    "plot", "candle", "amfmt", "tsym", "pt", "jenc", "cast", "long", "float",
    NULL
};

/* Where does the token under the cursor start? */
static size_t token_start(const char *buf, size_t len) {
    size_t i = len;
    while (i) {
        char c = buf[i - 1];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '.' || c == '_') { i--; continue; }
        break;
    }
    return i;
}

static void add_word(amCompletions *lc, const char *buf, size_t start,
                     const char *tok, const char *word) {
    char cand[LN_MAX_LINE];
    size_t tl = strlen(tok);
    if (tl && strncmp(word, tok, tl)) return;
    if (!strcmp(word, tok)) return;
    if (start + strlen(word) + 1 >= sizeof cand) return;
    memcpy(cand, buf, start);
    strcpy(cand + start, word);
    am_ln_add_completion(lc, cand);
}

static void repl_complete(const char *buf, amCompletions *lc) {
    size_t len = strlen(buf), start;
    char tok[256];
    int i;

    /* (a) an installed extension gets first refusal on the whole line */
    if (am_ext_complete) { am_ext_complete(buf, lc); if (lc->len) return; }

    /* (b) REPL commands, when the line IS a backslash command being typed */
    if (buf[0] == '\\' && !strchr(buf, ' ')) {
        for (i = 0; REPL_CMDS[i]; i++)
            if (!strncmp(REPL_CMDS[i], buf, len) && strcmp(REPL_CMDS[i], buf))
                am_ln_add_completion(lc, REPL_CMDS[i]);
        if (lc->len) return;
    }

    start = token_start(buf, len);
    if (len - start >= sizeof tok) return;
    memcpy(tok, buf + start, len - start);
    tok[len - start] = 0;

    /* (c) globals actually present in the workspace */
    {
        char names[16384];
        unsigned n = am_globals(names, sizeof names);
        const char *p = names;
        unsigned k;
        for (k = 0; k < n && *p; k++) {
            add_word(lc, buf, start, tok, p);
            p += strlen(p) + 1;
        }
    }

    /* (d) the standing vocabulary */
    if (*tok) for (i = 0; WORDS[i]; i++) add_word(lc, buf, start, tok, WORDS[i]);

    /* (e) whole lines already entered in this session.  Entirely local, and
     * the reason recalling a long query feels instant. */
    if (len >= 2) {
        int m, hn = 0;
        const char *const *h = am_ln_history(&hn);
        for (m = hn - 1; m >= 0; m--)
            if (!strncmp(h[m], buf, len) && strcmp(h[m], buf))
                am_ln_add_completion(lc, h[m]);
    }

    /* (f) last, and only if everything above found nothing: an extension's
     * wide/fuzzy source.  Deliberately after (c) so a recall file can never
     * shadow the name of a variable that is actually in scope. */
    if (!lc->len && am_ext_complete_late) am_ext_complete_late(buf, lc);
}

/* ---- REPL entry points --------------------------------------------------- */

static char g_histpath[512];

void am_repl_init(void) {
    static int done;
    const char *h;
    if (done) return;
    done = 1;
    am_ext_startup_once();
    am_ln_set_completion_callback(repl_complete);
    h = getenv("HOME");
    if (h && *h && strlen(h) + 20 < sizeof g_histpath) {
        sprintf(g_histpath, "%s/.amber_history", h);
        am_ln_history_load(g_histpath);
    }
}

char *am_repl_getline(const char *prompt, size_t *len) {
    char *line, *out;
    size_t n;

    am_repl_init();

    if (am_ln_interactive()) {
        line = am_ln_readline(prompt && *prompt ? prompt : "");
        if (!line) { if (len) *len = 0; return NULL; }
        if (g_histpath[0]) am_ln_history_save(g_histpath);
        n = strlen(line);
        out = (char *)realloc(line, n + 2);
        if (!out) { free(line); if (len) *len = 0; return NULL; }
        out[n] = '\n';
        out[n + 1] = 0;
        if (len) *len = n + 1;
        return out;
    }

    /* Not a terminal: print the prompt and read one line, one byte at a time,
     * so no input beyond the newline is ever swallowed from a shared stdin. */
    if (prompt && *prompt) wr(prompt, strlen(prompt));
    n = 0;
    out = (char *)malloc(LN_MAX_LINE);
    if (!out) { if (len) *len = 0; return NULL; }
    for (;;) {
        char c;
        ssize_t k = read(STDIN_FILENO, &c, 1);
        if (k < 0 && errno == EINTR) continue;
        if (k <= 0) break;
        if (n + 2 >= LN_MAX_LINE) break;
        out[n++] = c;
        if (c == '\n') break;
    }
    if (!n) { free(out); if (len) *len = 0; return NULL; }
    if (out[n - 1] != '\n') out[n++] = '\n';
    out[n] = 0;
    if (len) *len = n;
    return out;
}

#endif /* wasm */
