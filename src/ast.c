/* ast.c  -  Amber AST visualizer ("\ast <expression>"). See ast.h.
 *
 * Walks pk()'s raw (uncompiled) parse tree using the same shape dispatch
 * cr() uses to compile it (see b.c) -- but prints instead of emitting
 * bytecode. Only the parameterized, explicit-argument accessor macros from
 * a.h/g.h are used (_t, _n, _v, _t0, _x, _y, _A, TU, ...), never the bare
 * `x`-only convenience macros (xn, xt, xa, ...), so nothing here depends on
 * a local variable happening to be named `x`.
 *
 * Reading values is done with the raw, non-consuming accessors (_F/_L/_v,
 * su()) rather than gf()/gl() -- those *unref* their argument, which would
 * be wrong here since \ast only inspects pk()'s tree, it doesn't own it.
 *
 * ---- this revision fixes a real class of bugs -------------------------
 * The previous version's fallback formatter (fmt_scalar) only understood
 * five atom tags (ts/ti/tl/tf/tc) and treated *everything else* -- multi-
 * element data vectors (tI/tL/tF/tS/tC/tB/tG/tH), verb/adverb atoms used as
 * bare values (tu/tv/tw), lambda/projection/composition/derived-verb
 * values (to/tp/tq/tr), and the GAP sentinel that marks a curried-away
 * argument slot -- as a generic "bare atom" and printed a useless
 * "<X-atom>" placeholder (X = the raw type-tag character). That garbage
 * showed up constantly: any literal vector (`1 2 3`), any lambda body
 * (`{x+1}`), any curried projection (`1+`, `f[x;;z]`), and any tacit hook
 * or fork (`(f g)`, `(f g h)`) whose train elements got walked one at a
 * time. This revision gives every one of those shapes its own explicit,
 * correctly-typed rendering -- see describe_leaf()/describe_vector() below.
 *
 * It also fixes a real memory-safety bug in the *old* namespaced-identifier
 * path: a Symbol Vector (`tS`, e.g. the parsed form of `.ns.sub`, or a
 * literal `` `a`b`c ``) stores its elements as PACKED 32-BIT symbol ids
 * (`I`), not boxed 64-bit `A` values -- the old code read it via `_A(v)[i]`
 * (an `A*`/8-byte-stride cast), which silently read the wrong bytes and
 * printed garbled/duplicated names (`.ns.sub` rendered as "ns.ns"). Fixed
 * by reading through `(const I*)_V(v)` (4-byte stride) everywhere a tS
 * vector's elements are touched.
 */
#include "a.h"
#include "ast.h"
#include "ansi.h"
#include "arena.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
/* No <math.h> needed: fmt_float() below detects NaN/Infinity with plain
 * float comparison tricks (f!=f, f*2==f) instead of isnan()/isinf() --
 * <math.h> pulls in glibc's <bits/mathcalls.h>, which collides with a.h's
 * bare short macros (F, _n, ...) if a.h has already been included (a.h has
 * no #include guard and is designed to be the first "real" header in a
 * translation unit); simplest fix is just not needing it. */

/* ---- verb / adverb name tables ------------------------------------------
 * Index-parallel to the v1[]/v2[] builtin tables in a.h's V_ macro. Amber's
 * own monad/dyad *enum constants* (au=FLP=NEG=...=OUT, av=ADD=SUB=...=GAP)
 * are themselves the exact tagged atoms pk() embeds as verb heads, and
 * because they're declared sequentially their VALUE (_v(head)) is already
 * the 0-based index into these tables -- e.g. _v(ADD)==1, matching v2[1].
 */
