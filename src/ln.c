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
#include <signal.h>
#include <sys/time.h>
#include <time.h>      /* clock_gettime(CLOCK_MONOTONIC): the exec timer's clock */
#if defined(__APPLE__)
#include <sys/sysctl.h>
#include <mach/mach.h>
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
/* the optional status bar has no terminal to draw on in the wasm sandbox */
void  am_ln_statusbar(int on, const char *main, const char *info) {
    (void)on; (void)main; (void)info;
}
#else

/* ---- raw mode ------------------------------------------------------------ */

static struct termios g_orig;
static int g_raw;
static int g_atexit;
static int g_sb_alt;   /* the status bar runs in the alternate screen (like vim) */

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

static void ws_(const char *s);  /* fwd: defined below; used by raw enable/disable for bracketed paste */
static void ring_clear(void);    /* fwd: scroll-back ring, defined below; used by the bar teardown */

/* ---- scroll-back ring state (definitions of the helpers are further down) --
 * The alternate screen keeps no native scrollback, so to let the user scroll UP
 * through past output while the box stays locked we tee every byte the kernel
 * prints (ow -> am_ln_sb_capture) into a ring of completed lines and repaint the
 * region from it at an offset.  Declared here because the bar enable/teardown
 * (below) arm and clear it. */
#define SB_RING 5000
static char *g_ring[SB_RING];   /* completed output lines, oldest..newest (ANSI kept) */
static int   g_ring_n;          /* filled slots (caps at SB_RING) */
static int   g_ring_head;       /* next slot to write (ring index) */
static char  g_ring_part[16384];/* the line currently being accumulated */
static int   g_ring_plen;
static int   g_sb_capture;      /* tee ow() only while the bar owns the screen */
static int   g_scroll;          /* lines scrolled up from live (0 = live/bottom) */
static int   g_paste_folded;    /* the queued lines are a folded paste -> suppress per-line echo (used by sb_input's hint) */
static char  g_paste_ph[80];    /* the "[Pasted text #N +M lines]" placeholder text currently in the buffer */
static void disable_raw(void) {
    if (g_raw) { ws_("\x1b[?2004l"); tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig); g_raw = 0; }
}

/* Fires once at process exit (atexit). Also resets the DECSTBM scroll region to
   full, so an optional bottom status bar (repl.k's \sb, which sets a region) can
   never leave the terminal scrolled-in on ANY exit path (\\, Ctrl-D, a crash).
   Harmless when no region was set. Kept out of disable_raw() because that runs
   per-readline and the bar's region must persist across prompts. */
static void on_exit_restore(void) {
    /* If the alt screen is still up (a crash / Ctrl-D that bypassed sbb(0)),
     * reset the region WHILE STILL IN IT, then leave.  Never emit \x1b[r after
     * \x1b[?1049l: that runs in the restored PRIMARY screen, where a DECSTBM
     * reset homes the cursor to row 1 and the shell then overlaps the old
     * scrollback.  When the bar already tore down (the \\ path) do nothing here. */
    if (g_sb_alt) { ws_("\x1b[r"); ws_("\x1b[?1006l\x1b[?1000l"); ws_("\x1b[?1007h"); ws_("\x1b[?1049l"); g_sb_alt = 0; }
    disable_raw();
}

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
    ws_("\x1b[?2004h");   /* amber 2.0.0: enable bracketed paste so a pasted script arrives as one unit */
    return 0;
}

static int term_cols(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 4) return ws.ws_col;
    return 80;
}
static int term_rows(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 4) return ws.ws_row;
    return 24;
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

/* ---- Claude-Code-style status bar (amber 2.0.0) --------------------------
 * ln.c OWNS the rendering -- a 2-line panel on the bottom rows plus a DECSTBM
 * scroll region above it -- so the bar survives Ctrl-L, a resize and every
 * keystroke without the interpreter having to redraw it. repl.k only supplies
 * the two content strings (via the `sbb verb). ON by default; \sb turns it off
 * and restores the plain prompt. In a content string, byte 0x01 toggles the
 * accent colour, so repl.k can accent single segments. */
static int  g_sb_on = 0, g_sb_rows = 0;
static char g_sb_panel[640];                    /* the info-panel template (markers below) */
static volatile sig_atomic_t g_spin_on = 0;     /* spinner active during an evaluation */
static int  g_spin_frame = 0;
static int  g_input_plen = 0;                    /* prompt width, for the spinner's column */
static volatile sig_atomic_t g_winch = 0;        /* SIGWINCH: terminal was resized */
static void spin_stop(void);                     /* defined below; stops + erases the spinner */
static void sb_resync(void);                     /* defined below; full rebuild after a resize */
static void on_winch(int s) { (void)s; g_winch = 1; }

/* Truecolour: a warm-muted panel + Amber's own hex-logo amber (#FFB020). Terminals
 * that only do 256 colours fall back to the nearest cell; the layout is unaffected. */
#define SB_BG      "\x1b[48;2;30;27;23m"
#define SB_DIM     "\x1b[38;2;150;142;130m"
#define SB_ACCENT  "\x1b[1;38;2;255;176;0m"      /* the hex logo + `amber x.y.z` */
#define SB_RESET   "\x1b[0m"

/* Claude-Code-style footer: a bordered input box + a dim info line beneath it,
 * fixed at the bottom while output scrolls above.  Four rows, top to bottom:
 *     h-3  box top      ╭──────────────────────────────────╮
 *     h-2  input line   │ amber> <text>                    │   (cursor lives here)
 *     h-1  box bottom   ╰──────────────────────────────────╯
 *      h   info line    ⬡ amber 2.0.0 · exec · mem · [build]        \ help · …
 * The DECSTBM scroll region is rows 1..h-4.  Box glyphs are standard Unicode
 * box-drawing characters (U+2500 family), so any monospace terminal font aligns
 * them; no Nerd Font is required. */
#define SB_FOOT 4                                 /* footer height in rows */
#define BOX_TL "\xe2\x95\xad"                     /* ╭ */
#define BOX_TR "\xe2\x95\xae"                     /* ╮ */
#define BOX_BL "\xe2\x95\xb0"                     /* ╰ */
#define BOX_BR "\xe2\x95\xaf"                     /* ╯ */
#define BOX_H  "\xe2\x94\x80"                     /* ─ */
#define BOX_V  "\xe2\x94\x82"                     /* │ */

