# `ext/` — out-of-tree extensions

This directory is **empty in a stock checkout**, and Amber is complete without
it. It exists so that a separate package can add verbs, REPL commands and
editor behaviour to *this* installation without patching a single line of
`src/`.

## The protocol

1. Drop one or more `.c` files here.
2. Run `./build.sh` (or just `./a`, which rebuilds when sources are newer).

`build.sh` compiles `ext/*.c` with the same flags as `src/*.c` and links them
into the same binary. It also drops object files whose source has gone, so
removing a file here and rebuilding is a clean uninstall.

## The seam

Everything an extension can hook is declared and documented in
[`../src/ext.h`](../src/ext.h): a runtime verb registry, a `\`-command hook, an
editor hint (ghost text) hook, an extra Tab-completion source, a startup hook
and two cosmetic strings. A file registers itself from a constructor:

```c
#include "a.h"
#include "ext.h"

static A my_verb(A x) { mr(x); return ai(7); }

__attribute__((constructor))
static void reg(void) { am_ext_verb("pr7", (void *)my_verb); }
```

`tests/ext_probe.c` is a complete worked example that exercises every hook, and
`tests/test_ext_seam.sh` installs it, checks it, and uninstalls it again.

## Amber-level extensions

The `.k` half of an extension goes in `../lib/`. `repl.k` loads `lib/ext.k`
whole-file at startup if it exists, fully trapped and with diagnostics off, so
a missing or broken extension library leaves the REPL exactly as it was. An
extension may define any of these root-namespace functions, all optional:

| name | called | purpose |
|---|---|---|
| `ext.pre x`  | before each REPL line runs | e.g. reset per-line state |
| `ext.post x` | after each REPL line runs  | e.g. record what succeeded |
| `ext.err x`  | when a REPL line raises    | capture the error for later |
| `ext.raw x`  | before the qSQL rewriter   | return 1 to pass the line through verbatim |
| `ext.tag[]`  | at banner time             | a short string appended to the banner |

## Known extensions

* [`amber-ai`](https://github.com/bonucciandrea/amber-ai) — a local, offline
  AI co-pilot: schema-aware Tab suggestions, `\ai why`, `\ai profile`.