static const struct { const char *glyph, *name; } MONAD[] = {
    {":","Identity"},{"+","Flip"},{"-","Negate"},{"*","First"},{"%","Sqrt"},
    {"!","Til/Enumerate"},{"&","Where"},{"|","Reverse"},{"<","Ascend"},
    {">","Descend"},{"=","Group"},{"~","Not"},{",","Enlist"},{"^","Is-Null"},
    {"#","Count"},{"_","Floor"},{"$","String"},{"?","Distinct"},{"@","Type"},
    {".","Value/Eval"},{"0:","Unicode-0"},{"1:","Unicode-1"},{"2:","Unicode-2"},
    {"las","Last"},{"imin","Min-Index"},{"imax","Max-Index"},{"::","Identity/Out"},
};
static const struct { const char *glyph, *name; } DYAD[] = {
    {":","Right"},{"+","Add"},{"-","Subtract"},{"*","Multiply"},{"%","Divide"},
    {"!","Dict/Mod"},{"&","Min/And"},{"|","Max/Or"},{"<","Less Than"},
    {">","Greater Than"},{"=","Equal"},{"~","Match"},{",","Concat"},
    {"^","Fill"},{"#","Take/Reshape"},{"_","Drop/Cut"},{"$","Cast/Cond"},
    {"?","Find/Roll"},{"@","Apply"},{".","Apply/Eval"},
    {"0:","io0"},{"1:","io1"},{"2:","io2"},{"3:","io3"},{"4:","io4"},
    {"mkl","List"},{"gap","Block"},
};
static const struct { const char *glyph, *name; } ADVERB[] = {
    {"'","Each"},{"/","Over/Reduce"},{"\\","Scan"},
    {"':","Each-Prior"},{"/:","Each-Right"},{"\\:","Each-Left"},
};
#define NMONAD  ((int)(sizeof MONAD  / sizeof MONAD[0]))
#define NDYAD   ((int)(sizeof DYAD   / sizeof DYAD[0]))
#define NADVERB ((int)(sizeof ADVERB / sizeof ADVERB[0]))

/* Bounds-checked glyph/name lookup, shared by every place a verb or adverb
 * atom needs to become text -- whether it's the head of an application, a
 * bare value inside a hook/fork train, or the standalone argument to
 * `\ast +`. t0 must already be tu (monad), tv (dyad), or tw (adverb).
 * Always writes a safe value to *glyph and *name, even when out of range. */
static int verb_lookup(UC t0, I idx, const char **glyph, const char **name) {
    if (t0 == tu && idx >= 0 && idx < NMONAD)  { *glyph = MONAD[idx].glyph;  *name = MONAD[idx].name;  return 1; }
    if (t0 == tv && idx >= 0 && idx < NDYAD)   { *glyph = DYAD[idx].glyph;   *name = DYAD[idx].name;   return 1; }
    if (t0 == tw && idx >= 0 && idx < NADVERB) { *glyph = ADVERB[idx].glyph; *name = ADVERB[idx].name; return 1; }
    *glyph = "?"; *name = "Unknown";
    return 0;
}

/* Is `e` something that can stand in a tacit hook/fork train position, or
 * be the head of an application -- a verb, adverb, lambda, projection,
 * composition, or derived verb? Used to tell a genuine train `(f g)` apart
 * from an ordinary list literal `(1;2)`. */
static int is_applicable(A e) {
    UC t = _t(e);
    return t == tu || t == tv || t == tw || t == to || t == tp || t == tq || t == tr;
}

/* ---- node construction: arena-backed, not malloc/free --------------------
 * The whole ASTNode tree lives only for the duration of one \ast command
 * (ast_cmd() rewinds the arena when it's done printing), so it is bump-
 * allocated from Amber's real scratch region (arena_alloc(), src/arena.h)
 * exactly like csv.c's transient row/field grid -- no malloc, no free, no
 * per-node bookkeeping, and the whole tree is reclaimed in O(1). Amber has
 * no symbol literally named `g_scratch`; arena_alloc()/arena_reset() *is*
 * its scratch-arena API, and this module uses it directly. */

ASTNode *ast_new(ASTKind kind, const char *label, const char *annotation) {
    ASTNode *n = (ASTNode *)arena_alloc(sizeof *n);
    memset(n, 0, sizeof *n);
    n->kind = kind;
    if (label)      snprintf(n->label,      sizeof n->label,      "%s", label);
    if (annotation) snprintf(n->annotation, sizeof n->annotation, "%s", annotation);
    return n;
}

void ast_add_child(ASTNode *parent, ASTNode *child) {
    if (parent->nchildren >= parent->cap) {
        int newcap = parent->cap ? parent->cap * 2 : 4;
        ASTNode **grown = (ASTNode **)arena_alloc((size_t)newcap * sizeof *grown);
        if (parent->nchildren) memcpy(grown, parent->children, (size_t)parent->nchildren * sizeof *grown);
        parent->children = grown;
        parent->cap = newcap;
    }
    parent->children[parent->nchildren++] = child;
}

/* No-op -- see the memory note in ast.h. Kept so any existing "build, print,
 * free" call pattern keeps compiling without changes. */