/* Markers repl.k embeds in the panel string; ln.c fills / handles them:
 *   0x01 toggle the amber accent   0x02 -> process RSS in MB   0x04 -> build kind
 *   0x03 split point: everything after is right-aligned to the terminal's edge  */

static long sb_rss_mb(void) {                    /* real resident footprint of the process */
#if defined(__linux__)
    FILE *f = fopen("/proc/self/statm", "r");
    long total = 0, res = 0;
    if (!f) return 0;
    if (fscanf(f, "%ld %ld", &total, &res) != 2) res = 0;
    fclose(f);
    return (res * (sysconf(_SC_PAGESIZE) / 1024)) / 1024;   /* pages -> KB -> MB */
#elif defined(__APPLE__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &count) == KERN_SUCCESS)
        return (long)(info.resident_size / (1024 * 1024));   /* bytes -> MB */
    return 0;
#else
    return 0;
#endif
}
static const char *sb_build_kind(void) {
    /* Authoritative: build.sh defines AMBER_BUILD_NATIVE only when a -march/-mcpu
     * =native tuning flag was actually accepted.  (Inferring it from the SIMD
     * backend name was wrong on arm64, where NEON is the baseline and a portable
     * build therefore reported "native".) */
#ifdef AMBER_BUILD_NATIVE
    return "native";
#else
    return "portable";
#endif
}
/* display columns of a UTF-8 string, ignoring ANSI escapes and counting one column
 * per UTF-8 lead byte (correct for the BMP glyphs used here: hex, arrows, mid-dot). */
static int sb_disp(const char *s) {
    int w = 0;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if (*p == 0x1b) { while (*p && *p != 'm') p++; if (!*p) break; continue; }
        if ((*p & 0xc0) != 0x80) w++;
    }
    return w;
}
/* expand markers in a template into rendered bytes (with accent ANSI); returns width */
static int sb_expand(const char *tmpl, char *out, size_t cap) {
    size_t o = 0; int acc = 0; char nb[32];
    #define SBPUT(str) do{ const char *q=(str); while(*q && o+1<cap) out[o++]=*q++; }while(0)
    for (const char *p = tmpl; *p; p++) {
        if ((unsigned char)*p == 1) { acc = !acc; SBPUT(acc ? SB_ACCENT : "\x1b[22m" SB_DIM); continue; }
        if ((unsigned char)*p == 2) { snprintf(nb, sizeof nb, "%ld", sb_rss_mb()); SBPUT(nb); continue; }
        if ((unsigned char)*p == 4) { SBPUT(sb_build_kind()); continue; }
        if (o + 1 < cap) out[o++] = *p;
    }
    if (acc) SBPUT("\x1b[22m" SB_DIM);
    out[o] = 0;
    #undef SBPUT
    return sb_disp(out);
}
/* Write n spaces, in chunks -- NOT through a fixed stack buffer.  A Retina Mac
 * at a zoomed-out font is routinely 300-400 columns wide, and the 256/300-byte
 * buffers this replaces silently CLAMPED the pad there: the info line stopped
 * reaching the right edge, and the input row's closing box glyph was drawn ~313
 * columns in while the top and bottom borders still spanned the full width --
 * a box that looks broken (or doubled) at exactly the widths zooming out gives. */
static void wsp(int n) {
    static char SP[64];
    if (!SP[0]) memset(SP, ' ', sizeof SP);
    while (n > 0) { int k = n > (int)sizeof SP ? (int)sizeof SP : n; wr(SP, (size_t)k); n -= k; }
}
/* one horizontal box-border row (top or bottom), spanning the full width */
static void sb_border(int row, const char *lc, const char *rc) {
    char seq[32]; int cols = term_cols(), i;
    sprintf(seq, "\x1b[%d;1H", row); ws_(seq);
    ws_(SB_DIM); ws_(lc);
    for (i = 0; i < cols - 2; i++) ws_(BOX_H);
    ws_(rc); ws_(SB_RESET);
}
/* the dim info line on row h: left segments + right-aligned shortcuts */
static void sb_infoline(void) {
    int rows = term_rows(), cols = term_cols();
    char left[640], right[320], seq[32];
    const char *split = strchr(g_sb_panel, 3);
    int lw, rw, pad;
    if (split) {
        char lt[640]; size_t n = (size_t)(split - g_sb_panel);
        if (n >= sizeof lt) n = sizeof lt - 1;
        memcpy(lt, g_sb_panel, n); lt[n] = 0;
        lw = sb_expand(lt, left, sizeof left);
        rw = sb_expand(split + 1, right, sizeof right);
    } else { lw = sb_expand(g_sb_panel, left, sizeof left); right[0] = 0; rw = 0; }
    if (g_scroll > 0) {                                /* scroll-back active: take over the whole line */
        int n = snprintf(left, sizeof left,
            "\x1b[1m\xe2\x96\xb2 SCROLL-BACK \xc2\xb7 %d line%s up \xc2\xb7 PgUp/PgDn or wheel \xc2\xb7 any key = live\x1b[0m",
            g_scroll, g_scroll == 1 ? "" : "s");
        lw = (n > 0 && n < (int)sizeof left) ? sb_disp(left) : 0;
        right[0] = 0; rw = 0;
    }
    sprintf(seq, "\x1b[%d;1H", rows); ws_(seq);
    ws_(SB_DIM); ws_("\x1b[K");                        /* dim text on the normal background */
    ws_(left);
    pad = cols - lw - rw;
    if (pad > 0) wsp(pad);
    if (rw && lw + rw <= cols) ws_(right);
    ws_(SB_RESET);
}
/* the box interior on row h-2:  │ <prompt><input> ...pad... │  -- leaves the
 * cursor inside the box at the caret; scrolls a long line horizontally. */
