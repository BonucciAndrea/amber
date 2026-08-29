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
 * the codes. amber 2.0.0: this is the SAME warm truecolor palette the status bar
 * uses -- amber #FFB020 for accents, muted #968E82 for structure -- plus one
 * warm red (#FF5F57) reserved for the actual error and its caret, so a diagnostic
 * reads as part of the same UI instead of a 16-colour rainbow.  Truecolor
 * terminals get the exact hues; 256/16-colour terminals fall back to the nearest
 * cell, and the layout is unaffected either way. */
#define ANSI_ERR  "\x1b[1;38;2;255;95;87m"   /* warm red    : error[CODE]           */
#define ANSI_WARN "\x1b[1;38;2;255;176;0m"   /* amber       : warning / token label */
#define ANSI_TTL  "\x1b[1;38;2;232;226;214m" /* warm white  : the title line        */
#define ANSI_LOC  "\x1b[38;2;150;142;130m"   /* muted       : --> locator, gutter | */
#define ANSI_CAR  "\x1b[1;38;2;255;95;87m"   /* warm red    : ^^^ primary underline */
#define ANSI_SEC  "\x1b[1;38;2;214;158;74m"  /* muted amber : ~~~ secondary spans   */
#define ANSI_NUM  "\x1b[38;2;150;142;130m"   /* muted       : gutter line numbers   */
#define ANSI_HLP  "\x1b[1;38;2;255;176;0m"   /* amber       : = help:               */
#define ANSI_NOTE "\x1b[38;2;150;142;130m"   /* muted       : = note:               */
#define ANSI_DIM  "\x1b[2m"                   /* dim         : de-emphasised text    */

#endif /* AMBER_ANSI_H */