void ast_free(ASTNode *node) { (void)node; }

/* ---- small bounded string-building helpers -------------------------------
 * All temporary label/annotation text is composed directly into the fixed
 * label[]/annotation[] buffers inside an arena-allocated ASTNode -- there
 * is no separate heap/stack scratch buffer to route through the arena
 * beyond the nodes themselves, which already are arena-backed above. These
 * helpers just keep that composition bounds-safe (never overrun, always
 * NUL-terminated) when building up a multi-element vector preview. */

static void bufcat(char *dst, size_t dstsz, size_t *off, const char *s) {
    if (*off >= dstsz - 1) return;
    size_t rem = dstsz - 1 - *off;
    size_t n = strlen(s);
    if (n > rem) n = rem;
    memcpy(dst + *off, s, n);
    *off += n;
    dst[*off] = 0;
}

/* Formats a double the way Amber's own float literals read (see the type
 * cheat-sheet in repl.k: "-0w -0.0 0.0 0w 1.2e308 0n"), so a Float64 leaf
 * never looks identical to an Int64 leaf (plain "%g" on 1.0 prints "1",
 * indistinguishable from the Int64 atom 1) and Amber's own null/infinity
 * tokens are used instead of C's "nan"/"inf". */
static void fmt_float(F f, char *buf, size_t n) {
    if (f != f) { snprintf(buf, n, "0n"); return; }           /* NaN -> Amber null */
    if (f > 0 && f * 2 == f && f != 0) { snprintf(buf, n, "0w"); return; }  /* +inf (avoids <math.h> isinf portability quirks) */
    if (f < 0 && f * 2 == f) { snprintf(buf, n, "-0w"); return; }          /* -inf */
    snprintf(buf, n, "%g", f);
    if (!strpbrk(buf, ".eEnN")) {
        size_t L = strlen(buf);
        if (L + 3 <= n) { buf[L] = '.'; buf[L + 1] = '0'; buf[L + 2] = 0; }
    }
}

static const char *vec_elem_typename(UC t) {
    switch (t) {
        case tB: return "Boolean";
        case tG: return "Byte";
        case tH: return "Short";
        case tI: return "Int";
        case tL: return "Long";
        case tF: return "Float";
        case tS: return "Symbol";
        default: return "Value";
    }
}

/* Vector literals are previewed (first few elements + a count), never
 * expanded node-by-node -- a 10,000,000-element float vector is not
 * something you want printed one leaf per element. `tC` (Char Vector) is
 * Amber's string type, so it gets a quoted-string preview instead of a
 * space-joined element list. */
#define AST_VEC_PREVIEW_MAX 8

static ASTNode *describe_vector(A v, UC t) {
    U n = _n(v);
    char lb[96], an[80];

    if (t == tC) {
        U cap = (U)sizeof lb - 6;
        U cn = n < cap ? n : cap;
        size_t off = 0;
        bufcat(lb, sizeof lb, &off, "\"");
        if (cn) { size_t take = cn; if (take > sizeof lb - 1 - off) take = sizeof lb - 1 - off;
                  memcpy(lb + off, _C(v), take); off += take; lb[off] = 0; }
        if (n > cap) bufcat(lb, sizeof lb, &off, "...");
        bufcat(lb, sizeof lb, &off, "\"");
        snprintf(an, sizeof an, "(String[%u])", (unsigned)n);
        return ast_new(AST_VECTOR, lb, an);
    }

    size_t off = 0;
    if (n == 0) {
        bufcat(lb, sizeof lb, &off, "()");
    } else {
        U shown = n < AST_VEC_PREVIEW_MAX ? n : AST_VEC_PREVIEW_MAX;
        for (U i = 0; i < shown; i++) {
            char one[40];
            switch (t) {
                case tB: snprintf(one, sizeof one, "%d",  (int)((const B *)_V(v))[i]); break;
                case tG: snprintf(one, sizeof one, "0x%02x", (unsigned)((const UC *)_V(v))[i]); break;
                case tH: snprintf(one, sizeof one, "%d",  (int)((const H *)_V(v))[i]); break;
                case tI: snprintf(one, sizeof one, "%d",  (int)((const I *)_V(v))[i]); break;
                case tL: snprintf(one, sizeof one, "%lld", (long long)((const L *)_V(v))[i]); break;
                case tF: fmt_float(((const F *)_V(v))[i], one, sizeof one); break;
                /* tS stores PACKED 32-bit symbol ids -- (const I*), NOT (const A*).
                 * See the file header note: reading this as A* was the old bug. */
                case tS: snprintf(one, sizeof one, "`%s", su((U)((const I *)_V(v))[i])); break;
                default: snprintf(one, sizeof one, "?"); break;
            }
            if (i) bufcat(lb, sizeof lb, &off, " ");
            bufcat(lb, sizeof lb, &off, one);
        }
        if (n > shown) bufcat(lb, sizeof lb, &off, " ...");
    }
    snprintf(an, sizeof an, "(%s Vector[%u])", vec_elem_typename(t), (unsigned)n);
    return ast_new(AST_VECTOR, lb, an);
}

