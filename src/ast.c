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
 */
#include "a.h"
#include "ast.h"
#include "ansi.h"
#include <stdio.h>
#include <stdlib.h>

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

/* ---- node construction --------------------------------------------------- */

ASTNode *ast_new(ASTKind kind, const char *label, const char *annotation) {
    ASTNode *n = (ASTNode *)calloc(1, sizeof *n);
    n->kind = kind;
    if (label)      snprintf(n->label,      sizeof n->label,      "%s", label);
    if (annotation) snprintf(n->annotation, sizeof n->annotation, "%s", annotation);
    return n;
}

void ast_add_child(ASTNode *parent, ASTNode *child) {
    if (parent->nchildren >= parent->cap) {
        parent->cap = parent->cap ? parent->cap * 2 : 4;
        parent->children = (ASTNode **)realloc(parent->children, parent->cap * sizeof *parent->children);
    }
    parent->children[parent->nchildren++] = child;
}

void ast_free(ASTNode *node) {
    if (!node) return;
    for (int i = 0; i < node->nchildren; i++) ast_free(node->children[i]);
    free(node->children);
    free(node);
}

/* ---- scalar formatting ---------------------------------------------------
 * Non-consuming: reads bytes directly instead of calling gl()/gf() (which
 * unref their argument -- not appropriate here, we don't own the tree). */

static void fmt_scalar(A v, char *buf, size_t n) {
    UC t = _t(v);
    switch (t) {
        case ts: snprintf(buf, n, "`%s", su((U)_v(v))); return;
        case ti: snprintf(buf, n, "%d", (I)_v(v)); return;
        case tl: snprintf(buf, n, "%lld", (long long)*_L(v)); return;
        case tf: snprintf(buf, n, "%g", *_F(v)); return;
        case tc: snprintf(buf, n, "\"%c\"", (char)_v(v)); return;
        default: snprintf(buf, n, "<%c-atom>", (t < tn ? TS[t] : '?')); return;
    }
}

/* Vector literals are shown as "<N-elem TypeName>" rather than expanded --
 * a 10,000,000-element float vector is not something you want printed node
 * by node. */
static const char *vector_typename(UC t) {
    switch (t) {
        case tA: return "List";
        case tB: return "Boolean Vector";
        case tG: return "Byte Vector";
        case tH: return "Short Vector";
        case tI: return "Int Vector";
        case tL: return "Long Vector";
        case tF: return "Float Vector";
        case tC: return "String";
        case tS: return "Symbol Vector";
        default: return "Vector";
    }
}

/* ---- the recursive parse-tree -> ASTNode conversion ----------------------
 * Mirrors cr()'s shape dispatch (b.c) closely enough to label the common
 * forms correctly; anything genuinely exotic (rare idioms, keyed-table
 * ctors, ...) safely falls through to a generic AST_APPLY node instead of
 * misfiring, rather than trying to special-case every peephole cr() has. */

