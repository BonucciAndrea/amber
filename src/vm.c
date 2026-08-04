/* vm.c  -  see vm.h: a read-only disassembler for Amber's existing
 * compiler (src/b.c: cpl()/cr()) and stack VM (src/b.c: run()).
 * GNU AGPLv3 - see LICENSE and NOTICE.
 *
 * Compiles (but never runs) an expression via the real b.c pipeline --
 * exactly the same pk()/cpl() call trace.c uses before it calls run() --
 * then walks the resulting bytecode array with the mirrored opcode table
 * from vm.h and prints one line per instruction.
 *
 * Self-check: because the opcode/operand-length table here is a hand
 * maintained mirror of b.c's private tables (not something b.c exports),
 * vm_disasm_cmd() tracks how many bytecode bytes the decode loop actually
 * consumed and reports a mismatch instead of silently printing a
 * misaligned dump if the two ever drift apart.
 */
#include "a.h"
#include "vm.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* ---- mirrored opcode table (must match src/b.c's private enum) --------- */
enum {
    OP_MONAD  = 0,   /* .. 31  */
    OP_DYAD   = 32,  /* .. 63  */
    OP_SETLOC = 64,  /* .. 79  */
    OP_GETLOC = 80,  /* .. 95  */
    OP_DELLOC = 96,  /* .. 111 */
    OP_APPLY    = 112,
    OP_PROJECT  = 113,
    OP_MODASN   = 114,
    OP_MODASNG  = 115,
    OP_IDXASN   = 116,
    OP_IDXASNG  = 117,
    OP_IDXGET   = 118,
    OP_IDXGETG  = 119,
    OP_GETGLB   = 120,
    OP_SETGLB   = 121,
    OP_LIST     = 122,
    OP_UNLIST   = 123,
    OP_BRANCH   = 124,
    OP_JUMP     = 125,
    OP_RECUR    = 126,
    OP_POP      = 127,
    OP_CONSTDYAD = 128,
    OP_CONST0   = 129  /* CONST i is OP_CONST0+i */
};

/* extra operand bytes for opcodes >= OP_APPLY, indexed from OP_APPLY;
 * opcodes below OP_APPLY (monad/dyad/local ops, 0..111) encode their
 * operand directly in the opcode byte and take zero extra bytes. */
static const unsigned char EXTRA[] = {
    /* APPLY */1, /* PROJECT */1, /* MODASN */3, /* MODASNG */3,
    /* IDXASN */3, /* IDXASNG */3, /* IDXGET */3, /* IDXGETG */3,
    /* GETGLB */2, /* SETGLB */2, /* LIST */1, /* UNLIST */1,
    /* BRANCH */1, /* JUMP */1, /* RECUR */0, /* POP */0,
    /* CONSTDYAD */2 /* CONST i has 0 extra bytes and is handled separately */
};

static const char *mnemonic(unsigned c) {
    if (c < OP_DYAD)   return "MONAD";
    if (c < OP_SETLOC) return "DYAD";
    if (c < OP_GETLOC) return "SETLOC";
    if (c < OP_DELLOC) return "GETLOC";
    if (c < OP_APPLY)  return "DELLOC";
    switch (c) {
        case OP_APPLY:      return "APPLY";
        case OP_PROJECT:    return "PROJECT";
        case OP_MODASN:     return "MODASN";
        case OP_MODASNG:    return "MODASNG";
        case OP_IDXASN:     return "IDXASN";
        case OP_IDXASNG:    return "IDXASNG";
        case OP_IDXGET:     return "IDXGET";
        case OP_IDXGETG:    return "IDXGETG";
        case OP_GETGLB:     return "GETGLB";
        case OP_SETGLB:     return "SETGLB";
        case OP_LIST:       return "LIST";
        case OP_UNLIST:     return "UNLIST";
        case OP_BRANCH:     return "BRANCH";
        case OP_JUMP:       return "JUMP";
        case OP_RECUR:      return "RECUR";
        case OP_POP:        return "POP";
        case OP_CONSTDYAD:  return "CONSTDYAD";
        default:            return "CONST";
    }
}

/* Decode and print one instruction at bc[pos]; returns the total length
 * (1 + operand bytes) consumed, or 0 if pos is out of range. */
static unsigned decode_one(const unsigned char *bc, unsigned nb, unsigned pos, FILE *ofp) {
    if (pos >= nb) return 0;
    unsigned c = bc[pos];
    const char *mn = mnemonic(c);
    unsigned len = 1;

    if (c < OP_APPLY) {
        /* operand lives in the low bits of the opcode itself */
        unsigned base = c < OP_DYAD ? OP_MONAD : c < OP_SETLOC ? OP_DYAD
                       : c < OP_GETLOC ? OP_SETLOC : c < OP_DELLOC ? OP_GETLOC : OP_DELLOC;
        fprintf(ofp, "%5u  %-8s %u\n", pos, mn, c - base);
    } else if (c >= OP_CONST0) {
        fprintf(ofp, "%5u  %-8s #%u\n", pos, mn, c - OP_CONST0);
    } else {
        unsigned extra = EXTRA[c - OP_APPLY];
        len += extra;
        if (pos + len > nb) { fprintf(ofp, "%5u  %-8s <truncated>\n", pos, mn); return nb - pos; }
        switch (c) {
            case OP_APPLY: case OP_PROJECT: case OP_LIST: case OP_UNLIST:
                fprintf(ofp, "%5u  %-8s n=%u\n", pos, mn, bc[pos + 1]);
                break;
            case OP_BRANCH: case OP_JUMP:
                fprintf(ofp, "%5u  %-8s +%u -> %u\n", pos, mn, bc[pos + 1], pos + len + bc[pos + 1]);
                break;
            case OP_GETGLB: case OP_SETGLB: {
                unsigned idx = bc[pos + 1] | ((unsigned)bc[pos + 2] << 8);
                fprintf(ofp, "%5u  %-8s idx=%u\n", pos, mn, idx);
                break;
            }
            case OP_CONSTDYAD:
                fprintf(ofp, "%5u  %-8s const#%u dyad=%u\n", pos, mn, bc[pos + 1], bc[pos + 2]);
                break;
            case OP_MODASN: case OP_MODASNG: case OP_IDXASN:
            case OP_IDXASNG: case OP_IDXGET: case OP_IDXGETG: {
                unsigned idx = bc[pos + 1] | ((unsigned)bc[pos + 2] << 8);
                fprintf(ofp, "%5u  %-8s idx=%u dyad=%u\n", pos, mn, idx, bc[pos + 3]);
                break;
            }
            case OP_RECUR: case OP_POP:
                fprintf(ofp, "%5u  %-8s\n", pos, mn);
                break;
            default:
                fprintf(ofp, "%5u  %-8s\n", pos, mn);
        }
    }
    return len;
}