/* The single dispatcher for every "leaf" value -- anything that isn't
 * itself a `[head;arg;...]` application shape being walked structurally.
 * Replaces the old fmt_scalar(), and (unlike it) has an explicit case for
 * every type tag a.h defines, so nothing falls through to a generic
 * "<X-atom>" placeholder. */
static ASTNode *describe_leaf(A v, UC t) {
    char lb[96], an[80];
    switch (t) {
        case ti: snprintf(lb, sizeof lb, "%d", (I)_v(v)); return ast_new(AST_SCALAR, lb, "(Int64)");
        case tl: snprintf(lb, sizeof lb, "%lld", (long long)*_L(v)); return ast_new(AST_SCALAR, lb, "(Int64)");
        case tf: fmt_float(*_F(v), lb, sizeof lb); return ast_new(AST_SCALAR, lb, "(Float64)");
        case tc: snprintf(lb, sizeof lb, "\"%c\"", (char)_v(v)); return ast_new(AST_SCALAR, lb, "(Char)");
        case ts: snprintf(lb, sizeof lb, "`%s", su((U)_v(v))); return ast_new(AST_SCALAR, lb, "(Symbol)");
        /* Date/Time/Timestamp atoms: shown as their raw underlying integer
         * (days/ms/ns since epoch) rather than a calendar-formatted string.
         * Amber's real date/time formatters (s.c: "dstr"/"stime"/"pstr") are
         * invoked reflectively through the K evaluator (K1(name,...)), and
         * \ast must never evaluate anything -- see the file header -- so
         * this stays a raw, honestly-labeled value instead of reaching for
         * that machinery. */
        case tdt: snprintf(lb, sizeof lb, "%lld", (long long)_v(v)); return ast_new(AST_SCALAR, lb, "(Date, raw days)");
        case ttm: snprintf(lb, sizeof lb, "%lld", (long long)_v(v)); return ast_new(AST_SCALAR, lb, "(Time, raw ms)");
        case tnp: snprintf(lb, sizeof lb, "%lld", (long long)*_L(v)); return ast_new(AST_SCALAR, lb, "(Timestamp, raw ns)");

        case tu: case tv: case tw: {
            const char *g, *nm;
            verb_lookup(t, _v(v), &g, &nm);
            snprintf(an, sizeof an, "(%s)", nm);
            return ast_new(t == tw ? AST_ADVERBATOM : AST_VERBATOM, g, an);
        }

        case to: { /* a lambda literal: pk() itself already compiled this (see
                    * ast.h's file header) -- show the ORIGINAL source text
                    * the compiler stashed at field [0], never the bytecode
                    * (that's \disasm's job, src/vm.c). */
            U nf = _n(v);
            A src = nf > 0 ? _A(v)[0] : 0;
            if (src && _t(src) == tC) {
                U sn = _n(src);
                U cap = (U)sizeof lb - 4;
                U cn = sn < cap ? sn : cap;
                memcpy(lb, _C(src), cn);
                lb[cn] = 0;
                if (sn > cap) strcat(lb, "...");
            } else {
                snprintf(lb, sizeof lb, "{...}");
            }
            return ast_new(AST_LAMBDA, lb, "(Lambda)");
        }
        /* tp/tq/tr are RUNTIME values (a bound projection, a composed train,
         * a derived verb like `+/` after it's been assigned to a variable)
         * -- pk() is parse-only and essentially never produces these
         * directly (see ast.h's file header), so this is defensive-only:
         * render something safe and clearly labeled rather than guessing at
         * undocumented internal layout (no verified struct definition for
         * these three beyond the header-byte adverb tag on tr, which this
         * module deliberately does not decode by hand). */
        case tp: return ast_new(AST_VERBATOM, "f[..]", "(Projection, runtime value)");
        case tq: return ast_new(AST_VERBATOM, "f|g",   "(Composition, runtime value)");
        case tr: return ast_new(AST_VERBATOM, "f/",    "(Derived Verb, runtime value)");

        case tB: case tG: case tH: case tI: case tL: case tF: case tC: case tS:
            return describe_vector(v, t);

        case tA:
            return ast_new(AST_VECTOR, "()", "(Empty List)");

        default:
            snprintf(lb, sizeof lb, "atom");
            snprintf(an, sizeof an, "(tag=%d)", (int)t);
            return ast_new(AST_SCALAR, lb, an);
    }
}