static void sb_input(LnState *l) {
    char seq[64]; int cols = term_cols(), rows = term_rows();
    int inner = cols - 4;                             /* text cols between "│ " and " │" */
    size_t start = 0, show, used, plen = l->plen;
    if (inner < 8) inner = 8;
    g_input_plen = (int)plen;
    if ((int)plen > inner - 2) plen = 0;              /* pathologically narrow width */
    show = l->len;
    while ((int)(plen + (l->pos - start)) >= inner) start++;      /* keep caret visible */
    if ((int)(plen + (show - start)) > inner) show = start + ((size_t)inner - plen);
    sprintf(seq, "\x1b[%d;1H", rows - 2); ws_(seq);
    ws_(SB_DIM); ws_(BOX_V); ws_(SB_RESET); ws_(" ");            /* │ + space (cols 1-2) */
    if (plen) wr(l->prompt, plen);
    wr(l->buf + start, show - start);
    used = plen + (show - start);
    if (l->ghost[0] && l->pos == l->len) {                        /* dim autosuggestion */
        size_t room = (size_t)inner - used, gl = strlen(l->ghost);
        if (gl > room) gl = room;
        if (gl) { ws_("\x1b[2m"); wr(l->ghost, gl); ws_(SB_RESET); used += gl; }
    }
    { int p = inner - (int)used;                                 /* pad the text area */
      static const char HINT[] = "paste again to view";
      int hw = (int)sizeof HINT - 1;
      if (p < 0) p = 0;
      /* when the box holds an unedited paste placeholder and there is room, show a
       * dim right-aligned "paste again to view" hint (Claude-Code style). */
      if (g_paste_folded && g_paste_ph[0] && !strcmp(l->buf, g_paste_ph) && p >= hw + 2) {
          wsp(p - hw - 1);
          ws_("\x1b[2m"); ws_(HINT); ws_(SB_RESET); ws_(" ");
      } else wsp(p);
    }
    ws_(" "); ws_(SB_DIM); ws_(BOX_V); ws_(SB_RESET);            /* space + │ (cols-1, cols) */
    sprintf(seq, "\x1b[%d;%dH", rows - 2, 3 + (int)(plen + (l->pos - start)));
    ws_(seq);                                                    /* caret inside the box */
}
/* draw the static chrome (both borders + the info line), cursor-neutral */
static void sb_chrome(void) {
    int rows = term_rows();
    ws_("\x1b\x37");                                  /* DECSC: save cursor */
    sb_border(rows - 3, BOX_TL, BOX_TR);
    sb_border(rows - 1, BOX_BL, BOX_BR);
    sb_infoline();
    ws_("\x1b\x38");                                  /* DECRC: restore cursor */
}
static void sb_region(void) {
    char seq[32]; int rows = term_rows(); g_sb_rows = rows;
    sprintf(seq, "\x1b[1;%dr", rows - SB_FOOT); ws_(seq);   /* output scrolls in rows 1..h-4 */
}
/* Tear the bar down: release the scroll region and LEAVE the alternate screen,
 * which restores the terminal exactly as it was before the REPL started -- like
 * quitting vim.  No in-place erasing to get wrong after a resize. */
static void sb_teardown(void) {
    ws_("\x1b[r");                                      /* release the scroll region */
    ws_("\x1b[?1006l\x1b[?1000l");                      /* stop reporting the wheel */
    ws_("\x1b[?1007h");                                 /* restore alternate-scroll to the terminal default */
    if (g_sb_alt) { ws_("\x1b[?1049l"); g_sb_alt = 0; } /* leave the alt screen -> original restored */
    ws_("\x1b[?25h");                                   /* cursor visible */
    g_sb_capture = 0; g_scroll = 0; ring_clear();       /* drop the transcript with the bar */
}
/* repl.k calls this each prompt (and on \sb toggle). on=0 tears the footer down. */
void am_ln_statusbar(int on, const char *panel, const char *info) {
    (void)info;
    /* Only a real terminal gets the footer.  A pipe, a redirect, a dumb TERM or
     * AMBER_NO_EDIT=1 uses the plain prompt, so scripted/piped callers never see
     * box-drawing escapes in their output. */
    if (!am_ln_interactive()) return;
    spin_stop();                                         /* an update means evaluation finished */
    if (!on) {
        if (g_sb_on) { g_sb_on = 0; sb_teardown(); }
        return;
    }
    snprintf(g_sb_panel, sizeof g_sb_panel, "%s", panel ? panel : "");
    if (!g_sb_on) {
        ws_("\x1b[?1049h");                              /* first enable: enter the alternate screen */
        ws_("\x1b[?1007l");                              /* disable alternate-scroll (wheel->arrows): we
                                                          * handle the wheel ourselves for scroll-back */
        ws_("\x1b[?1000h\x1b[?1006h");                   /* report the wheel (SGR mouse) so scroll-up can
                                                          * page the internal transcript with the box
                                                          * locked.  Shift bypasses it for text selection. */
        g_sb_alt = 1;
        g_sb_capture = 1;                                /* start teeing ow() into the scroll-back ring */
        ring_clear();
    }
    { static int wired = 0;                              /* watch for resizes while the bar is up */
      if (!wired) { struct sigaction sa; memset(&sa, 0, sizeof sa);
          sa.sa_handler = on_winch; sigaction(SIGWINCH, &sa, NULL); wired = 1; } }
    if (!g_sb_on) { sb_region(); g_sb_on = 1; sb_chrome(); return; }   /* first enable */
    /* A resize that landed while the interpreter was BUSY was never seen by the
     * editor's read loop, so the previous footer is still on screen at its old
     * rows.  Resync before drawing: otherwise this sb_chrome() paints a SECOND
     * box next to the stale one -- the duplicated box seen when zooming. */
    if (g_winch || term_rows() != g_sb_rows) { g_winch = 0; sb_resync(); }
    else sb_chrome();
}

/* ---- execution spinner (status-bar mode only) ----------------------------
 * A query blocks the main thread, so the animation is driven by a repeating
 * SIGALRM.  The handler must be async-signal-safe, so it does no formatting --
 * spin_start() precomputes the ten full escape sequences and the handler just
 * write()s the current one.  It draws the braille frame just past the prompt on
 * the fixed input row (h) while `amber> ` stays put.  Stopped by am_ln_statusbar
 * (repl.k updates the panel the instant evaluation returns). */
