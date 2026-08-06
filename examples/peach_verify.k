/ ============================================================================
/ examples/peach_verify.k  -  binary serializer (-8!/-9!) and peach verifier.
/ GNU AGPLv3 - see LICENSE and NOTICE.
/ ----------------------------------------------------------------------------
/ Run:  ./amber examples/peach_verify.k
/       AMBER_THREADS=4 ./amber examples/peach_verify.k    / exercise the fork path
/ 
/ Four things are checked, in order:
/   1. (-9! -8! x) ~ x  over every shape the serializer claims to support.
/   2. peach returns exactly what serial each returns, for every result shape
/      (atoms, symbols, nested lists, dicts, tables) -- i.e. the binary wire
/      round trip is transparent to the caller.
/   3. peach over a large input, to show the parent's accumulate loop no longer
/      leaks a whole result vector per chunk (1.9.3 fix) and stays linear.
/   4. an error raised INSIDE a worker surfaces as a clean, trappable K error
/      rather than a hang, a wrong answer, or a crash.
/ Exits non-zero if anything fails, so it can be wired into CI as-is.
/ ============================================================================
dir:{$[#p:"/"/-1_"/"\x;p;"."]}`argv 1
LOAD:{{. x^(`c$13)}'0:x;}
LOAD dir,"/../amber.k"
LOAD dir,"/../std.k"

n:0;f:0
ok:{[nm;b]n::n+1;$[b;;[f::f+1;`0:"  FAIL: ",nm]];}
sec:{`0:"";`0:x;`0:(#x)#"-";}

/ ---- 1. serializer round trip ---------------------------------------------
sec"1. binary serializer  (-9! -8! x) ~ x"
rt:{[nm;v]ok[nm;.[{[v](-9!-8!v)~v};,v;{[e]0b}]];}
rt["int atom"        ;42]
rt["long atom"       ;123456789012345]
rt["float atom"      ;3.14159]
rt["char atom"       ;"q"]
rt["symbol atom"     ;`hello]
rt["long symbol"     ;`$"symbolnamelongerthaneightbytes"]
rt["date atom"       ;2024.01.15]
rt["time atom"       ;12:34:56.789]
rt["bool vector"     ;101011b]
rt["byte vector"     ;0x00ff7f]
rt["int vector"      ;1 2 3 4 5]
rt["long vector"     ;1000000000000 2 3]
rt["float vector"    ;1.5 2.5 3.5]
rt["char vector"     ;"hello world"]
rt["symbol vector"   ;`a`b`c]
rt["range"           ;!10]
rt["empty int"       ;0#0]
rt["empty symbol"    ;0#`]
rt["empty char"      ;""]
rt["empty general"   ;()]
rt["nested list"     ;(1 2;("ab";`c`d);3.5)]
rt["deeply nested"   ;(((1 2;3);(4;(5 6))))]
rt["null int"        ;0N]
rt["null float"      ;0n]
rt["infinity"        ;0w]
rt["negative infinity";-0w]
rt["nulls in vector" ;0N 1 2]
rt["nan in vector"   ;0n,1.5 2.5]
rt["infinities"      ;0w,-0w,1.0]
rt["dictionary"      ;`a`b!(1 2;3 4)]
rt["mixed dictionary";`x`y`z!(1;"ab";`c)]
rt["table"           ;+`a`b!(1 2 3;4 5 6)]
rt["keyed table"     ;xkey[`k;+`k`v!(`a`b;1 2)]]
rt["nested table"    ;+`a`b!((1 2;3 4);(`x`y;`z`w))]
rt["wide table"      ;+`s`p!(1000#`a`b;1000#1.5 2.5)]

/ attributes must survive: a sorted column that arrives unsorted-flagged would
/ silently lose the O(log n) binary-search path in `?` on the far side.
sa:`sa 1 2 3 4 5
ok["sorted attribute preserved";(`at sa)~`at -9!-8!sa]
ok["sorted attribute value"    ;(`at -9!-8!sa)~`s]

/ malformed input must be rejected cleanly, never crash: -9! parses bytes that
/ in the peach case came from another process and may be truncated.
bad:{[nm;g]ok[nm;"caught"~.[g;,0;{[e]"caught"}]];}
bad["reject empty input"    ;{-9!""}]
bad["reject garbage header" ;{-9!"XXXXXXXX"}]
bad["reject truncated body" ;{-9!8#-8!1 2 3 4 5}]
bad["reject non-char input" ;{-9!1 2 3}]
bad["reject trailing bytes" ;{-9!(-8!5),"junk"}]
bad["reject lambda encode"  ;{-8!{x+1}}]

/ ---- 2. peach result shapes ------------------------------------------------
sec"2. peach result shapes match serial each"
same:{[nm;g;d]ok[nm;(peach[g;d])~g'd];}
same["integers"      ;{x*x}              ;!20]
same["floats"        ;{0.5*x}            ;!50]
same["symbols"       ;{`$"s",$x}         ;!10]
same["char vectors"  ;{$x}               ;!10]
same["nested lists"  ;{(x;x*2)}          ;!8]
same["dictionaries"  ;{`a`b!(x;x*2)}     ;!6]
same["tables"        ;{+`v!,x+!3}        ;!5]
same["mixed widths"  ;{$[2!x;x;0.5*x]}   ;!12]
same["over symbols"  ;{$x}               ;`a`b`c`d`e`f]
same["single item"   ;{x*2}              ;,7]

/ ---- 3. large peach: correctness and flat memory ---------------------------
sec"3. large peach"
N:500000
big:!N
t0:`t 0
rp:peach[{1+x*2};big]
t1:`t 0
ok["500k peach correct" ;rp~1+2*big]
ok["500k peach count"   ;N=#rp]
`0:("  500,000 items in ",($0.001*0.0+t1-t0)," ms")
/ Before 1.9.3 the parent's `out=cat(out,part)` never released the previous
/ accumulator, so resident bytes grew with the number of chunks AND the append
/ reallocated every time (MINE(out) was false, so aa() could not grow in
/ place). `bi 0 reports (total bytes reserved; regions; arch; cc; version);
/ index 0 is the figure that used to climb on every repeat.
hp:{[q].[{(`bi 0)0};,0;{[e]0}]}
u0:hp 0
{[j]peach[{1+x*2};big];}'!3;
u1:hp 0
/ NB: K has no `<=`; writing it parses as `<` applied to MONADIC `=` (group),
/ which enumerates the whole integer -- use "not greater than" instead.
ok["heap flat across repeats";~u1>u0+16000000]
`0:("  heap bytes before ",($u0),", after 3 more passes ",($u1))

/ ---- 4. worker error propagation -------------------------------------------
sec"4. worker error propagation"
/ NB: use a REAL evaluation error (a type error). `'"msg"` does not raise a
/ trappable error in Amber -- `'` is the each adverb, not signal -- so a test
/ built on it would pass or fail for reasons unrelated to peach.
e1:.[{[q]peach[{$[x=5;x+`sym;x]};!10]};,0;{[e]"caught"}]
ok["worker error is trapped";e1~"caught"]
e2:.[{[q]peach[{$[x=400;x+`sym;x]};!1000]};,0;{[e]"caught"}]
ok["error deep in a chunk"  ;e2~"caught"]
/ the interpreter must still be usable afterwards: no orphaned pipes, no
/ zombie workers, no wedged global error state.
ok["peach works after error";(peach[{x+1};!30])~1+!30]
ok["serial each after error";({x+1}'!5)~1+!5]

/ ---- summary ----------------------------------------------------------------
`0:""
`0:"================ PEACH / SERIALIZER VERIFY ================"
`0:(($n)," tests run, ",($f)," failures")
$[f;`0:"FAILURES PRESENT";`0:"ALL PEACH/SERIALIZER TESTS PASSED"]
`0:"==========================================================="
$[f;`exit 1;]