/* ---- the recursive parse-tree -> ASTNode conversion ----------------------
 * Mirrors cr()'s shape dispatch (b.c) closely enough to label the common
 * forms correctly; anything genuinely exotic (rare idioms, keyed-table
 * ctors, ...) safely falls through to a generic AST_VECTOR node instead of
 * misfiring, rather than trying to special-case every peephole cr() has. */

static ASTNode *ast_from_k_d(A v, int depth) {
    if (depth > AST_MAX_DEPTH)
        return ast_new(AST_SCALAR, "\xe2\x80\xa6", "(max depth exceeded, truncated)");

    if (v == GAP) /* a curried-away / omitted argument slot, e.g. the blank
                    * in `f[x;;z]` or the missing right operand of `1+` */
        return ast_new(AST_BLANK, "\xc2\xb7", "(curried / omitted argument)");

    UC t = _t(v);

    if (t == tS || t == ts) {
        /* A variable / (possibly namespaced) identifier reference: `a`,
         * `.ns.sub`. A LITERAL symbol or symbol-vector constant (`` `a ``,
         * `` `a`b`c ``) is disambiguated by the parser wrapping it in a
         * 1-element list -- see the n==1 branch below -- so an unwrapped
         * tS/ts here is always a reference, never data. Note tS stores
         * PACKED 32-BIT symbol ids (I), not boxed A values -- see the file
         * header's BUG E note; `(const I*)_V(v)`, not `_A(v)`. */
        char nb[96];
        if (t == ts) {
            snprintf(nb, sizeof nb, "%s", su((U)_v(v)));
        } else {
            U vn = _n(v);
            const I *ids = (const I *)_V(v);
            size_t off = 0;
            if (vn == 0) bufcat(nb, sizeof nb, &off, "`");
            for (U i = 0; i < vn; i++) {
                if (i) bufcat(nb, sizeof nb, &off, ".");
                bufcat(nb, sizeof nb, &off, su((U)ids[i]));
            }
        }
        return ast_new(AST_VAR, nb, "(Variable)");
    }

    if (t != tA || _n(v) == 0) /* not an application shape: a literal leaf */
        return describe_leaf(v, t);

    U n = _n(v);
    A head = _x(v);

    if (n == 1) /* `[y]`: a quoted/wrapped literal (symbol, symbol vector, ...) */
        return describe_leaf(head, _t(head));

    if (head == GAP) { /* `x;y;z` block: statements start at index 1 */
        ASTNode *b = ast_new(AST_BLOCK, ";", "(Sequence)");
        for (U i = 1; i < n; i++) ast_add_child(b, ast_from_k_d(_A(v)[i], depth + 1));
        return b;
    }

    if (head == MKL) {
        /* `(x;y;z)`: either a plain list literal, or -- if every element is
         * itself verb-like and there are exactly 2 or 3 of them -- a tacit
         * Hook `(f g)` or Fork `(f g h)` train. */
        U cnt = n - 1;
        if (cnt == 2 || cnt == 3) {
            int all_applicable = 1;
            for (U i = 1; i < n && all_applicable; i++) all_applicable = is_applicable(_A(v)[i]);
            if (all_applicable) {
                ASTNode *tt = ast_new(cnt == 2 ? AST_HOOK : AST_FORK,
                                       cnt == 2 ? "Hook" : "Fork",
                                       cnt == 2 ? "(f g) \xe2\x80\x94 tacit 2-train, unary"
                                                 : "(f g h) \xe2\x80\x94 tacit 3-train, binary");
                for (U i = 1; i < n; i++) ast_add_child(tt, ast_from_k_d(_A(v)[i], depth + 1));
                return tt;
            }
        }
        ASTNode *l = ast_new(AST_LIST, "(...)", "(List Literal)");
        for (U i = 1; i < n; i++) ast_add_child(l, ast_from_k_d(_A(v)[i], depth + 1));
        return l;
    }

    if (n == 2 && !_t0(head) && _t(head) == tA && _n(head) == 2) {
        /* `[[adverbAtom;verb];arg]`: an adverb-derived verb applied once,
         * e.g. `+/x` (sum reduce). See p.c's pT(): the adverb form is
         * parsed as a nested 2-element list *before* cf()/cr() ever run. */
        A adv = _x(head), base = _y(head);
        if (_t0(adv) == tw) {
            const char *ag, *an_;
            verb_lookup(tw, _v(adv), &ag, &an_);
            const char *bg = "?", *bn = "Verb";
            if (_t0(base) == tv) verb_lookup(tv, _v(base), &bg, &bn);
            else if (_t0(base) == tu) verb_lookup(tu, _v(base), &bg, &bn);
            char lb[32], an[80];
            size_t split = (size_t)snprintf(lb, sizeof lb, "%s", bg);
            snprintf(lb + split, sizeof lb - split, "%s", ag);
            snprintf(an, sizeof an, "(%s %s)", bn, an_);
            ASTNode *vb = ast_new(AST_VERB, lb, an);
            vb->split = (int)split;
            ast_add_child(vb, ast_from_k_d(_A(v)[1], depth + 1));
            return vb;
        }
    }

    if (n == 2 && _t0(head) == tu) { /* `+x`: monadic verb application */
        const char *g, *nm;
        verb_lookup(tu, _v(head), &g, &nm);
        char an[80]; snprintf(an, sizeof an, "(%s)", nm);
        ASTNode *vb = ast_new(AST_VERB, g, an);
        ast_add_child(vb, ast_from_k_d(_A(v)[1], depth + 1));
        return vb;
    }

    if (n == 3 && _t0(head) == tv) {
        /* `x+y`: dyadic application -- OR, if either operand slot is the
         * GAP sentinel, a curried projection (`1+`, `+1`... whichever side
         * the parser leaves blank) rather than a real binary op. */
        const char *g, *nm;
        verb_lookup(tv, _v(head), &g, &nm);
        A l = _A(v)[1], r = _A(v)[2];
        int curried = (l == GAP) || (r == GAP);
        char an[80];
        ASTNode *node;
        if (curried) {
            snprintf(an, sizeof an, "(%s \xc2\xb7 curried projection)", nm);
            node = ast_new(AST_PROJECTION, g, an);
        } else {
            snprintf(an, sizeof an, "(%s)", nm);
            node = ast_new(AST_BINOP, g, an);
        }
        ast_add_child(node, ast_from_k_d(l, depth + 1));
        ast_add_child(node, ast_from_k_d(r, depth + 1));
        return node;
    }

    if (TU(_t(head)) || !_t0(head)) {
        /* generic application f[x;y;...], or a computed head like
         * (f;g)[i][x]. If any argument slot is GAP, this is a partial
         * application / projection (`f[x;;z]`) rather than a full call. */
        int any_blank = 0;
        for (U i = 1; i < n; i++) if (_A(v)[i] == GAP) { any_blank = 1; break; }
        ASTNode *ap = ast_new(any_blank ? AST_PROJECTION : AST_APPLY,
                               any_blank ? "Projection" : "Apply",
                               any_blank ? "(partial application \xe2\x80\x94 some arguments curried away)" : NULL);
        ast_add_child(ap, ast_from_k_d(head, depth + 1));
        for (U i = 1; i < n; i++) ast_add_child(ap, ast_from_k_d(_A(v)[i], depth + 1));
        return ap;
    }

    /* Fallback: an unrecognized shape (rare idioms, keyed-table ctors, ...).
     * Show it as a generic list of its own elements rather than guessing. */
    ASTNode *g = ast_new(AST_VECTOR, vec_elem_typename(t), "(Node)");
    for (U i = 0; i < n; i++) ast_add_child(g, ast_from_k_d(_A(v)[i], depth + 1));
    return g;
}