static const char *const SPIN[] = {"\xe2\xa0\x8b","\xe2\xa0\x99","\xe2\xa0\xb9","\xe2\xa0\xb8",
    "\xe2\xa0\xbc","\xe2\xa0\xb4","\xe2\xa0\xa6","\xe2\xa0\xa7","\xe2\xa0\x87","\xe2\xa0\x8f"};
static char g_spin_seq[10][64];
static void spin_tick(int sig) {
    (void)sig;
    if (!g_spin_on) return;
    /* The ten escape sequences were precomputed for the geometry in force when
     * the eval started (they must be, to stay async-signal-safe).  Once a resize
     * is pending they point at a row that no longer holds the box, so drawing
     * would scribble inside the scroll region; the imminent resync repaints. */
    if (g_winch) return;
    const char *s = g_spin_seq[g_spin_frame];
    (void)!write(STDOUT_FILENO, s, strlen(s));
    g_spin_frame = (g_spin_frame + 1) % 10;
}
static void spin_start(void) {
    /* draw the braille frame inside the box, just past the prompt (row h-2, the
     * input row; col 3 clears the "│ " border + a space, then g_input_plen).
     * NOTE: driven by SIGALRM on the main thread (a dedicated spinner thread was
     * tried but races with the eval's own fd-1 output). */
    int i, row = (g_sb_rows ? g_sb_rows : term_rows()) - 2, col = g_input_plen + 3;
    if (getenv("AMBER_NO_SPINNER")) return;   /* diagnostic: run without the SIGALRM spinner
                                               * (to check whether it slows a parallel eval) */
    for (i = 0; i < 10; i++)
        /* DECSC/DECRC (ESC 7 / ESC 8) to save+restore the cursor, NOT CSI s/u:
         * macOS Terminal.app doesn't honour CSI s/u, so the cursor was left on the
         * spinner cell and streaming output from `\l` (many statements) landed in
         * the box instead of the scroll region -- i.e. printed nothing visible. */
        snprintf(g_spin_seq[i], sizeof g_spin_seq[i], "\x1b\x37\x1b[%d;%dH%s%s%s\x1b\x38",
                 row, col, SB_ACCENT, SPIN[i], SB_RESET);
    g_spin_frame = 0; g_spin_on = 1;
    { struct sigaction sa; memset(&sa, 0, sizeof sa);
      sa.sa_handler = spin_tick; sa.sa_flags = SA_RESTART; sigaction(SIGALRM, &sa, NULL); }
    { struct itimerval it; it.it_value.tv_sec = 0; it.it_value.tv_usec = 90000;
      it.it_interval = it.it_value; setitimer(ITIMER_REAL, &it, NULL); }
}
static void spin_stop(void) {
    struct itimerval it; char seq[40];
    if (!g_spin_on) return;
    g_spin_on = 0;
    memset(&it, 0, sizeof it); setitimer(ITIMER_REAL, &it, NULL);
    signal(SIGALRM, SIG_IGN);
    snprintf(seq, sizeof seq, "\x1b[%d;%dH ", (g_sb_rows ? g_sb_rows : term_rows()) - 2, g_input_plen + 3);
    (void)!write(STDOUT_FILENO, seq, strlen(seq));       /* erase the spinner cell in the box */
}

