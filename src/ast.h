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
 * or evaluating anything -- \ast never touches cpl()/run().
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

typedef enum {
    AST_ROOT,     /* synthetic wrapper for the whole printed tree           */
    AST_BLOCK,    /* `;`-separated sequence of top-level statements         */
    AST_LIST,     /* `(x;y;z)` list literal                                 */
    AST_VERB,     /* a verb used as a value: monadic, or adverb-derived     */
    AST_BINOP,    /* dyadic (binary) verb application: `x + y`              */
    AST_APPLY,    /* general application `f[x;y]` / projection              */
    AST_VAR,      /* a variable or (possibly namespaced) symbol reference   */
    AST_SCALAR,   /* an atomic literal: number, char, symbol, boolean, ...  */
    AST_VECTOR    /* a literal vector/list payload (shown, not expanded)    */
} ASTKind;

typedef struct ASTNode {
    ASTKind kind;
    char label[48];       /* e.g. "+/", "+", "a", "2.5"                    */
    char annotation[64];  /* e.g. "(Sum Reduce)", "(Float Vector)"         */
    struct ASTNode **children;
    int nchildren;
    int cap;              /* children array capacity (internal bookkeeping) */
} ASTNode;

/* Build one node (no children yet); label/annotation may be NULL for "none". */
ASTNode *ast_new(ASTKind kind, const char *label, const char *annotation);

/* Append `child` to `parent`'s children (grows the array as needed). */
void ast_add_child(ASTNode *parent, ASTNode *child);

/* Recursively free a node and all of its children. Safe on NULL. */
void ast_free(ASTNode *node);

/* Convert pk()'s raw parse-tree value `x` into an ASTNode tree. Does not
 * consume/free/unref `x` (the caller -- ast_cmd() below -- owns that). */
ASTNode *ast_from_k(A x);

/* Pretty-print an ASTNode tree with Unicode box-drawing connectors and
 * ANSI colors (verbs = bold cyan, vars = yellow, scalars = green). */
void print_ast(const ASTNode *root);

/* The `\ast <expression>` REPL command handler: parses `s` (WITHOUT
 * compiling or running it), converts the result to an ASTNode tree, prints
 * it, and cleans up after itself. Always returns the K "unit" value `au`
 * (see a.h) since \ast produces no result value of its own. */
A ast_cmd(S s);

#ifdef __cplusplus
}
#endif

#endif /* AMBER_AST_H */
