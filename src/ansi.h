/* ansi.h  -  shared ANSI SGR reset code for Amber's REPL diagnostic output.
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * ast.c and diagnostic.c each defined their own identical `C_RST "\x1b[0m"`;
 * this header is the one place it's spelled out now. Module-specific color
 * palettes (ast.c's verb/var/scalar colors, diagnostic.c's error/title/help
 * colors) stay local to each file since they don't overlap.
 */
#ifndef AMBER_ANSI_H
#define AMBER_ANSI_H

#define ANSI_RST "\x1b[0m"

/* ---- shared diagnostic palette (src/diagnostic.c) -------------------------
 * Kept here rather than in diagnostic.c so a future consumer (a REPL banner, a
 * notebook renderer) can match the error styling exactly instead of guessing at
 * the codes. Chosen for legibility on BOTH dark and light terminals: the bright
 * variants (9x/6x) stay readable on white, which plain 3x red does not. */
#define ANSI_ERR  "\x1b[1;91m"  /* bold bright red   : error[CODE]           */
#define ANSI_WARN "\x1b[1;93m"  /* bold bright yellow: warning / token label */
#define ANSI_TTL  "\x1b[1;97m"  /* bold bright white : the title line        */
#define ANSI_LOC  "\x1b[1;96m"  /* bold bright cyan  : --> locator, gutter | */
#define ANSI_CAR  "\x1b[1;91m"  /* bold bright red   : ^^^ primary underline */
#define ANSI_SEC  "\x1b[1;94m"  /* bold bright blue  : ~~~ secondary spans   */
#define ANSI_NUM  "\x1b[2;37m"  /* dim grey          : gutter line numbers   */
#define ANSI_HLP  "\x1b[1;92m"  /* bold bright green : = help:               */
#define ANSI_NOTE "\x1b[2;37m"  /* dim grey          : = note:               */
#define ANSI_DIM  "\x1b[2m"     /* dim               : de-emphasised text    */

#endif /* AMBER_ANSI_H */