static void refresh(LnState *l) {
    char seq[64];
    size_t start = 0, show, plen = l->plen;
    size_t cols = (size_t)(l->cols = term_cols());

    if (g_sb_on) { sb_input(l); return; }       /* box interior + caret; chrome is already up */

    g_input_plen = (int)plen;                   /* remember for the spinner column */
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

static void accept_ghost(LnState *l) {
    if (!l->ghost[0]) return;
    ins(l, l->ghost, strlen(l->ghost));
    am_ext_set_accepted(l->buf);   /* an extension may treat this as feedback */
    l->ghost[0] = 0;
}

/* ---- the editor loop ----------------------------------------------------- */

/* ---- bracketed paste (amber 2.0.0) --------------------------------------
 * A pasted multi-line script is run line by line, exactly as if each line had
 * been typed and Enter pressed -- so trailing (`x:1 / c`) and full-line (`/ c`)
 * comments work, and the whole block is interpreted. The first pasted line is
 * submitted immediately; the rest are queued and drained by the next
 * am_ln_readline() calls (each echoed under the prompt). The whole-block eval
 * path (`. text`) was rejected: it raises 'limit on a multi-statement string. */
static char **g_pq; static int g_pq_n, g_pq_i;
static int g_paste_no;       /* running [Pasted text #N] counter */
static char *g_paste_raw;    /* verbatim text of the last folded paste (for the Ctrl-V-Ctrl-V preview) */
static int g_paste_raw_n;    /* its physical-line count */
static char *g_last_paste;   /* the raw bytes of the last folded paste, for "paste again to view" detection */

/* ---- scroll-back ring helpers (state declared up top) --------------------- */
static void ring_push(const char *s, int n) {
    char *d;
    if (n < 0) n = 0;
    d = (char *)malloc((size_t)n + 1);
    if (!d) return;
    memcpy(d, s, (size_t)n); d[n] = 0;
    if (g_ring[g_ring_head]) free(g_ring[g_ring_head]);
    g_ring[g_ring_head] = d;
    g_ring_head = (g_ring_head + 1) % SB_RING;
    if (g_ring_n < SB_RING) g_ring_n++;
}
/* logical index 0 = oldest retained line */
static const char *ring_get(int idx) {
    int start;
    if (idx < 0 || idx >= g_ring_n) return "";
    start = ((g_ring_head - g_ring_n) % SB_RING + SB_RING) % SB_RING;
    return g_ring[(start + idx) % SB_RING];
}
static void ring_clear(void) {
    int i; for (i = 0; i < SB_RING; i++) { free(g_ring[i]); g_ring[i] = NULL; }
    g_ring_n = g_ring_head = g_ring_plen = 0; g_scroll = 0;
}
/* Called from m.c's ow() for every kernel stdout write. Splits on '\n' into
 * ring lines; a bare '\r' (in-place redraw) restarts the current line. */
void am_ln_sb_capture(const char *s, size_t n) {
    size_t i;
    if (!g_sb_capture) return;
    for (i = 0; i < n; i++) {
        char c = s[i];
        if (c == '\n')      { ring_push(g_ring_part, g_ring_plen); g_ring_plen = 0; }
        else if (c == '\r') { g_ring_plen = 0; }
        else if (g_ring_plen < (int)sizeof g_ring_part - 1) g_ring_part[g_ring_plen++] = c;
    }
}
/* Paint the scroll region (rows 1..h-SB_FOOT) from the ring at the current
 * offset.  The footer rows are never touched, so the box stays locked; auto-wrap
 * is off for the duration so an over-wide line truncates instead of spilling
 * onto the box or scrolling the region. */
static void sb_repaint_region(void) {
    int rows = term_rows(), cols = term_cols(), rh = rows - SB_FOOT, i, bottom;
    char seq[48];
    if (rh < 1) return;
    /* Ring line shown on the region's last row.  AT LIVE that row is the EMPTY
     * one the next line of output will land on -- which is exactly how the screen
     * looks after ordinary output has scrolled -- so the newest retained line
     * belongs one row higher.  Getting this wrong let the next prompt echo
     * (printed at row h-SB_FOOT) overwrite the last transcript line instead of
     * scrolling it up.  While SCROLLED BACK, fill the last row too, so PgUp shows
     * as much of the transcript as the region can hold. */
    bottom = g_ring_n - 1 - g_scroll + (g_scroll ? 0 : 1);
    ws_("\x1b\x37");                          /* save cursor */
    ws_("\x1b[?7l");                          /* no auto-wrap while we paint */
    for (i = 0; i < rh; i++) {
        int line = bottom - (rh - 1 - i);
        sprintf(seq, "\x1b[%d;1H\x1b[2K", i + 1); ws_(seq);
        if (line >= 0 && line < g_ring_n) { const char *L = ring_get(line); wr(L, strlen(L)); ws_("\x1b[0m"); }
    }
    (void)cols;
    ws_("\x1b[?7h");                          /* restore auto-wrap for live output */
    sb_infoline();                            /* info line shows the scroll-back indicator */
    ws_("\x1b\x38");                          /* restore cursor */
}
/* Rebuild the entire screen for the CURRENT terminal geometry.  Called for every
 * SIGWINCH.
 *
 * It deliberately assumes NOTHING about what the terminal did to the old
 * contents, because terminals disagree: xterm clips the alternate screen at the
 * bottom, Terminal.app and iTerm2 keep the bottom and shift content up, and they
 * differ again on whether DECSTBM survives a resize at all.  Any repaint that
 * erases "wherever the old footer was" therefore leaves a stranded copy of the
 * box on some terminal -- which is exactly the duplicated box seen when zooming
 * (Cmd-+ / Cmd--) on macOS, where a zoom changes rows AND columns and fires a
 * burst of SIGWINCHes.
 *
 * So: release the region, wipe the WHOLE screen, re-set the region for the new
 * size, and repaint the transcript from the ring.  Nothing is lost -- the bar
 * owns the alternate screen and the ring holds every line printed since it came
 * up -- and the result is identical on every terminal. */
static void sb_resync(void) {
    int rh, maxs;
    if (!g_sb_on) return;
    ws_("\x1b[r");                     /* release the old region, whatever it was */
    ws_("\x1b[H\x1b[2J");              /* wipe everything: no stale footer can survive */
    g_sb_rows = 0;
    sb_region();                       /* scroll region for the NEW geometry */
    rh = term_rows() - SB_FOOT;
    maxs = g_ring_n - rh; if (maxs < 0) maxs = 0;
    if (g_scroll > maxs) g_scroll = maxs;   /* fewer rows may mean less scroll-back */
    sb_repaint_region();               /* transcript, re-laid-out at the new width */
    sb_chrome();
}
static void sb_scroll_by(int delta) {
    int rows = term_rows(), rh = rows - SB_FOOT, maxs, old = g_scroll;
    if (!g_sb_on) return;
    maxs = g_ring_n - rh; if (maxs < 0) maxs = 0;   /* can't scroll past the oldest retained line */
    g_scroll += delta;
    if (g_scroll < 0) g_scroll = 0;
    if (g_scroll > maxs) g_scroll = maxs;
    if (g_scroll == old) return;                    /* already at a boundary (e.g. scroll down at live) -> leave the screen alone */
    sb_repaint_region();
}
static void sb_scroll_reset(void) {           /* snap back to live and repaint the tail */
    if (!g_scroll) return;
    g_scroll = 0;
    sb_repaint_region();
}
static void pq_clear(void){ while(g_pq_i<g_pq_n) free(g_pq[g_pq_i++]); free(g_pq); g_pq=NULL; g_pq_n=g_pq_i=0; }

/* Net bracket depth a physical line adds, ignoring brackets inside "..." strings
 * and after a `/ ` comment.  A line that leaves depth > 0 (an open {, ( or [ )
 * is continued by the next line, so a pasted multi-line function/list/qSQL is
 * rejoined into one logical statement before it is evaluated. */
static int sb_depth_delta(const char *s) {
    int d = 0, instr = 0;
    for (const char *p = s; *p; p++) {
        char c = *p;
        if (instr) { if (c == '\\' && p[1]) p++; else if (c == '"') instr = 0; continue; }
        if (c == '"') { instr = 1; continue; }
        if (c == '/' && (p == s || p[-1] == ' ' || p[-1] == '\t')) break;  /* comment to EOL */
        if (c == '(' || c == '[' || c == '{') d++;
        else if (c == ')' || c == ']' || c == '}') d--;
    }
    return d;
}

/* Truncate a physical line at the start of a trailing `/ ` line comment (ignoring
 * a `/` inside a "..." string).  A continued line is comment-stripped before the
 * next line is space-joined onto it, so the comment can't swallow that line. */
static void sb_strip_comment(char *s) {
    int instr = 0;
    for (char *p = s; *p; p++) {
        char c = *p;
        if (instr) { if (c == '\\' && p[1]) p++; else if (c == '"') instr = 0; continue; }
        if (c == '"') { instr = 1; continue; }
        if (c == '/' && (p == s || p[-1] == ' ' || p[-1] == '\t')) { *p = 0; return; }
    }
}

/* One byte from the keyboard, retrying on EINTR.  A SIGWINCH -- and a macOS zoom
 * fires a BURST of them -- or the spinner's SIGALRM must never truncate a
 * multi-byte escape sequence or a bracketed paste: the unread remainder would
 * then be parsed as, or inserted as, literal text, which is how a resize during
 * a paste or a trackpad scroll ends up corrupting the input line.  Returns 1, or
 * 0 at EOF / a real error.  The MAIN read loop deliberately does NOT use this:
 * it wants the EINTR so it can service the resize promptly. */
static int rd1(char *c) {
    for (;;) {
        ssize_t k = read(STDIN_FILENO, c, 1);
        if (k == 1) return 1;
        if (k < 0 && errno == EINTR) continue;
        return 0;
    }
}

static void paste_preview(LnState *l);   /* defined below; read_paste calls it for "paste again to view" */
static int read_paste(LnState *l) {
    size_t cap = 1024, n = 0; char *p = (char*)malloc(cap);
    if (!p) return 0;
    for (;;) {
        char c; if (!rd1(&c)) break;
        if (c == 27) {                                   /* possible ESC[201~ end */
            char e; if (!rd1(&e)) break;
            if (e == '[') { int num = 0; char t = 0;
                while (rd1(&t) && t >= '0' && t <= '9') num = num*10 + (t-'0');
                if (t == '~' && num == 201) break;       /* end of paste */
            }
            continue;                                    /* ignore any other seq inside a paste */
        }
        if (c == '\r') c = '\n';
        if (n + 2 >= cap) { cap *= 2; char *q = (char*)realloc(p, cap); if (!q) { free(p); return 0; } p = q; }
        p[n++] = c;
    }
    p[n] = 0;
    /* Claude-Code-style "paste again to view": if the SAME content is pasted again
     * while its placeholder is still in the box, don't fold a second time -- show
     * the full text so the user can read it (amber's box is one line, so it shows
     * as an overlay above the box rather than expanding in place). */
    if (g_last_paste && g_paste_folded && !strcmp(l->buf, g_paste_ph) && !strcmp(p, g_last_paste)) {
        free(p);
        paste_preview(l);
        return 0;                                        /* placeholder stays; Enter still runs the batch */
    }
    free(g_last_paste); g_last_paste = strdup(p);        /* remember this paste for re-paste detection */
    size_t cl = l->len;                                  /* prepend the current line */
    char *comb = (char*)malloc(cl + n + 1);
    if (!comb) { free(p); return 0; }
    memcpy(comb, l->buf, cl); memcpy(comb + cl, p, n + 1); free(p);
    free(g_paste_raw); g_paste_raw = strdup(comb);       /* keep a verbatim copy for the preview */
    pq_clear();
    int cap2 = 8, ns = 0; char **segs = (char**)malloc(cap2 * sizeof *segs);
    for (char *q = comb;;) {
        char *nl = strchr(q, '\n'); if (nl) *nl = 0;
        if (ns >= cap2) { cap2 *= 2; segs = (char**)realloc(segs, cap2 * sizeof *segs); }
        segs[ns++] = q;
        if (!nl) break; q = nl + 1;
    }
    if (ns > 1 && segs[ns-1][0] == 0) ns--;              /* drop trailing empty from a final newline */
    if (ns > 1) {
        /* Fold the whole block behind a placeholder; rejoin physical lines into
         * complete logical statements (a line that leaves a bracket open pulls in
         * the following lines) so multi-line functions, lists and qSQL evaluate as
         * one unit.  The batch runs on Enter, echoed only as the placeholder. */
        pq_clear();
        g_pq = (char**)malloc((size_t)ns * sizeof *g_pq);
        for (int i = 0; i < ns; ) {
            int depth = sb_depth_delta(segs[i]);
            if (depth <= 0) { g_pq[g_pq_n++] = strdup(segs[i]); i++; continue; } /* single line, verbatim */
            sb_strip_comment(segs[i]);                   /* multi-line: strip comment, then space-join */
            char *stmt = strdup(segs[i]);
            i++;
            while (depth > 0 && i < ns) {                /* bracket still open -> keep pulling lines */
                sb_strip_comment(segs[i]);
                char *cont = segs[i]; while (*cont == ' ' || *cont == '\t') cont++;
                stmt = (char*)realloc(stmt, strlen(stmt) + strlen(cont) + 2);
                strcat(stmt, " "); strcat(stmt, cont);
                depth += sb_depth_delta(segs[i]);
                i++;
            }
            g_pq[g_pq_n++] = stmt;                        /* one logical statement (joined with spaces) */
        }
        g_paste_folded = 1;
        g_paste_raw_n = ns;                              /* remember line count for the preview header */
        snprintf(g_paste_ph, sizeof g_paste_ph, "[Pasted text #%d +%d lines]", ++g_paste_no, ns);
        set_line(l, g_paste_ph);                         /* the buffer now holds exactly the placeholder */
        free(segs); free(comb);
        return 0;    /* keep editing: the user sees the placeholder, submits on Enter */
    }
    free(g_paste_raw); g_paste_raw = NULL; g_paste_raw_n = 0;  /* single-line: nothing to preview */
    set_line(l, segs[0]);
    free(segs); free(comb);
    return 0;        /* single-line paste -> just drop it into the edit buffer */
}

/* Reveal the last folded paste in full: a header rule, then every physical line
 * at NORMAL width behind a dim line-number gutter -- so the whole line is visible
 * and it renders identically on every terminal (no DECDWL double-width, which cut
 * lines in half and rendered badly on Windows Terminal / VS Code).  In status-bar
 * mode it scrolls inside the region above the fixed box; otherwise above the
 * prompt.  The placeholder line is redrawn untouched afterwards. */
static void paste_preview(LnState *l) {
    int rows = term_rows(), cols = term_cols(), shown = 0, cap, ln = 0, k;
    char *s, *nl; char seq[32];
    if (!g_paste_raw || !g_paste_raw_n) { ws_("\x07"); return; }   /* nothing to show -> bell */
    cap = g_sb_on ? rows - SB_FOOT - 2 : rows - 2;                 /* leave room on screen */
    if (cap < 3) cap = 3;
    if (g_sb_on) { sprintf(seq, "\x1b[%d;1H", rows - SB_FOOT); ws_(seq); }
    else ws_("\r\n");
    ws_("\x1b[?7l");                                               /* no auto-wrap: long lines truncate cleanly */
    { char hdr[80]; int hw = snprintf(hdr, sizeof hdr, "\xe2\x94\x80\xe2\x94\x80 Pasted text #%d  (%d lines) ", g_paste_no, g_paste_raw_n);
      ws_(SB_ACCENT); ws_(hdr);
      for (k = (hw > 0 ? sb_disp(hdr) : 0); k < cols; k++) ws_("\xe2\x94\x80");   /* rule to the edge */
      ws_(SB_RESET); ws_("\r\n"); }
    for (s = g_paste_raw; s && *s; ) {
        nl = strchr(s, '\n');
        int len = nl ? (int)(nl - s) : (int)strlen(s);
        if (shown >= cap) {                                       /* ran out of room */
            char more[48]; snprintf(more, sizeof more, "  \xe2\x80\xa6 %d more line%s", g_paste_raw_n - shown, g_paste_raw_n - shown == 1 ? "" : "s");
            ws_(SB_DIM); ws_(more); ws_(SB_RESET); ws_("\r\n");
            break;
        }
        { char g[16]; snprintf(g, sizeof g, "%4d ", ++ln); ws_(SB_DIM); ws_(g); ws_(SB_RESET); }  /* line-number gutter */
        if (len > cols - 6) len = cols - 6;                       /* keep one screen row per line */
        if (len > 0) wr(s, (size_t)len);
        ws_("\r\n");
        shown++;
        if (!nl) break; s = nl + 1;
    }
    ws_("\x1b[?7h");                                              /* restore auto-wrap */
    if (g_sb_on) sb_chrome();                                     /* restore box borders + info line */
    refresh(l);                                                   /* redraw the input (placeholder) */
}

/* ---- exec timer -----------------------------------------------------------
 * A precise, monotonic wall-clock stopwatch for the status bar's "exec" figure.
 * CLOCK_MONOTONIC is the right clock for a stopwatch: it never steps under NTP
 * and (on real hardware) advances at true wall rate.  g_mono_start_ms is stamped
 * the instant a line is handed to the interpreter, and repl.k reads `sbt[] (this
 * clock, in ms) both just before and just after the eval and subtracts -- so the
 * figure is the eval's true wall time, measured entirely in-interpreter and
 * excluding hook/cleanup overhead.  The previous basis was gettimeofday, which
 * over-counted a threaded eval on macOS. */
static double g_mono_start_ms;
static double mono_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1.0e6;
}
static void   exec_timer_begin(void) { g_mono_start_ms = mono_ms(); }
double        am_ln_exec_ms(void)    { return mono_ms() - g_mono_start_ms; }