ASTNode *ast_from_k(A v) {
    UC t = _t(v);

    if (t == tS || t == ts) { /* variable / (possibly qualified) symbol ref */
        char nb[48];
        if (t == ts) snprintf(nb, sizeof nb, "%s", su((U)_v(v)));
        else if (_n(v) == 0) snprintf(nb, sizeof nb, "`");
        else if (_n(v) == 1) snprintf(nb, sizeof nb, "%s", su((U)_v(_A(v)[0])));
        else snprintf(nb, sizeof nb, "%s.%s", su((U)_v(_A(v)[0])), su((U)_v(_A(v)[_n(v)-1])));
        return ast_new(AST_VAR, nb, "Variable");
    }

    if (t != tA || _n(v) == 0) { /* bare atom, or empty list: a literal */
        char nb[48];
        fmt_scalar(v, nb, sizeof nb);
        return ast_new(AST_SCALAR, nb, "Scalar");
    }

    U n = _n(v);
    A head = _x(v);

    if (n == 1) { /* `[y]`: a quoted literal (e.g. a quoted symbol, not a var ref) */
        char nb[48];
        fmt_scalar(head, nb, sizeof nb);
        return ast_new(AST_SCALAR, nb, "Literal");
    }

    if (head == GAP) { /* `x;y;z` block: statements start at index 1 */
        ASTNode *b = ast_new(AST_BLOCK, ";", "Sequence");
        for (U i = 1; i < n; i++) ast_add_child(b, ast_from_k(_A(v)[i]));
        return b;
    }

    if (head == MKL) { /* `(x;y;z)` list literal: elements start at index 1 */
        ASTNode *l = ast_new(AST_LIST, "(...)", "List Literal");
        for (U i = 1; i < n; i++) ast_add_child(l, ast_from_k(_A(v)[i]));
        return l;
    }

    if (n == 2 && !_t0(head) && _t(head) == tA && _n(head) == 2) {
        /* `[[adverbAtom;verb];arg]`: an adverb-derived verb applied once,
         * e.g. `+/x` (sum reduce). See p.c's pT(): the adverb form is
         * parsed as a nested 2-element list *before* cf()/cr() ever run. */
        A adv = _x(head), base = _y(head);
        if (_t0(adv) == tw) {
            I ai = _v(adv);
            char lb[48], an[64];
            const char *ag = (ai >= 0 && ai < NADVERB) ? ADVERB[ai].glyph : "?";
            const char *and_ = (ai >= 0 && ai < NADVERB) ? ADVERB[ai].name : "adverb";
            if (_t0(base) == tv) {
                I bi = _v(base);
                const char *bg = (bi >= 0 && bi < NDYAD) ? DYAD[bi].glyph : "?";
                snprintf(lb, sizeof lb, "%s%s", bg, ag);
                snprintf(an, sizeof an, "(%s %s)",
                    (bi >= 0 && bi < NDYAD) ? DYAD[bi].name : "Verb", and_);
            } else if (_t0(base) == tu) {
                I bi = _v(base);
                const char *bg = (bi >= 0 && bi < NMONAD) ? MONAD[bi].glyph : "?";
                snprintf(lb, sizeof lb, "%s%s", bg, ag);
                snprintf(an, sizeof an, "(%s %s)",
                    (bi >= 0 && bi < NMONAD) ? MONAD[bi].name : "Verb", and_);
            } else {
                snprintf(lb, sizeof lb, "?%s", ag);
                snprintf(an, sizeof an, "(%s)", and_);
            }
            ASTNode *vb = ast_new(AST_VERB, lb, an);
            ast_add_child(vb, ast_from_k(_A(v)[1]));
            return vb;
        }
    }

    if (n == 2 && _t0(head) == tu) { /* `+x`: monadic verb application */
        I i = _v(head);
        const char *g = (i >= 0 && i < NMONAD) ? MONAD[i].glyph : "?";
        const char *nm = (i >= 0 && i < NMONAD) ? MONAD[i].name : "Verb";
        char an[64]; snprintf(an, sizeof an, "(%s)", nm);
        ASTNode *vb = ast_new(AST_VERB, g, an);
        ast_add_child(vb, ast_from_k(_A(v)[1]));
        return vb;
    }

    if (n == 3 && _t0(head) == tv) { /* `x+y`: dyadic (binary) verb application */
        I i = _v(head);
        const char *g = (i >= 0 && i < NDYAD) ? DYAD[i].glyph : "?";
        const char *nm = (i >= 0 && i < NDYAD) ? DYAD[i].name : "Binary Op";
        char lb[48], an[64];
        snprintf(lb, sizeof lb, "%s", g);
        snprintf(an, sizeof an, "(%s)", nm);
        ASTNode *bo = ast_new(AST_BINOP, lb, an);
        ast_add_child(bo, ast_from_k(_A(v)[1]));
        ast_add_child(bo, ast_from_k(_A(v)[2]));
        return bo;
    }

    if (TU(_t(head)) || !_t0(head)) { /* generic application f[x;y;...], or a
                                        * computed head like (f;g)[i][x]     */
        ASTNode *ap = ast_new(AST_APPLY, "Apply", NULL);
        ast_add_child(ap, ast_from_k(head));
        for (U i = 1; i < n; i++) ast_add_child(ap, ast_from_k(_A(v)[i]));
        return ap;
    }

    /* Fallback: an unrecognized shape (rare idioms, keyed-table ctors, ...).
     * Show it as a generic list of its own elements rather than guessing. */
    ASTNode *g = ast_new(AST_VECTOR, vector_typename(t), "Node");
    for (U i = 0; i < n; i++) ast_add_child(g, ast_from_k(_A(v)[i]));
    return g;
}

