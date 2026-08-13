/* ast.h  -  Amber AST visualizer ("\ast <expression>").
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * Amber is homoiconic: pk() (the parser, see p.c) does not build a separate
 * tagged-node datatype -- it returns the SAME `A` K-value type used
 * everywhere at runtime. An expression like `a+2.5` parses to a plain
 * 3-element list `[dyadAtom(+); a; 2.5]`; `+/a` parses to
 * `[[adverbAtom(/); dyadAtom(+)]; a]` (an adverb-decorated verb applied to
 * one argument), and so on (see cr() in b.c, the real compiler, for the
 * authoritative shape dispatch this module mirrors).
 *
 * This module converts that raw parse tree into an explicit, easy-to-print
 * ASTNode tree (so callers get the conventional struct/print_ast() API even
 * though Amber itself has no such struct internally), *without* compiling
 * or evaluating anything -- \ast never touches cpl()/run() -- with ONE
 * unavoidable exception: pk() itself eagerly compiles lambda LITERALS
 * (`{...}`) at parse time (see p.c's '{' handling), so a `to`-tagged
 * compiled closure can legitimately appear as a leaf inside an otherwise
 * uncompiled tree. This module never disassembles that closure's bytecode
 * (that's `\disasm`'s job, src/vm.{h,c}) -- it shows the lambda's original
 * source text instead (the closure's field [0], a Char vector the compiler
 * itself stashes for exactly this kind of annotation).
 *
 * Memory: node/child-array storage is bump-allocated from Amber's real
 * scratch region (arena_alloc()/arena_reset(), see src/arena.h) instead of
 * malloc/calloc/free -- the whole tree is a strictly single-command scratch
 * structure with no cross-call lifetime, a perfect fit for the arena, and
 * it means \ast produces zero net heap churn per invocation (ast_cmd()
 * arena_reset()s at the end). ast_free() is kept only so any existing
 * caller pattern of "build then free" keeps compiling; it is a documented
 * no-op -- do not rely on it for cleanup, arena_reset() is what reclaims
 * the memory.
 */
#ifndef AMBER_AST_H
#define AMBER_AST_H

/* Requires a.h to already be included by the translation unit (for the `A`
 * K-value type and pk()/su() etc). Not included here directly because
 * a.h has no #include guard and is meant to be pulled in exactly once
 * per .c file. */

