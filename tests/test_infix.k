/ ============================================================================
/ tests/test_infix.k  -  named two-argument library verbs work as right-to-left
/ infix operators.  GNU AGPLv3 - see LICENSE and NOTICE.
/ ----------------------------------------------------------------------------
/     ./amber tests/test_infix.k          (exits non-zero on any failure)
/ ----------------------------------------------------------------------------
/ amber 2.0.0 generalised infix (src/p.c + src/m.c am_infix_dyad): the parser
/ treats ANY name that resolves to a rank-2 global function as an infix verb, so
/ `x f y` parses as f[x;y] with k's right-to-left associativity -- uniformly, not
/ from a hard-coded name list.  Monadic verbs (rank 1) and inline {lambda}s (not
/ names) are NEVER coerced, so `avg {x*x} x` keeps its meaning.
/ The core check is `x f y ~ f[x;y]` -- bit-identical to the nested bracket form,
/ so a case is correct without knowing what f actually computes.
/ ============================================================================
DIR:{$[#p:"/"/-1_"/"\x;p;"."]}`argv 1
LOAD:{{. x^(`c$13)}'0:x;}
LOAD DIR,"/../amber.k"
LOAD DIR,"/../std.k"
LOAD DIR,"/../qsql.k"
LOAD DIR,"/harness.k"

ti :{[tag;got;want]hpush[tag;got~want;"FAIL got<",(hs got),"> want<",(hs want),">"]}
tir:{[tag;got]hpush[tag;1b;""]}                 / reached here => the expr ran clean
tie:{[tag;q]e:.[{(1b;. x)};,q;{(0b;x)}];hpush[tag;~e 0;"FAIL expected an error <- ",q]}

t:([]a:1 2 3;b:10 20 30)
q:([a:1 2]c:`x`y)

/ ---- one representative per category: infix == nested bracket, bit-identical --
ti["xkey";    `a xkey t;             xkey[`a;t]]
ti["xasc";    `a xasc t;             xasc[`a;t]]
ti["xdesc";   `b xdesc t;            xdesc[`b;t]]
ti["in";      1 2 5 in 2 3 4;        in[1 2 5;2 3 4]]
ti["within";  2 within 1 3;          within[2;1 3]]
ti["except";  1 2 3 except 2;        except[1 2 3;2]]
ti["inter";   1 2 3 inter 2 3 4;     inter[1 2 3;2 3 4]]
ti["union";   1 2 union 2 3;         union[1 2;2 3]]
ti["sublist"; 2 sublist 10 20 30;    sublist[2;10 20 30]]
ti["wsum";    1 2 3 wsum 4 5 6;      wsum[1 2 3;4 5 6]]
ti["wavg";    1 2 3 wavg 4 5 6;      wavg[1 2 3;4 5 6]]
ti["cor";     1 2 3 cor 2 4 6;       cor[1 2 3;2 4 6]]
ti["cov";     1 2 3 cov 2 4 6;       cov[1 2 3;2 4 6]]
ti["like";    "hello" like "he*";    like["hello";"he*"]]
ti["ss";      "abcabc" ss "bc";      ss["abcabc";"bc"]]
ti["sv";      "," sv ("a";"b";"c");  sv[",";("a";"b";"c")]]
ti["vs";      "," vs "a,b,c";        vs[",";"a,b,c"]]

/ ---- right-to-left chaining of 3+ infix calls -------------------------------
conntable:([]cname:`x`y`z;v:10 20 30)
ti["reported-idiom"; `cname xkey 1 rotate () xkey conntable; xkey[`cname;rotate[1;xkey[();conntable]]]]
ti["chain3-rassoc";  1 2 3 except 2 union 5 6;                except[1 2 3;union[2;5 6]]]

/ ---- mixed infix + bracket in one expression --------------------------------
ti["mixed-outer-monad"; sum 1 2 3 wsum 4 5 6;   sum wsum[1 2 3;4 5 6]]
ti["mixed-bracket-arg"; except[1 2 3;2] union 5; union[except[1 2 3;2];5]]

/ ---- infix inside qSQL (the where clause is rewritten by qrw, then parsed) ---
tir["qsql-in-where";    . qrw "select from t where a in 1 3"]

/ ---- generalisation: a rank-2 USER function also goes infix ------------------
f:{x+2*y}
ti["user-dyad-infix";   10 f 3;                 f[10;3]]

/ ---- negative: monadic verbs and inline lambdas are NEVER coerced -----------
ti["monadic-avg-lambda"; avg {x*x} 1 2 3.0;     (1.0+4+9)%3]   / must NOT rank-error
ti["monadic-sum";        sum 1 2 3;             6]
ti["monadic-neg-notfix"; neg 1 2 3;             -1 -2 -3]

hexpect[26]
hreport"AMBER INFIX DYADS"
$[FAILN;`exit 1;]