/* ---- printing -------------------------------------------------------------
 * Every node kind gets its own color so the tree reads at a glance:
 * monadic/adverb-derived verbs = bold cyan, binary operators = bold
 * magenta (a related but distinguishable hue), variables = yellow, scalar
 * literals = green, vector/generic payloads = plain cyan, function
 * application = bold blue, list literals = bold green, statement blocks =
 * dim gray. Tree connectors and annotations are dimmed so the colored
 * labels pop without the whole tree turning into a wall of color. */

#define C_VERB  "\x1b[1;36m" /* bold cyan    : verbs / adverb-derived verbs */
#define C_BINOP "\x1b[1;35m" /* bold magenta : binary operators             */
#define C_VAR   "\x1b[33m"   /* yellow       : variables / symbols          */
#define C_LIT   "\x1b[32m"   /* green        : scalar literals              */
#define C_VEC   "\x1b[36m"   /* cyan         : vector / generic payloads    */
#define C_APPLY "\x1b[1;34m" /* bold blue    : function application         */
#define C_LIST  "\x1b[1;32m" /* bold green   : list literals                */
#define C_BLOCK "\x1b[90m"   /* bright black : statement-sequence separators*/
#define C_DIM   "\x1b[2m"    /* dim          : connectors + annotations     */
#define C_RST   ANSI_RST

static const char *node_color(ASTKind k) {
    switch (k) {
        case AST_VERB:     return C_VERB;
        case AST_BINOP:    return C_BINOP;
        case AST_VAR:      return C_VAR;
        case AST_SCALAR:   return C_LIT;
        case AST_VECTOR:   return C_VEC;
        case AST_APPLY:    return C_APPLY;
        case AST_LIST:     return C_LIST;
        case AST_BLOCK:    return C_BLOCK;
        default:           return "";
    }
}

static const char *kind_prefix(ASTKind k) {
    switch (k) {
        case AST_VERB:   return "Verb";
        case AST_BINOP:  return "Binary Op";
        case AST_VAR:    return "Var";
        case AST_SCALAR: return "Scalar";
        case AST_VECTOR: return "Vector";
        case AST_APPLY:  return "Apply";
        case AST_BLOCK:  return "Block";
        case AST_LIST:   return "List";
        default:         return "Node";
    }
}

static void print_node(const ASTNode *n, char *prefix, int is_last, int depth) {
    if (depth > 200) { printf("%s... (truncated: too deep)\n", prefix); return; }

    printf("%s%s", prefix, is_last ? "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 " /* "└── " */
                                   : "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 " /* "├── " */);
    const char *color = node_color(n->kind);
    printf("%s : %s%s%s", kind_prefix(n->kind), color, n->label, *color ? C_RST : "");
    if (n->annotation[0]) printf(" %s", n->annotation);
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
    S p = s;
    A tree = pk(&p, 10); /* parse only -- never cpl()/run() this */
    if (!tree) { printf("\\ast: parse error\n"); return au; }

    ASTNode *root = ast_from_k(tree);
    print_ast(root);
    ast_free(root);
    mr(tree); /* release the parse tree pk() handed us */
    return au;
}