ASTNode *ast_from_k(A v) { return ast_from_k_d(v, 0); }

/* ---- printing -------------------------------------------------------------
 * Palette (per spec): bold cyan for verbs (monadic or dyadic, bare or
 * applied, including curried projections and function application), bold
 * magenta for adverbs, bright green for numeric/literal scalars, yellow for
 * symbols/variables, dim gray for the tree connectors themselves. A few
 * purely structural wrapper kinds (list literals, statement blocks, tacit
 * hook/fork train labels) get their own restrained accent so the tree
 * still reads at a glance without overloading the five required colors. */

#define C_VERB  "\x1b[1;36m" /* bold cyan    : verbs (bare, applied, curried, calls) */
#define C_ADV   "\x1b[1;35m" /* bold magenta : adverbs                               */
#define C_VAR   "\x1b[33m"   /* yellow       : variables / symbols                   */
#define C_LIT   "\x1b[92m"   /* bright green : numeric / literal scalars             */
#define C_VEC   "\x1b[36m"   /* cyan         : vector / generic payloads             */
#define C_LAM   "\x1b[96m"   /* bright cyan  : lambda literals (callable, distinct)  */
#define C_LIST  "\x1b[1;32m" /* bold green   : list literals                         */
#define C_BLOCK "\x1b[90m"   /* bright black : statement-sequence separators         */
#define C_TRAIN "\x1b[1;34m" /* bold blue    : tacit hook/fork train labels          */
#define C_BLANK "\x1b[2;33m" /* dim yellow   : curried/omitted argument placeholder  */
#define C_DIM   "\x1b[2m"    /* dim          : connectors + annotations              */
#define C_RST   ANSI_RST

