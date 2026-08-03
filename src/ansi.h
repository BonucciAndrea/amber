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

#endif /* AMBER_ANSI_H */
