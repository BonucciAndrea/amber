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

/ ---- amber 2.2: a NAME that happens to be an infix verb is still a name -----
/ The infix reading was made from the name alone, so a lambda PARAMETER called
/ `ss` was parsed as the string-search verb: `1_ss` built a projection instead
/ of dropping and the function silently returned its input.  The same applied to
/ a global the program had rebound to data, because the hard-coded keyword list
/ claims a name whatever the name currently holds.
pdrop:{[r;ss]1_ss}
ti["param-shadows-ss";     #pdrop[0;,(1;2)];    0]
ti["param-shadows-ss-2";   pdrop[0;(1;2;3)];    (2;3)]
pin:{[x;in]1_in}
ti["param-shadows-in";     pin[0;1 2 3];        2 3]
pex:{[except]#except}
ti["param-shadows-except"; pex[1 2 3 4];        4]
pxa:{[xasc;xdesc]xasc+xdesc}
ti["param-shadows-xasc";   pxa[10;3];           13]
/ a rebound GLOBAL is data too, not a verb
ss:1 2 3
ti["global-rebound-ss";    #1_ss;               2]
ti["global-rebound-drop";  1_ss;                2 3]
ss:{[s;p]$[(#s)<#p;!0;&{[s;p;i]p~(#p)#i_s}[s;p]'!1+(#s)-#p]}   / put it back
ti["ss-restored-infix";    "abcabc" ss "bc";    ss["abcabc";"bc"]]
/ ... and the ordinary infix readings are all still infix
ti["still-infix-in";       3 in 1 2 3;          1b]
ti["still-infix-except";   1 2 3 except 2;      1 3]
ti["still-infix-xasc";     `a xasc t;           xasc[`a;t]]

/ ---- amber 2.2: `in` and the set ops accept an ATOM on the right -----------
/ An atom y fell through the C membership kernel onto `~^y?x`, and with y an
/ integer atom `y?x` is k's RANDOM DEAL, not find -- so `1 2 3 in 2` answered
/ `1 1`: two random picks, the wrong values AND the wrong length, with no error.
/ A symbol atom raised 'domain.  q accepts an atom on the right of all four.
ti["in-atom-int";      1 2 3 in 2;          0 1 0]
ti["in-atom-sym";      `a`b`c in `b;        0 1 0]
ti["in-atom-float";    1.0 2 3 in 2.0;      0 1 0]
ti["in-atom-char";     "abc" in "b";        0 1 0]
ti["in-atom-miss";     1 2 3 in 9;          0 0 0]
ti["in-atom-vs-1vec";  1 2 3 in 2;          1 2 3 in ,2]
ti["except-atom";      1 2 3 except 2;      1 3]
ti["except-atom-sym";  `a`b`c except `b;    `a`c]
ti["except-atom-miss"; 1 2 3 except 9;      1 2 3]
ti["inter-atom";       1 2 3 inter 2;       ,2]
ti["inter-atom-miss";  1 2 3 inter 9;       !0]
ti["union-atom";       1 2 3 union 5;       1 2 3 5]
ti["union-atom-dup";   1 2 3 union 2;       1 2 3]
/ the vector forms are untouched
ti["in-vec-unchanged";     1 2 3 in 2 3;    0 1 1]
ti["except-vec-unchanged"; 1 2 3 except 2 3;,1]

hexpect[52]
hreport"AMBER INFIX DYADS"
$[FAILN;`exit 1;]