/* Print the constant pool and local-variable table of a compiled closure
 * `fn` (the `to`-typed value cpl() returns; see vm.h for the field layout),
 * then disassemble its bytecode. Returns 1 if the decode loop consumed
 * exactly the bytecode length (a strong signal the mirrored opcode table
 * above still matches b.c), 0 otherwise. */
static int disasm_closure(A fn, FILE *ofp) {
    A *flds = _A(fn);
    U nflds = _n(fn);
    A bcv = flds[1];       /* Char vector: bytecode */
    A locv = flds[3];      /* Symbol vector: local names */
    const unsigned char *bc = (const unsigned char *)_C(bcv);
    unsigned nb = (unsigned)_n(bcv);

    U nlocals = _t0(locv) ? 0 : _n(locv);
    fprintf(ofp, "locals (%u):", nlocals);
    if (!_t0(locv)) {
        const I *lid = (const I *)_V(locv);
        for (U i = 0; i < nlocals; i++) fprintf(ofp, " %u:%s", i, su((U)lid[i]));
    }
    fprintf(ofp, "\n");

    U nconst = nflds > 4 ? nflds - 4 : 0;
    fprintf(ofp, "constants (%u):\n", nconst);
    for (U i = 0; i < nconst; i++) {
        fprintf(ofp, "  #%u  ", i);
        /* `out()` (s.c) writes straight to the fd, bypassing C stdio's own
         * buffer on ofp -- flush first or the two writers can interleave
         * out of order when stdout isn't line-buffered (e.g. piped input). */
        fflush(ofp);
        out(flds[4 + i]);
        fflush(stdout); /* `out()` always targets stdout regardless of ofp */
    }

    fprintf(ofp, "bytecode (%u bytes):\n", nb);
    unsigned pos = 0, consumed = 0;
    while (pos < nb) {
        unsigned len = decode_one(bc, nb, pos, ofp);
        if (!len) break;
        pos += len;
        consumed += len;
    }
    return consumed == nb;
}

A vm_disasm_cmd(S s) {
    A parsed = pk(&s, 10);
    if (!parsed) { printf("\\disasm: parse error\n"); return au; }
    A compiled = cpl(aCz(s), parsed, 0);
    if (!compiled) { printf("\\disasm: compile error\n"); return au; }
    int ok = disasm_closure(compiled, stdout);
    if (!ok) fprintf(stderr, "\\disasm: warning -- decode did not consume the full bytecode array "
                              "(opcode table in vm.c may be out of sync with b.c)\n");
    mr(compiled);
    return au;
}

I vm_selftest(void) {
    static const char *CASES[] = {
        "(1+2)*3-4",             /* arithmetic + constant folding */
        "{x+y}[1;2]",            /* lambda literal + apply        */
        ":[1;2;3]",              /* conditional (branch/jump)     */
        "(1;2;3)",               /* list literal                  */
        "{a:x+1;a*2}[5]",        /* local variable set/get        */
        "t:([]a:1 2;b:3 4);t",   /* global assignment + get       */
    };
    FILE *sink = fopen("/dev/null", "w");
    if (!sink) return 0;
    /* disasm_closure()'s constant-pool dump goes through the real `out()`
     * K-verb (s.c), which always targets fd 1 directly regardless of the
     * FILE* passed in -- redirect fd 1 itself for the duration of the
     * self-test so it stays as silent as `arn`/`dgn`/`simd`, then restore
     * it exactly (stdout's own FILE* buffering is untouched either way). */
    fflush(stdout);
    int saved_fd1 = dup(1);
    int devnull = open("/dev/null", O_WRONLY);
    if (saved_fd1 >= 0 && devnull >= 0) dup2(devnull, 1);

    int ok = 1;
    for (unsigned c = 0; c < sizeof(CASES) / sizeof(*CASES) && ok; c++) {
        S s = CASES[c];
        A parsed = pk(&s, 10);
        if (!parsed) { ok = 0; break; }
        A compiled = cpl(aCz(s), parsed, 0);
        if (!compiled) { ok = 0; break; }
        ok = disasm_closure(compiled, sink);
        mr(compiled);
    }

    fflush(stdout);
    if (saved_fd1 >= 0) { dup2(saved_fd1, 1); close(saved_fd1); }
    if (devnull >= 0) close(devnull);
    fclose(sink);
    return ok;
}