#ifdef __cplusplus
extern "C" {
#endif

/* Recursion depth guard: ast_from_k() refuses to recurse past this many
 * nested levels and instead returns a synthetic "(max depth exceeded)"
 * leaf, so a pathological or accidentally-cyclic-looking input can't blow
 * the C stack. 64 comfortably covers any hand-written expression while
 * staying well inside a default 8 MB thread stack even with a generous
 * per-frame budget. */
#define AST_MAX_DEPTH 64

typedef enum {
    AST_ROOT,       /* synthetic wrapper for the whole printed tree           */
    AST_BLOCK,      /* `;`-separated sequence of top-level statements         */
    AST_LIST,       /* `(x;y;z)` list literal                                 */
    AST_HOOK,       /* `(f g)` tacit hook -- a 2-verb train                   */
    AST_FORK,       /* `(f g h)` tacit fork -- a 3-verb train                 */
    AST_VERB,       /* a monadic verb, or adverb-derived verb, WITH an operand*/
    AST_VERBATOM,   /* a bare verb value with no operand (yet)                */
    AST_ADVERBATOM, /* a bare adverb value on its own (rare, but well-formed) */
    AST_BINOP,      /* dyadic (binary) verb application: `x + y`              */
    AST_PROJECTION, /* a curried/partial application: `1+`, `f[x;;z]`         */
    AST_APPLY,      /* general application `f[x;y]`                          */
    /* ---- qSQL clause specialization ------------------------------------
     * Amber's SQL-ish surface syntax is not a separate grammar: qsql.k's
     * `qrw` rewrites `select px by sym from t where px>10` into an ordinary
     * application of a K function to a string -- `sel"px by sym from t
     * where px>10"` -- and THAT is what pk() hands this module (see the
     * probe in ast_qsql_kind() below). Rendering it as a bare Apply with an
     * opaque 25-character string leaf tells the reader nothing about the
     * query, so \ast recognises the four query verbs (sel/exq/upd/del, and
     * their functional-form counterparts qselect/qexec/qby/qwhere) and
     * explodes the string back into the clause structure it encodes.
     *
     * The four *_SELECT/_EXEC/_UPDATE/_DELETE kinds are query BLOCK heads,
     * one per query; _BY and _WHERE are the optional clause children that
     * hang off a block. */
    AST_QSQL_SELECT,/* `select ... from ...`  query block head               */
    AST_QSQL_EXEC,  /* `exec ... from ...`    query block head               */
    AST_QSQL_UPDATE,/* `update ... from ...`  query block head               */
    AST_QSQL_DELETE,/* `delete ... from ...`  query block head               */
    AST_QSQL_BY,    /* `by <cols>` grouping clause                           */
    AST_QSQL_WHERE, /* `where <pred>` row-filter clause                      */
    AST_VAR,        /* a variable or (possibly namespaced) symbol reference   */
    AST_SCALAR,     /* an atomic literal: number, char, symbol, boolean, ...  */
    AST_VECTOR,     /* a literal vector/list payload (previewed, not expanded)*/
    AST_LAMBDA,     /* a `{...}` lambda literal (shown as its source text)    */
    AST_BLANK       /* a curried-away argument slot: `f[x;;z]`'s middle `;;`  */
} ASTKind;

typedef struct ASTNode {
    ASTKind kind;
    char label[96];        /* e.g. "+/", "+", "a", "2.5", "1 2 3", "{x+1}"   */
    char annotation[80];   /* e.g. "(Sum Reduce)", "(Float64)", "(Int Vector[3])" */
    int split;              /* for AST_VERB combined "<verb><adverb>" labels
                              * (e.g. "+/"): byte offset in `label` where the
                              * adverb glyph begins, so print_node() can color
                              * the verb part bold cyan and the adverb part
                              * bold magenta within one node. 0 = no split. */
    int badge;              /* 1 = render `annotation` as a highlighted badge
                              * (bold bright yellow) instead of the usual dim
                              * grey. Used for the time-series join callouts
                              * (`aj` -> As-Of Time-Series Join, `wj` -> Window
                              * Join), which are the one annotation a reader is
                              * actually scanning the tree FOR and so should not
                              * be de-emphasised like a type tag. */
    struct ASTNode **children;
    int nchildren;
    int cap;                /* children array capacity (internal bookkeeping) */
} ASTNode;

/* Build one node (no children yet); label/annotation may be NULL for "none".
 * Allocated from Amber's scratch arena -- see the memory note above. */
ASTNode *ast_new(ASTKind kind, const char *label, const char *annotation);

/* Append `child` to `parent`'s children (grows the array as needed, via the
 * scratch arena -- never realloc/free). */
void ast_add_child(ASTNode *parent, ASTNode *child);

/* No-op: the tree is scratch-arena-backed and reclaimed in bulk by
 * ast_cmd()'s arena_reset(). Kept for API compatibility; safe on NULL. */
void ast_free(ASTNode *node);

/* Convert pk()'s raw parse-tree value `x` into an ASTNode tree. Does not
 * consume/free/unref `x` (the caller -- ast_cmd() below -- owns that). */
ASTNode *ast_from_k(A x);

/* Pretty-print an ASTNode tree with Unicode box-drawing connectors and
 * ANSI colors: bold cyan for verbs (monadic or dyadic, applied or bare),
 * bold magenta for adverbs, bright green for numeric/literal scalars,
 * yellow for symbols/variables, dim gray for the tree connectors
 * themselves and for annotations. */
void print_ast(const ASTNode *root);

/* The `\ast <expression>` REPL command handler: parses `s` (WITHOUT
 * compiling or running it -- except for embedded lambda literals, which
 * pk() itself always compiles, see the file header above), converts the
 * result to an ASTNode tree, prints it, and rewinds the scratch arena.
 * Always returns the K "unit" value `au` (see a.h) since \ast produces no
 * result value of its own. */
A ast_cmd(S s);

/* Self-test: runs \ast against a set of representative expressions
 * (arithmetic, a lambda literal, a tacit hook, a tacit fork, a curried
 * projection, a literal symbol vector, a namespaced identifier, and a
 * partial application with a blank argument slot) with stdout captured,
 * and checks the printed tree contains the expected labels for each --
 * i.e. that none of the historical "<X-atom>" bugs have regressed. Returns
 * 1 iff every case matches, 0 otherwise. See also tests/test_ast.c for a
 * standalone (non-self-test-builtin) test harness covering the same
 * ground plus a few extra structural checks. */
I ast_selftest(void);

#ifdef __cplusplus
}
#endif

#endif /* AMBER_AST_H */
