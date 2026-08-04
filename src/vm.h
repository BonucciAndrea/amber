/* vm.h  -  documentation + disassembler for Amber's existing bytecode VM.
 * GNU AGPLv3 - see LICENSE and NOTICE.
 * Requires a.h to already be included by the translation unit.
 *
 * IMPORTANT -- read this before looking for "the VM": Amber already compiles
 * every expression to a flat bytecode array and runs it on a stack machine.
 * That compiler and VM are `cr()`/`cpl()` (AST -> bytecode + constant pool)
 * and `run()` (the execution loop) in src/b.c -- they are the real, hot-path
 * core evaluator, and per this project's own safety guardrails they are not
 * touched here (or anywhere in this feature set). This file does not add a
 * second, parallel VM; it adds read-only introspection into the one that
 * already exists, the same way src/ast.c visualises the parser's output and
 * src/trace.c times the real pipeline without altering it.
 *
 * A compiled closure (the `to`-typed value b.c's cpl() returns) is a plain
 * `A` array with this layout (see b.c: OFF, cc(), and the final cpl() body):
 *
 *   [0]         source        the original expression, as an "annotated
 *                              char vector" (source text + parse position)
 *                              used for error locations
 *   [1]         bytecode       a Char vector: the flat opcode stream, one
 *                              instruction after another, no padding
 *   [2]         map            a Char vector, same length as [1]: map[i] is
 *                              the AST node index that compiled to
 *                              bytecode[i], used to point an error at the
 *                              right source span
 *   [3]         locals         a Symbol vector of local-variable names, in
 *                              slot order
 *   [4..]       constants      every literal value the expression folds to
 *                              a "load constant" instruction for (numbers,
 *                              symbols, quoted lists, ...), in append order
 *
 * The opcode set below (byte value, mnemonic, operand layout) is mirrored
 * byte-for-byte from the private `enum{...}` at the top of b.c and from its
 * `di[]` (extra operand bytes per opcode) table, so vm_disasm() can decode
 * without depending on any symbol exported by b.c. If b.c's opcode values
 * ever change, this mirror must be updated to match -- vm_disasm() defends
 * against silent drift with a self-check: it always verifies the decoded
 * instructions consume exactly the bytecode array's length; see vm.c.
 *
 *   byte value   mnemonic     operand bytes           stack effect
 *   ----------   -----------  ----------------------  --------------------------
 *   0..31        MONAD n      (n encoded in opcode)   x -> monads[n](x)
 *   32..63       DYAD n       (n encoded in opcode)   y x -> dyads[n](x,y)
 *   64..79       SETLOC i     (i encoded in opcode)   x -> ..              locals[i]:=x
 *   80..95       GETLOC i     (i encoded in opcode)   -> .. locals[i]
 *   96..111      DELLOC i     (i encoded in opcode)   -> ..                locals[i]:=null
 *   112          APPLY n      1: n (arg count)         .. args f -> f[args]
 *   113          PROJECT n    1: n (arg count)         .. args f -> partial application
 *   114          MODASN i,d   3: idxLo,idxHi,dyad      x -> ..              var[i] op= x
 *   115          MODASNG i,d  3: idxLo,idxHi,dyad      x -> ..              global[i] op= x
 *   116          IDXASN i,d   3: idxLo,idxHi,dyad      z y -> ..            var[i][y]:=dyad(.,z)
 *   117          IDXASNG i,d  3: idxLo,idxHi,dyad      z y -> ..            global[i][y]:=dyad(.,z)
 *   118          IDXGET i,d   3: idxLo,idxHi,dyad      z y -> .. r          r:=.[var[i];y;dyad;z]
 *   119          IDXGETG i,d  3: idxLo,idxHi,dyad      z y -> .. r          r:=.[global[i];y;dyad;z]
 *   120          GETGLB i     2: idxLo,idxHi           -> .. globals[i]
 *   121          SETGLB i     2: idxLo,idxHi           x -> ..              globals[i]:=x
 *   122          LIST n       1: n (element count)     .. -> .. (list of n)
 *   123          UNLIST n     1: n (element count)     x -> .. x[0] x[1] ..
 *   124          BRANCH n     1: n (byte offset)        x -> ..             if falsy, PC += n
 *   125          JUMP n       1: n (byte offset)         -> ..              PC += n
 *   126          RECUR        0                         -> .. self
 *   127          POP          0                        x -> ..
 *   128          CONSTDYAD i,d 2: constIdx,dyad         x -> .. r           r:=dyad(consts[i],x)
 *   129+i        CONST i      0 (i encoded in opcode)   -> .. consts[i]
 *
 * (MONAD/DYAD "n" select from the same builtin tables b.c's run() indexes
 * with v1[]/v2[]; GETLOC/SETLOC/DELLOC "i" select a local slot 0..15.)
 */
#ifndef AMBER_VM_H
#define AMBER_VM_H

/* \disasm <expr>: compile (but do not run) `s`, then print its bytecode as
 * one line per instruction (offset, mnemonic, operands) followed by the
 * constant pool and local-variable table. Never evaluates the expression. */
A vm_disasm_cmd(S s);

/* Self-test (backs the `` `vmd`` builtin, a.c): compiles a handful of
 * expressions exercising different opcode shapes (arithmetic/constant
 * folding, lambda + apply, conditionals, list literals, local variables)
 * and disassembles each, discarding the printed text but keeping the
 * "decode consumed exactly the bytecode length" self-check from each one.
 * Returns 1 iff every case's mirrored opcode table stayed byte-accurate. */
I vm_selftest(void);

#endif /* AMBER_VM_H */