static const char *node_color(ASTKind k) {
    switch (k) {
        case AST_VERB:       return C_VERB;
        case AST_VERBATOM:   return C_VERB;
        case AST_ADVERBATOM: return C_ADV;
        case AST_BINOP:      return C_VERB;
        case AST_PROJECTION: return C_VERB;
        case AST_APPLY:      return C_VERB;
        case AST_VAR:        return C_VAR;
        case AST_SCALAR:     return C_LIT;
        case AST_VECTOR:     return C_VEC;
        case AST_LAMBDA:     return C_LAM;
        case AST_LIST:       return C_LIST;
        case AST_BLOCK:      return C_BLOCK;
        case AST_HOOK:       return C_TRAIN;
        case AST_FORK:       return C_TRAIN;
        case AST_BLANK:      return C_BLANK;
        default:              return "";
    }
}

static const char *kind_prefix(ASTKind k) {
    switch (k) {
        case AST_VERB:       return "Verb";
        case AST_VERBATOM:   return "Verb";
        case AST_ADVERBATOM: return "Adverb";
        case AST_BINOP:      return "Binary Op";
        case AST_PROJECTION: return "Projection";
        case AST_VAR:        return "Var";
        case AST_SCALAR:     return "Scalar";
        case AST_VECTOR:     return "Vector";
        case AST_LAMBDA:     return "Lambda";
        case AST_APPLY:      return "Apply";
        case AST_BLOCK:      return "Block";
        case AST_LIST:       return "List";
        case AST_HOOK:       return "Hook";
        case AST_FORK:       return "Fork";
        case AST_BLANK:      return "Blank";
        default:               return "Node";
    }
}

/* Prints `label`, split at byte offset `split` into a bold-cyan verb part
 * and a bold-magenta adverb part (used for combined glyphs like "+/"). If
 * split==0, the whole label is printed in `color` (the node's own color). */
static void print_label(const char *label, const char *color, int split) {
    if (split > 0 && split < (int)strlen(label)) {
        printf("%s%.*s%s%s%s%s", C_VERB, split, label, C_RST, C_ADV, label + split, C_RST);
    } else if (*color) {
        printf("%s%s%s", color, label, C_RST);
    } else {
        printf("%s", label);
    }
}

static void print_node(const ASTNode *n, char *prefix, int is_last, int depth) {
    if (depth > AST_MAX_DEPTH) { printf("%s%s... (truncated: too deep)%s\n", prefix, C_DIM, C_RST); return; }

    printf("%s%s%s%s", prefix, C_DIM,
           is_last ? "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 " /* "└── " */
                   : "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 " /* "├── " */,
           C_RST);
    const char *color = node_color(n->kind);
    printf("%s : ", kind_prefix(n->kind));
    print_label(n->label, color, n->split);
    if (n->annotation[0]) printf(" %s%s%s", C_DIM, n->annotation, C_RST);
    printf("\n");

    char child_prefix[256];
    snprintf(child_prefix, sizeof child_prefix, "%s%s", prefix,
             is_last ? "    " : "\xe2\x94\x82   " /* "│   " */);
    for (int i = 0; i < n->nchildren; i++)
        print_node(n->children[i], child_prefix, i == n->nchildren - 1, depth + 1);
}