char *am_ln_readline(const char *prompt) {
    LnState l;
    char *out;

    memset(&l, 0, sizeof l);
    l.prompt = prompt ? prompt : "";
    l.plen   = strlen(l.prompt);
    l.cols   = term_cols();
    l.hidx   = g_hist_n;

    if (enable_raw() < 0) return NULL;
    if (g_sb_on) sb_chrome();                /* draw the box + info line before the first keystroke */
    refresh(&l);

    int cv_armed = 0;                        /* one Ctrl-V seen; a second one triggers the paste preview */
    for (;;) {
        char c;
        ssize_t k;
        if (g_winch) {                       /* terminal resized (a zoom, a window drag) */
            g_winch = 0;
            sb_resync();                     /* no-op unless the bar owns the screen */
            refresh(&l);
        }
        k = read(STDIN_FILENO, &c, 1);
        if (k < 0 && errno == EINTR) continue;   /* SIGWINCH/SIGALRM woke us: loop -> handle winch */
        if (k <= 0) { disable_raw();                 /* EOF (Ctrl-D) or read error */
            if (l.len) break;                        /* content pending -> treat as Enter */
            if (g_sb_on) { g_sb_on = 0; sb_teardown(); }  /* abrupt exit never called sbb(0) */
            return NULL; }

        /* Any keystroke other than an explicit accept discards the ghost. */
        if (l.ghost[0] && c != 9 && c != 6 && c != 27) l.ghost[0] = 0;
        if (c != 22 && c != 27) cv_armed = 0;    /* the preview pair (Ctrl-V or Alt-V) must be consecutive */

        switch (c) {
        case 9:                                  /* Tab -- intentionally ignored.
                                                  * Tab completion was REMOVED at the
                                                  * user's explicit request: with amber/k's
                                                  * terse syntax it was near-useless and
                                                  * uncomfortable to use.  Tab is a no-op. */
            continue;
        case 22:                                 /* Ctrl-V: press twice to preview the last paste */
            if (cv_armed) { cv_armed = 0; paste_preview(&l); }
            else cv_armed = 1;
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
        case 12:                                                /* Ctrl-L: clear upper area, keep footer */
            ws_("\x1b[H\x1b[2J");
            if (g_sb_on) { sb_region(); sb_chrome(); }
            break;
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
            if (!rd1(&s[0])) break;
            if (s[0] == 'v' || s[0] == 'V') {                   /* Alt-V: same as Ctrl-V, for terminals
                                                                 * (WSL / Windows Terminal) that eat Ctrl-V.
                                                                 * Press twice to preview the last paste. */
                if (cv_armed) { cv_armed = 0; paste_preview(&l); }
                else cv_armed = 1;
                l.ghost[0] = 0;
                continue;
            }
            cv_armed = 0;                                       /* any other escape seq breaks the pair */
            if (!rd1(&s[1])) break;
            if (s[0] == '[') {
                if (s[1] == '<') {                          /* SGR mouse report: \x1b[<b;x;y(M|m) */
                    int pb = 0; char t = 0;
                    while (rd1(&t) && t >= '0' && t <= '9') pb = pb*10 + (t-'0');
                    while (t && t != 'M' && t != 'm') { if (!rd1(&t)) break; }
                    if (g_sb_on) {                          /* scroll the transcript, box stays locked */
                        if (pb == 64)      { sb_scroll_by(+3); refresh(&l); }  /* wheel up   -> older */
                        else if (pb == 65) { sb_scroll_by(-3); refresh(&l); }  /* wheel down -> newer */
                    }
                    l.ghost[0] = 0;
                    continue;                               /* the wheel never edits the line */
                } else if (s[1] == 'M') {                   /* legacy X10 mouse: \x1b[M b x y (3 bytes) */
                    unsigned char mb[3]; int got = 0;
                    while (got < 3 && rd1((char *)&mb[got])) got++;
                    if (got == 3 && g_sb_on) {
                        int b = (mb[0] - 32) & 0x43;        /* button bits incl. the wheel flag (64) */
                        if (b == 64)      { sb_scroll_by(+3); refresh(&l); }   /* wheel up */
                        else if (b == 65) { sb_scroll_by(-3); refresh(&l); }   /* wheel down */
                    }
                    l.ghost[0] = 0;
                    continue;
                } else if (s[1] >= '0' && s[1] <= '9') {
                    /* accumulate the numeric parameter so multi-digit codes work:
                       200~/201~ are bracketed paste, 3~ Delete, 1~/7~ Home, 4~/8~ End */
                    int num = s[1] - '0'; char t = 0;
                    while (rd1(&t) && t >= '0' && t <= '9') num = num*10 + (t-'0');
                    if (t == '~') {
                        if (num == 200) { if (read_paste(&l)) goto done; }  /* pasted block */
                        else if ((num == 5 || num == 6) && g_sb_on) {        /* PageUp / PageDown: scroll-back */
                            int page = term_rows() - SB_FOOT - 1; if (page < 1) page = 1;
                            sb_scroll_by(num == 5 ? +page : -page);         /* 5 = up (older), 6 = down */
                            refresh(&l);
                            l.ghost[0] = 0;
                            continue;                                       /* don't snap back to live */
                        }
                        else if (num == 3) del_right(&l);                   /* Delete */
                        else if (num == 1 || num == 7) l.pos = 0;
                        else if (num == 4 || num == 8) l.pos = l.len;
                        /* num == 201: stray paste-end, ignore */
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
        if (g_sb_on) sb_scroll_reset();          /* any edit key snaps the view back to live */
        refresh(&l);
    }

done:
    exec_timer_begin();                              /* start the exec stopwatch at submission */
    disable_raw();
    if (g_sb_on) {
        /* Commit the typed line to the scrolling transcript (region bottom = h-4),
         * empty the box, then spin while repl.k evaluates.  The cursor is parked on
         * the region's last row so the output repl.k is about to print scrolls
         * INSIDE the region, above the fixed box+info footer. */
        char dseq[48]; int rows = term_rows();
        LnState e;
        g_scroll = 0;                                                 /* commit at the live tail */
        sprintf(dseq, "\x1b[%d;1H", rows - SB_FOOT); ws_(dseq);       /* row h-4 */
        ws_(SB_ACCENT); if (l.plen) wr(l.prompt, l.plen); ws_(SB_RESET);
        wr(l.buf, l.len);
        ws_("\r\n");                                                  /* scroll the region up */
        {   /* mirror the echoed prompt+command into the scroll-back transcript */
            char *ce = (char *)malloc(strlen(SB_ACCENT) + l.plen + strlen(SB_RESET) + l.len + 1);
            if (ce) { ce[0] = 0; strcat(ce, SB_ACCENT); if (l.plen) strncat(ce, l.prompt, l.plen);
                      strcat(ce, SB_RESET); strncat(ce, l.buf, l.len);
                      ring_push(ce, (int)strlen(ce)); free(ce); }
        }
        memset(&e, 0, sizeof e); e.prompt = l.prompt; e.plen = l.plen;
        sb_input(&e);                                                 /* redraw the box empty */
        g_input_plen = (int)l.plen;
        sprintf(dseq, "\x1b[%d;1H", rows - SB_FOOT); ws_(dseq);       /* park cursor in the region */
        spin_start();                                                 /* braille spinner inside the box */
    } else {
        ws_("\r\n");
    }
    /* If the user edited or deleted the placeholder, they did NOT mean to run the
     * folded batch -- abandon it and fall through to evaluating whatever they left. */
    if (g_paste_folded && strcmp(l.buf, g_paste_ph) != 0) { g_paste_folded = 0; pq_clear(); }
    if (g_paste_folded && g_pq_i < g_pq_n) {
        /* Folded paste: hand repl.k the whole batch in one line, statements joined
         * by 0x1e (record separator).  repl.k splits on it and evaluates each in
         * turn, so the batch runs as a unit without depending on the read loop
         * being re-entered per statement.  The placeholder is already on screen. */
        size_t tot = 0; int i; char *w;
        for (i = g_pq_i; i < g_pq_n; i++) tot += strlen(g_pq[i]) + 1;
        out = (char *)malloc(tot + 1);
        if (!out) return NULL;
        w = out;                                         /* O(n) join (strcat would be O(n^2) on a huge paste) */
        for (i = g_pq_i; i < g_pq_n; i++) {
            size_t li = strlen(g_pq[i]);
            if (i > g_pq_i) *w++ = '\x1e';
            memcpy(w, g_pq[i], li); w += li;
        }
        *w = 0;
        pq_clear(); g_paste_folded = 0;
        return out;                                      /* don't add the batch to history */
    }
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

/* (The REPL-command / K-vocabulary tables and token_start() that fed Tab
 * completion were removed with it, at the user's request.) */

/* NOTE: the Tab-completion engine (repl_complete / add_word and the do_tab /
 * list_candidates / common_prefix / ext_ghost helpers above) was REMOVED at the
 * user's explicit request -- with amber/k's terse syntax it was near-useless and
 * uncomfortable.  Tab is now a no-op; no completion source is consulted. */

/* ---- REPL entry points --------------------------------------------------- */

static char g_histpath[512];

void am_repl_init(void) {
    static int done;
    const char *h;
    if (done) return;
    done = 1;
    am_ext_startup_once();
    /* Tab completion removed at the user's explicit request (clunky with k syntax);
     * no completion callback is registered, so Tab does nothing. */
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
    exec_timer_begin();                              /* start the exec stopwatch at submission */
    return out;
}

#endif /* wasm */