/* `\ast` always shows a synthetic "Root" header, per the target layout. If
 * the parsed input was a `;`-separated block of statements, each statement
 * becomes a direct child of Root; a single expression becomes Root's one
 * child. Either way nothing is drawn as a redundant "Block : ;" node. */
void print_ast(const ASTNode *root) {
    if (!root) { printf("(empty)\n"); return; }
    printf("Root\n");
    char prefix[256] = "";
    if (root->kind == AST_BLOCK) {
        for (int i = 0; i < root->nchildren; i++)
            print_node(root->children[i], prefix, i == root->nchildren - 1, 1);
    } else {
        print_node(root, prefix, 1, 1);
    }
}

/* ---- REPL command handler ------------------------------------------------- */

A ast_cmd(S s) {
    /* Rewind to a clean scratch region before building the tree (mirrors
     * src/trace.c's \trace handler), then rewind again after printing so
     * \ast leaves no arena footprint behind for the next command -- see
     * the memory note in ast.h: every ASTNode/children-array in this tree
     * is arena-backed, not malloc'd. */
    arena_reset();

    S p = s;
    A tree = pk(&p, 10); /* parse only -- never cpl()/run() this (except the
                           * one unavoidable exception documented in ast.h:
                           * pk() itself compiles embedded lambda literals) */
    if (!tree) { printf("\\ast: parse error\n"); arena_reset(); return au; }

    ASTNode *root = ast_from_k(tree);
    print_ast(root);
    ast_free(root); /* no-op, see ast.h -- arena_reset() below does the real work */
    mr(tree);        /* release the parse tree pk() handed us */

    arena_reset();
    return au;
}

/* ---- self-test ------------------------------------------------------------
 * Captures ast_cmd()'s stdout (it prints directly, like \disasm/\hl) into a
 * real file rather than /dev/null (contrast vm_selftest(), which only needs
 * silence) so each case's output can be checked for the labels that used to
 * come out as "<v-atom>"/"<w-atom>"/"<o-atom>"/"<I-atom>"/"<S-atom>" before
 * this module's rewrite -- see ast.c's file header for the full bug list. */
I ast_selftest(void) {
    static const struct { const char *src, *need1, *need2; } CASES[] = {
        {"1+2",               "Binary Op",     "Add"},
        {"1.5",                "Float64",       "1.5"},
        {"{x+1}",              "Lambda",        "{x+1}"},
        {"(+;-)",              "Hook",          "tacit 2-train"},
        {"(*;+;%)",            "Fork",          "tacit 3-train"},
        {"1+",                 "Projection",    "curried"},
        {"`a`b`c",             "Symbol Vector[3]", "`a"},
        {".ns.sub",            "ns.sub",        "Value/Eval"},
        {"f:{x+y+z};f[1;;3]",  "Blank",         "curried"},
    };
    CO C *P_ = "/tmp/.amber_ast_selftest.out";
    B ok = 1;
    fflush(stdout);
    I saved = dup(1);
    I fd = open(P_, O_RDWR | O_CREAT | O_TRUNC, 0600); /* O_RDWR, not O_WRONLY -- this fd is read back below */
    if (saved < 0 || fd < 0) { if (saved >= 0) close(saved); if (fd >= 0) close(fd); return 0; }

    for (U c = 0; c < sizeof(CASES) / sizeof(*CASES) && ok; c++) {
        lseek(fd, 0, SEEK_SET);
        if (ftruncate(fd, 0)) {}
        fflush(stdout);
        dup2(fd, 1);
        ast_cmd((S)CASES[c].src);
        fflush(stdout);
        dup2(saved, 1);

        lseek(fd, 0, SEEK_SET);
        C buf[4096];
        L n = read(fd, buf, sizeof buf - 1);
        if (n < 0) n = 0;
        buf[n] = 0;
        ok = ok && strstr(buf, CASES[c].need1) && strstr(buf, CASES[c].need2);
    }

    close(fd);
    close(saved);
    remove(P_);
    return ok;
}
