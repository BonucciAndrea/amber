/ ============================================================================
/ tests/test_qsql.k  -  qSQL query matrix.
/ GNU AGPLv3 - see LICENSE and NOTICE.
/ ----------------------------------------------------------------------------
/ Run it from anywhere:
/     ./amber tests/test_qsql.k
/     /path/to/amber /path/to/amber/tests/test_qsql.k
/ Exits non-zero if any case fails.
/ ----------------------------------------------------------------------------
/ These cases are written in the BARE qSQL syntax you actually type at the
/ prompt -- "select px from trades where px>25" -- not the sel"..." wrapper.
/ The bare form is not K syntax: repl.k's line1 runs every input line through
/ qsql.k's rewriter (qrw) first, which turns it into the matching sel/exq/upd/
/ del call. A .k script loaded with \l does NOT go through line1, so the
/ helpers below apply the exact same rewrite the REPL does -- ". qrw q" -- and
/ the suite therefore exercises the rewriter and the query engine together, on
/ the same path a user's keystrokes take. Section 9 checks that the explicit
/ sel"..." wrapper still gives an identical answer.
/ ----------------------------------------------------------------------------
/ Covered: the full clause lattice of select / exec / update / delete, with and
/ without where, with and without by, single vs multiple by keys, aggregations
/ over empty / single-row / heavily-duplicated tables, the functional qexec
/ form, and malformed queries asserted to raise cleanly rather than crash.
/ ============================================================================
DIR:{$[#p:"/"/-1_"/"\x;p;"."]}`argv 1
LOAD:{{. x^(`c$13)}'0:x;}
LOAD DIR,"/../amber.k"
LOAD DIR,"/../std.k"
LOAD DIR,"/../qsql.k"
LOAD DIR,"/harness.k"

/ ---- query assertions: the argument IS the query text ----------------------
/ tq  [tag; query; want]     assert  (. qrw query) ~ want
/ tqa [tag; query; f; want]  assert  (f . qrw query) ~ want   (f = #, cols, ...)
/ tqe [tag; query]           assert  the query raises a clean error
/ tqk [tag; query]           assert  the query runs (value ignored)
tq:{[tag;q;want]e:.[{(1b;. qrw x)};,q;{(0b;x)}];$[e 0;hpush[tag;(e 1)~want;"FAIL got<",(hs e 1),"> want<",(hs want),">  <-  ",q];hpush[tag;0b;"FAIL raised: ",(he e 1),"  <-  ",q]]}
tqa:{[tag;q;f;want]e:.[{[f;q](1b;f(. qrw q))}[f];,q;{(0b;x)}];$[e 0;hpush[tag;(e 1)~want;"FAIL got<",(hs e 1),"> want<",(hs want),">  <-  ",q];hpush[tag;0b;"FAIL raised: ",(he e 1),"  <-  ",q]]}
tqe:{[tag;q]e:.[{(1b;. qrw x)};,q;{(0b;x)}];hpush[tag;~e 0;"FAIL expected an error  <-  ",q]}
tqk:{[tag;q]e:.[{(1b;. qrw x)};,q;{(0b;x)}];$[e 0;hpush[tag;1b;""];hpush[tag;0b;"FAIL unexpected error: ",(he e 1),"  <-  ",q]]}
NR:{#x}
CL:{cols x}

trades:([]sym:`a`b`a`b`c`a; px:10 20 30 40 50 60.0; sz:1 2 3 4 5 6)
mt:([]a:`x`x`y`y`x; b:1 2 1 2 1; v:10 20 30 40 50)
one:([]sym:,`a; px:,1.0; sz:,1)
none:0#trades
dup:([]sym:1000#`a`b; px:1000#1.0 2.0; sz:1000#1 2)

/ ============================ 1. select: clause lattice =====================
tq[`selAll;       "select from trades";                trades];
tq[`selStar;      "select * from trades";              trades];
tq[`selCols;      "select sym,px from trades";         ([]sym:`a`b`a`b`c`a; px:10 20 30 40 50 60.0)];
tq[`selOneCol;    "select px from trades";             ([]px:10 20 30 40 50 60.0)];
tq[`selWhere;     "select from trades where px>25";    ([]sym:`a`b`c`a; px:30 40 50 60.0; sz:3 4 5 6)];
tq[`selWhereCol;  "select px from trades where px>25"; ([]px:30 40 50 60.0)];
tq[`selWhereAnd;  "select from trades where px>15, sz<5"; ([]sym:`b`a`b; px:20 30 40.0; sz:2 3 4)];
tq[`selRename;    "select p:px from trades";           ([]p:10 20 30 40 50 60.0)];
tq[`selAgg;       "select tot:sum px from trades";     ([]tot:,210.0)];
tq[`selTwoAgg;    "select a:sum px, b:max sz from trades"; ([]a:,210.0; b:,6)];
tq[`selExprAgg;   "select v:sum px*sz from trades";    ([]v:,910.0)];
tqa[`selWhereNone; "select from trades where px>1000"; NR; 0];
tqa[`selWhereAll;  "select from trades where px>0";    NR; 6];

/ ---- by: grouping ----------------------------------------------------------
tq[`selByAgg;     "select tot:sum px by sym from trades"; ((+(,`sym)!,`a`b`c)!+(,`tot)!,100 60 50.0)];
tq[`selByNoProj;  "select by sym from trades";        ((+(,`sym)!,`a`b`c)!+`px`sz!(60 40 50.0;6 4 5))];
tqa[`selByCount;   "select by sym from trades";        NR; 3];
tqa[`selByWhere;   "select tot:sum px by sym from trades where px>25"; NR; 3];
tq[`selByTwoKeys; "select s:sum v by a,b from mt";    ((+`a`b!(`x`x`y`y;1 2 1 2))!+(,`s)!,60 20 30 40)];
tqa[`selByTwoCount;"select s:sum v by a,b from mt";    NR; 4];
tqa[`selByKeyed;   "select s:sum v by a from mt";      {iskeyed x}; 1b];

/ ======================== 2. empty / single-row tables =======================
tqa[`emptySelect;  "select from none";                 NR; 0];
tqa[`emptyCols;    "select from none";                 CL; `sym`px`sz];
tqa[`emptyWhere;   "select from none where px>0";      NR; 0];
tqa[`emptyBy;      "select t:sum px by sym from none"; NR; 0];
tqa[`emptyAgg;     "select t:sum px from none";        NR; 1];
tq[`emptyAggVal;  "select t:sum px from none";        ([]t:,0.0)];
tqa[`oneSelect;    "select from one";                  NR; 1];
tqa[`oneWhereHit;  "select from one where px>0";       NR; 1];
tqa[`oneWhereMiss; "select from one where px>99";      NR; 0];
tqa[`oneBy;        "select t:sum px by sym from one";  NR; 1];
tq[`oneAgg;       "select t:sum px from one";         ([]t:,1.0)];

/ ==================== 3. heavily duplicated key sets ========================
tqa[`dupBy;        "select t:sum sz by sym from dup";  NR; 2];
tqa[`dupByTotal;   "select t:sum sz by sym from dup";  {+/(value x)`t}; 1500];
tqa[`dupCount;     "select c:#:px by sym from dup";    {+/(value x)`c}; 1000];
tqa[`dupWhereBy;   "select t:sum sz by sym from dup where sz>1"; NR; 1];
tqa[`dupDistinct;  "select from dup";                  {#?x`sym}; 2];

/ ============================ 4. exec ========================================
tq[`execCol;      "exec px from trades";              10 20 30 40 50 60.0];
tq[`execAgg;      "exec sum px from trades";          210.0];
tq[`execWhere;    "exec px from trades where px>35";  40 50 60.0];
tqa[`execEmpty;    "exec px from none";                NR; 0];
tq[`execSym;      "exec sym from trades";             `a`b`a`b`c`a];
/ exec with no item is the last row, one named item a dict, and ?[t;c;();a] (by ()) an exec, as q's
tq[`execNoItem;   "exec from trades";                 `sym`px`sz!(`a;60.0;6)];
tq[`execNoItemWh; "exec from trades where px<25";     `sym`px`sz!(`b;20.0;2)];
tq[`execNamed;    "exec p:px from trades where px>45"; (,`p)!,50 60.0];
tv[`fExecCol;     "?[trades;();();`sz]";              1 2 3 4 5 6];
tv[`fExecRow;     "?[trades;,(>;`px;45);();()]";      `sym`px`sz!(`a;60.0;6)];
tv[`fExecDict;    "?[trades;();();`p`s!(`px;(sum;`sz))]"; `p`s!(10 20 30 40 50 60.0;21)];

/ ============================ 5. update ======================================
tqa[`updNewCols;   "update d:px*2 from trades";        CL; `sym`px`sz`d];
tqa[`updNewVals;   "update d:px*2 from trades";        {x`d}; 20 40 60 80 100 120.0];
tqa[`updReplace;   "update px:px+1 from trades";       {x`px}; 11 21 31 41 51 61.0];
tqa[`updWhere;     "update px:0.0 from trades where px>35"; {x`px}; 10 20 30 0 0 0.0];
tqa[`updRowCount;  "update d:px*2 from trades";        NR; 6];
tqa[`updBy;        "update t:sum px by sym from trades"; NR; 6];
tqa[`updByVals;    "update t:sum px by sym from trades"; {x`t}; 100 60 100 60 50 100.0];
tqa[`updEmpty;     "update d:px*2 from none";          NR; 0];

/ ============================ 6. delete ======================================
tq[`delRows;      "delete from trades where px>35";   ([]sym:`a`b`a; px:10 20 30.0; sz:1 2 3)];
tqa[`delNone;      "delete from trades where px>1000"; NR; 6];
tqa[`delAll;       "delete from trades where px>0";    NR; 0];
tqa[`delCol;       "delete sz from trades";            CL; `sym`px];
tqa[`delTwoCols;   "delete px,sz from trades";         CL; ,`sym];
tqa[`delColKeeps;  "delete sz from trades";            NR; 6];

/ ==================== 7. functional form (qexec) ============================
t[`qexecPlain;   #qexec[trades;();();()];                    6];
t[`qexecWhere;   #qexec[trades;trades[`px]>25;();()];        4];
t[`qexecBy;      #qexec[trades;();`sym;(,`t)!,{sum x`px}];   3];
t[`qexecAgg;     (qexec[trades;();();(,`t)!,{sum x`px}])`t;  ,210.0];
t[`qwhereMask;   #qwhere[trades;trades[`px]>25];             4];
t[`qselectDict;  cols qselect[trades;(,`t)!,{sum x`px}];     ,`t];
t[`qbyGroups;    #qby[trades;`sym;(,`t)!,{sum x`px}];        3];
/ functional update-by (digest #73 item 94): each row gets its group's value, as q's ![t;c;b;a]
t[`fupdBy;       (+![trades;();(,`sym)!,`sym;(,`m)!,(max;`sz)])`m;          6 4 6 4 5 6];
t[`fupdByWhere;  (+![trades;,(>;`px;25);(,`sym)!,`sym;(,`m)!,(max;`sz)])`m; 0N 0N 6 4 5 6];
t[`fupdByTwo;    (+![mt;();`a`b!`a`b;(,`s)!,(sum;`v)])`s;                  60 20 30 40 60];
t[`fupdByUnif;   (+![mt;();(,`a)!,`a;(,`s)!,(sums;`v)])`s;                 10 30 30 70 80];
t[`fupdByI;      (+![+`sym`n!(`a`b`a`b;1 2 3 4);();(,`sym)!,`sym;(,`m)!,(+;`n;`i)])`m;        1 3 5 7];
t[`fupdByIWhere; (+![+`sym`n!(`a`b`a`b;1 2 3 4);,(>;`n;1);(,`sym)!,`sym;(,`m)!,(+;`n;`i)])`m; 0N 3 5 7];
t[`fupdByOld;    (+![trades;,(>;`px;25);(,`sym)!,`sym;(,`sz)!,(count;`i)])`sz; 1 2 2 1 1 2];

/ ==================== 8. the rewriter itself (qrw) ==========================
t[`qrwSelect;    qrw"select from trades";     "sel\"select from trades\""];
t[`qrwExec;      4#qrw"exec a from t";        "exq\""];
t[`qrwUpdate;    4#qrw"update a:1 from t";    "upd\""];
t[`qrwDelete;    4#qrw"delete from t";        "del\""];
t[`qrwPassThru;  qrw"1+1";                    "1+1"];
t[`qrwNoFrom;    qrw"select 1";               "select 1"];
t[`qrwInString;  qrw"\"select from t\"";      "\"select from t\""];
t[`qrwPlainK;    qrw"a:1 2 3";                "a:1 2 3"];

/ ============ 9. bare syntax == explicit sel"..." wrapper ===================
t[`wrapAll;      sel"select from trades";                  . qrw"select from trades"];
t[`wrapWhere;    sel"select from trades where px>25";      . qrw"select from trades where px>25"];
t[`wrapBy;       sel"select tot:sum px by sym from trades";. qrw"select tot:sum px by sym from trades"];
t[`wrapExec;     exq"exec px from trades";                 . qrw"exec px from trades"];
t[`wrapUpd;      upd"update d:px*2 from trades";           . qrw"update d:px*2 from trades"];
t[`wrapDel;      del"delete from trades where px>35";      . qrw"delete from trades where px>35"];

/ ============ 10. malformed queries: clean error, never a crash =============
tqe[`noFrom;        "select"];
tqe[`fromNothing;   "select from"];
tqe[`byNoFrom;      "select by sym"];
tqe[`unknownTable;  "select from no_such_table"];
tqe[`unknownCol;    "select from trades where nosuchcol>1"];
tqe[`whereNoPred;   "select from trades where"];
tqe[`danglingComma; "select px, from trades"];
tqe[`unbalancedBrk; "select sum[px from trades"];
tqe[`execNoFrom;    "exec px"];
tqe[`updNoFrom;     "update a:1"];
tqe[`delNoFrom;     "delete a"];
tqe[`badAggType;    "select t:sum sym from trades"];
tqe[`emptyQuery;    "sel\"\""];
tqk[`byOnly;        "select by sym from trades"];
tqk[`starWhere;     "select * from trades where sz>2"];
tqk[`nestedAgg;     "select t:sum px*sz by sym from trades"];
/ An unknown by-key now RAISES, the way q does.  Until 1.9.7 the by-clause was
/ turned into a symbol of its own source text and grouping on a column no table
/ has collapsed every row into one null-keyed group, so the query returned a
/ single wrong row and returned it silently.  By-items are compiled like any
/ other column expression now, so an unknown name is an undefined variable.
tqe[`unknownByKey;  "select t:sum px by nosuchkey from trades"];

/ ============ 11. trailing comments, multi-line text, by-expressions ========
/ Every case below used to fail, and the first three failed the worst way: the
/ website's own examples were written like this and did not run.
/ A trailing "/ comment" is stripped before the clause split, so it can no
/ longer be compiled into the generated where-lambda.
t[`cmtWhere;   . qrw"select from trades where px>35   / filter rows";  . qrw"select from trades where px>35"];
t[`cmtBy;      . qrw"select sum sz by sym from trades / grouped";      . qrw"select sum sz by sym from trades"];
t[`cmtUpd;     . qrw"update px:px*2 from trades  / double";            . qrw"update px:px*2 from trades"];
t[`cmtDel;     . qrw"delete from trades where px>35  / drop";          . qrw"delete from trades where px>35"];
tqk[`cmtOnlySlash; "select from trades where px>35 / 2"];    / an adverb, not a comment
/ A query split across lines parses as one query: the clause splitters look for
/ " from " / " where " / " by " with their spaces, which a line break destroyed.
t[`multiline;  sel"select vwap:wavg[sz;px]\nby sym\nfrom trades\nwhere px>15"; sel"select vwap:wavg[sz;px] by sym from trades where px>15"];
t[`multiCmt;   sel"select sum sz  / total\nby sym       / per symbol\nfrom trades"; sel"select sum sz by sym from trades"];
/ the same holds for the BARE rewriter, which is what an amberd client or a
/ Grafana panel hands a whole multi-line query to.
t[`multiBare;  . qrw"select vwap:wavg[sz;px]\nby sym\nfrom trades"; sel"select vwap:wavg[sz;px] by sym from trades"];
t[`multiKeeps; qrw"1+1\nnosuchword 2"; "1+1\nnosuchword 2"];   / non-qSQL multi-line text: passed through untouched
/ by-items are compiled like any other column expression, so a by-clause may be
/ an expression and not just a bare column name.
tqa[`byExpr;    "select sum sz by g:2*sz from trades"; cols; `g`sz];
tqa[`byExprNm;  "select sum sz by px:xbar[20;px] from trades"; cols; `px`sz];
tqa[`byExprMix; "select sum sz by sym, g:2*sz from trades"; cols; `sym`g`sz];
t[`byBare;     . qrw"select sum sz by sym from trades"; . qrw"select sum sz by sym from trades"];
/ an unnamed aggregate over exactly one column takes that column's name (q does
/ the same); it used to come back as x0 whatever it was computed from.
tqa[`aggName;    "select last px by sym from trades"; cols; `sym`px];
tqa[`aggNameSum; "select sum sz by sym from trades";  cols; `sym`sz];
tqa[`aggNameWide;"select wavg[sz;px] by sym from trades"; cols; `sym`sz];   / q names it by the first argument (digest #56)
/ Digest #56 #61: q's column names and repeats; an atom beside a list is extended, as update
tqa[`nameRepeats;"select max px, min px, 2*px, sum px*sz from trades"; cols; `px`px1`x`px2];
tqa[`selectExtend;"select px, s:sum sz from trades"; {(+x)`s}; 6#21];
/ within[x;(lo;hi)] -- inclusive, and the reason time-range queries now run.
t[`within;      within[10 20 30 40.0;(20;30)]; 0 1 1 0];
t[`withinQuery; . qrw"select from trades where within[px;(20;40)]"; . qrw"select from trades where (~px<20)&~px>40"];

/ ---- qrwf / qtry: the whole-file qSQL rewriter the loader (bsl) applies so
/ bare qSQL works in a .k script with no sel"..." wrapper (amber 2.0.0) ----
t[`qrwfSelect;  qrwf"select px from t";        "sel\"select px from t\""]
t[`qrwfPlain;   qrwf"a:1+2";                   "a:1+2"]
t[`qtryIdentity;qtry"x:from+select";           "x:from+select"]
t[`qrwfMulti;   qrwf"a:1\nselect px from t\nb:2"; "a:1\nsel\"select px from t\"\nb:2"]
t[`qrwfInString;qrwf"y:\"select px from t\"";  "y:\"select px from t\""]

/ select ... by <symbol column> uses the 2.0.0 symbol-grouping fast path;
/ keys are first-appearance order (c,a,b) and aggregates are correct
t[`selectBySymGroup; sel"select s:sum v by k from ([]k:`c`a`b`a`c; v:1 2 3 4 5)"; ([k:`a`b`c] s:6 3 6)]   / groups in key order, as q (2.7)

/ ---- amber 2.2: select[...] -- q's sorted and limited select --------------
/ Order is q's: where, then by/select, then sort, then limit.  A sort spec is
/ ">expr" or "<expr"; anything else in the bracket is the row limit.  Several
/ sort keys apply right-to-left so the FIRST one listed is the primary key.
st:([]sym:`c`a`b`a`c`b; px:3.0 1 2 4 6 5; sz:30 10 20 40 60 50)
t[`selLimit;      sel"select[3] from st";        3#st]
t[`selLimitNeg;   sel"select[-2] from st";       -2#st]
sh:2#st
t[`selLimitPast;  sel"select[5] from sh";        sh]
t[`selLimitPastNeg; sel"select[-3] from sh";     sh]
t[`selLimitCols;  sel"select[3] sym,px from st"; 3#sel"select sym,px from st"]
t[`selLimitZero;  sel"select[0] from st";        0#st]
t[`selSortDesc;   sel"select[>px] from st";      st@>st`px]
t[`selSortAsc;    sel"select[<px] from st";      st@<st`px]
t[`selSortSym;    (sel"select[<sym] from st")`sym; `a`a`b`b`c`c]
t[`selSortLimit;  sel"select[2;>px] from st";    2#st@>st`px]
t[`selSortLimitA; sel"select[2;<px] from st";    2#st@<st`px]
/ two keys: sym ascending, px descending inside each sym
t[`selTwoKeys;    (sel"select[<sym;>px] from st")`px; 4.0 1 5 2 6 3]
t[`selSortWhere;  sel"select[2;>px] from st where sz>15"; 2#(st@&(st`sz)>15)@>(st@&(st`sz)>15)`px]
/ a sort key may be any expression over the columns, not only a column name
t[`selSortExpr;   (sel"select[>px*sz] from st")`px; 6.0 5 4 3 2 1]
/ ... and it applies to a KEYED (by-clause) result with the key put back
t[`selSortKeyed;  (value sel"select[>s] s:sum px by sym from st")`s; 9.0 7 5]
t[`selLimitKeyed; #sel"select[2] s:sum px by sym from st"; 2]
/ select[m n] is n rows from row m (past the end, the rows there are); a null, a negative, or a third number is 'type;
/ every item an atom is one row, as before, in both forms; q gives 'rank unless an item applies one of its aggregates
/ (#20 row 17: c:0.5, and c:sz 0 in selAtomCol too)
tq[`selFromCount; "select[1 2] from st";        st 1 2]
tq[`selFromCountPast; "select[4 5] from st";    st 4 5]
t[`selLimitBad;  {5#.[sel;,x;{x}]}'("select[0N] from st";"select[1 0N] from st";"select[-1 2] from st";"select[1 2 3] from st"); 4#,"'type"]
t[`selAtomRow;   (sel"select c:0.5 from st";sel"select c:abs 0.5 from st"); 2#,+(,`c)!,,0.5]
t[`selAtomAgg;   sel"select c:1+sum sz from st";  +(,`c)!,,211]
t[`selAtomCol;   sel"select c:sz 0,d:0.5 from st"; +`c`d!(,30;,0.5)]
t[`selAtomFunc;  {$[`C~@r:.[{?[st;();0b;(,`c)!,x]};,x;{x}];5#r;r]}'(0.5;(abs;0.5);(sum;`sz));(+(,`c)!,,0.5;+(,`c)!,,0.5;+(,`c)!,,210)]
/ both at once on a grouped result -- the form docs/language.html publishes:
/ group, then sort the aggregate descending, then take the first n.
t[`selSortLimitKeyed;  (value sel"select[3;>v] v:sum px by sym from st")`v; 9.0 7 5]
t[`selSortLimitKeyed2; #sel"select[2;>v] v:sum px by sym from st"; 2]
/ the bare (unquoted) form must accept the bracket too, and only for `select`
t[`qrwBracket;    qrw"select[2;>px] from st";  "sel\"select[2;>px] from st\""]
t[`qrwBracketAsg; qrw"z:select[3] from st";    "z:sel\"select[3] from st\""]
t[`qokcSpace;     qokc["select";" "]; 1]
t[`qokcBracket;   qokc["select";"["]; 1]
t[`qokcExecBrk;   qokc["exec";"["];   0]
/ an unbalanced bracket is left alone rather than eating the query
t[`qbrkUnbalanced; qbrk"[3 from st"; ("";"[3 from st")]
t[`qbrkNone;       qbrk"px from st"; ("";"px from st")]
t[`qbrkPeel;       qbrk"[2;>px] from st"; ("2;>px";"from st")]
/ the spec is semicolon-separated, unlike a select list
t[`qscut;          qscut"2;>px";  (,"2";">px")]
t[`qscutNested;    qscut"2;>f[a;b]"; (,"2";">f[a;b]")]

/ ---- amber 2.2: the filtered table is not built on paths that ignore it ----
/ `select from t` with no where clause returns the table itself rather than a
/ row-by-row copy of it, and every by-query keeps its exact previous answer.
t[`selAllIdentity;  (sel"select from st")~st; 1]
t[`selStarIdentity; (sel"select * from st")~st; 1]
t[`selWhereStill;   sel"select from st where sz>25"; st@&(st`sz)>25]
t[`selByNoWhere;    sel"select s:sum px by sym from st"; ([sym:`a`b`c] s:5.0 7 9)]
t[`selByWhere;      sel"select s:sum px by sym from st where sz>15"; ([sym:`a`b`c] s:4.0 7 9)]

/ ---- fby inside a WHERE-CLAUSE -------------------------------------------
/ docs/MISSING.md claimed this was not implemented. It always was: qidsub
/ substitutes the column names inside the aggregate pair too, so
/ "px=(max;px) fby sym" becomes (x`px)=(max;(x`px)) fby (x`sym) and `fby`
/ applies infix like any other dyadic name. These pin it down so the claim
/ cannot drift again.
ft:([]sym:`a`b`a`b`a; ex:`X`X`Y`Y`X; px:10 20 30 40 50.0; sz:1 2 3 4 5)
t[`fbyWhereMax;   (sel"select sym,px from ft where px=(max;px) fby sym")`px; 40 50.0]
t[`fbyWhereAvg;   (sel"select px from ft where px>(avg;px) fby sym")`px;     40 50.0]
t[`fbyWhereSum;   #sel"select from ft where sz<(sum;sz) fby sym";            5]
t[`fbySelectList; (sel"select sym, m:(max;px) fby sym from ft")`m;   50 40 50 40 50.0]
t[`fbyExec;       exq"exec px from ft where px=(max;px) fby sym";            40 50.0]
t[`fbyUpdate;     (upd"update f:(max;px) fby sym from ft")`f;        50 40 50 40 50.0]
/ and on SEVERAL group columns, which used to return one value per column
t[`fbyWhereCols;  (sel"select px from ft where px=(max;px) fby (sym;ex)")`px; 20 30 40 50.0]

/ ---- two keyed tables: + - * % & | over the union of the keys, as dicts (issue #19); comparison 'nyi (they crashed); a keyed table with an atom works ----
KA:(+(,`a)!,1 2 3)!4 5 6;KB:(+(,`a)!,2 3 4)!10 20 30;KE:{.[x;y;{(x?"\n")#x}]}
t[`keyedArith;  {x[KA;KB]}'(+;-;*;%;&;|);  (+(,`a)!,1 2 3 4)!/:(4 15 26 30;4 -5 -14 -30;4 50 120 30;(4;0.5;0.3;30);4 5 6 30;4 10 20 30)]
t[`keyedNyi;    {KE[x;(KA;KB)]}'(<;>;=);  3#,"'nyi"]
t[`keyedAtom;   KA+1;                                       (+(,`a)!,1 2 3)!5 6 7]
/ ---- distinct of a table: its distinct rows, first seen first, as q (issue #19) ----
t[`tableDistinct;   ?([]a:1 1 2 1;b:`x`x`y`x);               ([]a:1 2;b:`x`y)]
t[`tableDistinctNa; ?([]a:2 1 2 1;b:1.5 0n 1.5 0n);       ([]a:2 1;b:1.5 0n)]
/ (compared as text: ~ cannot tell 0.0 from -0.0, and the first row seen, 0.0, is the one kept)
t[`tableDistinctZero; `k@?([]a:0.0 -0.0 0n 0n);           `k@([]a:0.0 0n)]
t[`tableDistinctEmpty; ?0#([]a:1 2;b:`x`y);              0#([]a:1 2;b:`x`y)]
t[`tableUnion;      union[([]a:1 2);([]a:2 3)];             ([]a:1 2 3)]
/ ---- sum avg min max (and mins maxs) are q's names, so they skip nulls, as q's do (issue #14) ----
t[`qaggSum;     (sum 0N 1;sum 0n 1.0;sum 0N 0N);                      (1;1.0;0)]
t[`qaggAvg;     (avg 0N 1;avg 0n 1.0;avg 0N 0N);                      1.0 1.0 0n]
t[`qaggMinMax;  (min 0N 5;min 0n 1.0;max 0n 1.0;max 0N 0N;min 0N 0N); (5;1.0;1.0;-9223372036854775807;9223372036854775807)]
t[`qaggScans;   (mins 0n 1.0 0.5;maxs 0N 5 3);                        (0w 1.0 0.5;-9223372036854775807 5 5)]
QW:([]g:`a`b`a`b;f:1.5 0n -2.0 3.0;v:5 0N -7 0N)
t[`qaggGrouped; value sel"select mxf:max f, mnf:min f, af:avg f, sf:sum f, s:sum v, a:avg v, mn:min v, mx:max v by g from QW";+`mxf`mnf`af`sf`s`a`mn`mx!(1.5 3.0;-2.0 3.0;-0.25 3.0;-0.5 3.0;-2 0;-1.0 0n;-7 9223372036854775807;5 -9223372036854775807)]

/ Guard against a silently skipped block (see tests/harness.k): this is the
/ exact number of assertions this file is expected to record. Bump it when
/ you add or remove cases -- if it ever disagrees, some of them stopped running.
/ ---- an empty grouped select keeps each column's type (from its aggregate, as q) ----
t[`byEmptySum;  @(.(sel"select sum px by sym from ft where px>1000"))`px; `F]
t[`byEmptyLast; @(.(sel"select by sym from ft where px>1000"))`px; `F]
/ an empty temporal column has no type to keep (it's a general list): max, min and sum of it stay (), not ints (A112)
TT:([]sym:`a`b;n:1 2;d:10:00:00.000 11:00:00.000)
t[`byEmptyTemporal;  (.(sel"select mx:max d, mn:min d, s:sum d by sym from TT where n>5"))@/:`mx`mn`s; (();();())]
t[`byEmptyTemporal0; `k@(.(sel"select mx:max d, c:count d, l:last n by sym from 0#TT"))@/:`mx`c`l; "(();!0;!0)"]
/ avg, dev and wavg of it are floats, as q's (they were (), and with no by 'type)
tqa[`byEmptyTempAvg; "select a:avg d, s:dev d, w:wavg[n;d] by sym from TT where n>5"; {`k@(. x)@/:`a`s`w}; "(0#0n;0#0n;0#0n)"]
tq[`selEmptyTempAvg; "select a:avg d, s:dev d, w:wavg[n;d] from TT where n>5"; ([]a:,0n;s:,0n;w:,0n)]

/ ---- digest #39: constraints cascade, as q's (each sees the rows the earlier ones kept); #40: a leading minus ----
tq[`whereCascade;     "select from trades where sym=`a, px=max px";   ([]sym:,`a; px:,60.0; sz:,6)];
tq[`whereCascadeExec; "exec sz from trades where px>15, sz<count sz"; 2 3 4];
tq[`whereLocalAnd;    "select from trades where sym=`a, px>25";       ([]sym:`a`a; px:30 60.0; sz:3 6)];
tq[`execNegLead;      "exec -1+sz from trades";                       0 1 2 3 4 5];
tqa[`updNegLead;      "update d:-1+sz from trades"; {(+x)`d};         0 1 2 3 4 5];

/ ---- the virtual column i is the row's index in the table, not among the rows a where-clause or a group kept, as q ----
tq[`iExecWhere;   "exec i from trades where sym=`b";                  1 3];
tq[`iCascade;     "select from trades where sym=`a, i>1";             ([]sym:`a`a; px:30 60.0; sz:3 6)];
tq[`iSelWhere;    "select n:i from trades where px>25";               ([]n:2 3 4 5)];
tq[`iSelAll;      "select from trades where i<2";                     ([]sym:`a`b; px:10 20.0; sz:1 2)];
tqa[`iBy;         "select c:i by sym from trades"; {(+value x)`c};    (0 2 5;1 3;,4)];
tq[`iUpdate;      "update j:i from trades where px>35";               ([]sym:`a`b`a`b`c`a; px:10 20 30 40 50 60.0; sz:1 2 3 4 5 6; j:0N 0N 0N 3 4 5)];
tqa[`iUpdateBy;   "update j:first i by sym from trades where px>15"; {(+x)`j}; 0N 1 2 1 4 2];
tq[`iDelete;      "delete from trades where sym=`b, i=1";             ([]sym:`a`a`b`c`a; px:10 30 40 50 60.0; sz:1 3 4 5 6)];
tq[`iFby;         "select from trades where px>15, i>(count;i) fby sym"; ([]sym:`b`c`a; px:40 50 60.0; sz:4 5 6)];
t[`iFunctional;   ?[trades;,(=;`sym;,`b);0b;(,`x)!,`i];               ([]x:1 3)]
tq[`iExecByAll;   "exec by sym from trades where i>0";                `a`b`c!+`px`sz!(60 40 50.0;6 4 5)];
tq[`iEmptyExec;   "exec i from none where px>0";                      !0];
tqa[`iEmptyUpd;   "update j:i from none where px>0"; CL;              `sym`px`sz`j];
tqa[`iEmptyBy;    "select c:i by sym from none"; CL;                  `sym`c];
TI:([]i:5 6 7; n:1 2 3)
tq[`iOwnColumn;   "exec i from TI where n>1";                         6 7];
TA:([]amberi:`x`y`z; n:1 2 3)
tq[`iAmberiCol;   "select from TA";                                   TA];
tq[`iAmberiColI;  "select from TA where i>0";                         1_TA];
/ words inside "strings" and `symbols are not columns, nor the virtual i
TS:([]s:("in";"out";"ix"); n:1 2 3)
tq[`iInString;    "select from TS where s like \"i*\"";              TS 0 2];
tq[`colInString;  "select from TS where s like \"*n\"";              TS@,0];
tqa[`iInSym;      "select from trades where sym=`i"; NR;              0];
tqa[`iInSyms;     "select from trades where sym in `i`b"; NR;         2];
tqa[`iInPathSym;  "select from trades where sym=`:i"; NR;             0];
/ a by-expression over i alone is named x, as q (the hidden column has no name to give), and leaves i alone
tqa[`iByExprKey;  "select c:count sz by i mod 2 from trades"; {(cols x;value(+value x)`c)}; (`x`c;3 3)];
tq[`iByExprSum;   "exec sum i by i mod 2 from trades";                0 1!6 9];
tqa[`iUpdByExpr;  "update j:sum i by i mod 2 from trades"; {(+x)`j};  6 9 6 9 6 9];
/ a table's own columns are never dropped, whatever they are called
TQ:+(`n;`$"(i)")!(1 2 3;10 20 30)
tq[`iOwnHidden;   "select from TQ";                                   TQ];
tq[`iOwnHiddenW;  "delete from TQ where n=1";                         1_TQ];
/ a quote after an escaped backslash ends the string
tq[`bsString;     "exec (\"\\\\\";sz) from trades where sz>4";           ("\\";5 6)];
TB:([]s:("a\\";"b";"c";"d\\"); n:1 2 3 4)
tq[`bsStringI;    "select m:(\"\\\\\"=last'[s])|i=1 from TB";        ([]m:1 1 0 1)];
tq[`bsString3;    "exec (\"a\\\\\\\"b\";i) from TB where n>2";        ("a\\\"b";2 3)];
tqa[`iInPathSym2; "select from trades where sym=`:db/i"; NR;          0];
tqa[`iInPathSym3; "select from trades where sym=`:db/x-i_n.i"; NR;    0];
tqa[`colInPathSym; "select from trades where sym=`:a/sz"; NR;         0];
tqa[`iInPathSym4; "select from trades where sym=`:i:j"; NR;           0];
/ an i inside a string is not a reference to the hidden column, so the key keeps its column's name
tqa[`iByStrKey;   "select c:count n by s like \"i*\" from TS where i>0"; CL; `s`c];
/ the functional distinct drops the hidden column before it looks for equal rows
T5:([]sym:`a`b`a; n:1 2 1)
t[`iFuncDistinct;   ?[T5;,(>;`i;-1);1b;()];                         ([]sym:`a`b; n:1 2)]
/ the same answers where i is read without the hidden column: count i, and-ed constraints, a where then items
tq[`iCountWhere;  "exec count i from trades where px>25";             4];
tq[`iCountBy;     "select c:count i by sym from trades where px>15";  ([sym:`a`b`c]c:2 2 1)];
tq[`iAnded;       "select from trades where px>15, i<4";              ([]sym:`b`a`b; px:20 30 40.0; sz:2 3 4)];
tq[`iExecItems;   "exec i,sz from trades where sym=`a";               `x`sz!(0 2 5;1 3 6)];
t[`iFuncLater;    ?[trades;((>;`px;15);(<;`i;4));0b;()];              ([]sym:`b`a`b; px:20 30 40.0; sz:2 3 4)]
t[`iFuncCount;    ?[trades;,(>;`px;25);0b;(,`n)!,(count;`i)];         ([]n:,4)]
t[`iFuncUpd;      (+![trades;,(>;`px;35);0b;(,`j)!,`i])`j;            0N 0N 0N 3 4 5]

/ ---- digest #73 item 95: a bare distinct is q's distinct flag, not a column; with by it is q's distinct function ----
DT:([]sym:`a`b`a`a;n:1 2 1 3;m:5 6 5 7)
tq[`selDistinctBare;  "select distinct from DT";              ([]sym:`a`b`a;n:1 2 3;m:5 6 7)];
tq[`selDistinctWhere; "select distinct from DT where n<3";    ([]sym:`a`b;n:1 2;m:5 6)];
tq[`selDistinctCols;  "select distinct sym from DT where n>1"; ([]sym:`b`a)];
tq[`selDistinctBy;    "select distinct by sym from DT";       ((+(,`sym)!,`a`b)!+`n`m!(3 2;7 6))];
tq[`selDistinctColBy; "select distinct n,m by sym from DT";   ((+(,`sym)!,`a`b)!+`n`m!((1 3;,2);(5 5 7;,6)))];
tq[`selDistinctNamedBy; "select distinct x:n by sym from DT";  ((+(,`sym)!,`a`b)!+(,`x)!,(1 3;,2))];
tq[`selDistinctExprBy;  "select distinct x:n+1 by sym from DT"; ((+(,`sym)!,`a`b)!+(,`x)!,(2 4;,3))];

/ ---- a by key named like a column is the key's values, but the aggregates still read the column, as q ----
tq[`byNameAgg;    "select s:sum sz by sz:sz mod 2 from trades";      (+(,`sz)!,0 1)!+(,`s)!,12 9];
tq[`byNameFused;  "select s:sum sz,p:max px by sz:sz mod 2 from trades where px>15"; (+(,`sz)!,0 1)!+`s`p!(12 8;60 50.0)];
tq[`byNameOther;  "select s:sum sz by sym:sz mod 2 from trades";     (+(,`sym)!,0 1)!+(,`s)!,12 9];
tq[`byNameLast;   "select by sz:sz mod 2 from trades";                (+(,`sz)!,0 1)!+`sym`px!(`a`c;60 50.0)];
tq[`byNameExec;   "exec sum sz by sz:sz mod 2 from trades";          0 1!12 9];
tqa[`byIKey;      "select c:count sz by i from trades"; CL;          `x`c];
tqa[`byIAll;      "select by i from trades"; CL;                      `x`sym`px`sz];
tq[`byTwoSame;    "select s:sum sz by sz mod 2,sz mod 3 from trades"; (+`sz`sz1!(0 0 0 1 1 1;0 1 2 0 1 2))!+(,`s)!,6 4 2 3 1 5];
tq[`byNamedKeeps; "select s:sum sz by sz mod 2,sz:sz mod 3 from trades"; (+`sz1`sz!(0 0 0 1 1 1;0 1 2 0 1 2))!+(,`s)!,6 4 2 3 1 5];
tq[`byBareKeeps;  "select s:sum px by sz mod 2,sz from trades";      (+`sz1`sz!(0 0 0 1 1 1;2 4 6 1 3 5))!+(,`s)!,20 40 60 10 30 50.0];
/ key names: a given or bare one is numbered after an earlier given or bare one, as q; a derived one after any
/ given or bare one, or an earlier name, past the table's columns
TN:([]sym:`a`b`a`b;n:1 2 3 4;m:5 6 7 8)
tqa[`byNmSymSym;  "select sum m by sym,sym from TN"; CL;              `sym`sym1`m];
tqa[`byNmBare2;   "select sum m by n,n from TN"; CL;                  `n`n1`m];
tqa[`byNmGiven2;  "select sum m by a:n mod 2,a:n mod 3 from TN"; CL;  `a`a1`m];
tqa[`byNmDerBare; "select sum m by n mod 2,n from TN"; CL;            `n1`n`m];
tqa[`byNmDerGiven; "select sum m by n mod 2,n:n mod 3 from TN"; CL;   `n1`n`m];
tqa[`byNmDer2;    "select sum m by n mod 2,n mod 3 from TN"; CL;      `n`n1`m];
tqa[`byNmI2;      "select sum m by i,i mod 2 from TN"; CL;            `x`x1`m];
tqa[`byNmThree;   "select sum m by n mod 2,n,n:n mod 3 from TN"; CL;  `n2`n`n1`m];
TN1:([]sym:`a`b`a`b;n:1 2 3 4;m:5 6 7 8;n1:9 8 7 6)
tqa[`byNmSkipCol; "select sum m by n mod 2,n from TN1"; CL;           `n2`n`m];
tqa[`byNmKeepCol; "select by n mod 2,n from TN1"; CL;                 `n2`n`sym`m`n1];
tqa[`selNmPlace;  "select sum n,n from TN"; CL;                     `n`n1];
tqa[`updNmPlace;  "update sum n,n from TN"; {(+x)`n};                 4#10];
tv[`byFunc;       "?[trades;();(,`sz)!,(mod;`sz;2);(,`s)!,(sum;`sz)]"; (+(,`sz)!,0 1)!+(,`s)!,12 9];
tv[`byFuncTwo;    "?[trades;();`a`b!((mod;`sz;2);(mod;`sz;3));(,`s)!,(sum;`sz)]"; (+`a`b!(0 0 0 1 1 1;0 1 2 0 1 2))!+(,`s)!,6 4 2 3 1 5];

/ ---- update-by that groups no row types the column by its expression on no rows, and a value of another type
/ into a column is 'type, as q (digest #73 item 109) ----
tqa[`updByEmpty;     "update m:max sz by sym from none"; {(+x)`m};                 !0];
tqa[`updByNoRow;     "update m:max sz by sym from trades where px>100"; {(+x)`m};  6#0N];
tqa[`updByNoRowF;    "update m:avg sz by sym from trades where px>100"; {(+x)`m};  6#0n];
tqa[`updByEmptyOld;  "update sz:avg sz by sym from none"; {(+x)`sz};               0#0n];
tqe[`updByType;      "update sz:avg sz by sym from trades where px>25"];
te[`fupdByType;      "![trades;,(>;`px;25);(,`sym)!,`sym;(,`sz)!,(avg;`sz)]"];
tqa[`updBySameType;  "update sz:sum sz by sym from trades where px>25"; {(+x)`sz}; 1 2 9 4 5 9];
tqe[`updByTypeAll;   "update sz:avg sz by sym from trades where sz>0"];
tqe[`updByTypeCount; "update px:count px by sym from trades where sz>0"];
tqa[`updByUnifEmpty; "update sz:sums sz by sym from none"; {(+x)`sz};          !0];
tqa[`updByUnifNoRow; "update m:sums px by sym from trades where px>100"; {(+x)`m}; 6#,()];
TG:0#([]sym:`a`b`a;n:1 2 3;g:(1 2;3 4;5 6);st:("ab";"cd";"ef"))
tqa[`updByGenFirst;  "update st:first st by sym from TG"; {(+x)`st};             ()];
tqa[`updByGenMax;    "update g:max g by sym from TG"; {(+x)`g};                  ()];
tqa[`updByGenOther;  "update n:sums n by sym from TG"; {(+x)`n};                 !0];
TD:0#([]sym:`a`b`a;n:1 2 3;d:2026.01.01 2026.01.02 2026.01.03)
tv[`fupdByUnifTemporal; "(+![TD;();(,`sym)!,`sym;(,`n)!,(sums;`n)])`n";   !0];
t[`fupdByNoRow;      (+![trades;,(>;`px;100);(,`sym)!,`sym;(,`m)!,(max;`sz)])`m;   6#0N];

/ ---- qSQL on a keyed table runs on its rows, as q's: select (no columns, no by), update and delete keep the key ----
KT:([s:`a`b`c]n:1 2 3;m:4 5 6)
tq[`ktSelect;     "select from KT where n>1";                         ([s:`b`c]n:2 3;m:5 6)];
tq[`ktSelCols;    "select n from KT";                                 ([]n:1 2 3)];
tq[`ktSelBy;      "select sum n by b:m>4 from KT";                    ([b:0 1]n:1 5)];
tq[`ktSelLimit;   "select[2] from KT";                                ([s:`a`b]n:1 2;m:4 5)];
tq[`ktExec;       "exec n from KT where m>4";                         2 3];
tq[`ktUpdate;     "update n:10 from KT where s=`b";                   ([s:`a`b`c]n:1 10 3;m:4 5 6)];
tq[`ktUpdKey;     "update s:`z from KT where n=1";                    ([s:`z`b`c]n:1 2 3;m:4 5 6)];
tq[`ktUpdBy;      "update n:sum n by m>4 from KT";                    ([s:`a`b`c]n:1 5 5;m:4 5 6)];
tq[`ktDelete;     "delete from KT where n=2";                         ([s:`a`c]n:1 3;m:4 6)];
tq[`ktDelCol;     "delete m from KT";                                 ([s:`a`b`c]n:1 2 3)];
tq[`ktDelKey;     "delete s from KT";                                 ([]n:1 2 3;m:4 5 6)];
tv[`ktFsel;       "?[KT;,(>;`n;1);0b;()]";                            ([s:`b`c]n:2 3;m:5 6)];
tv[`ktFselCols;   "?[KT;();0b;(,`n)!,`n]";                            ([]n:1 2 3)];
te[`ktFdistinct;  "?[KT;();1b;()]"];
tv[`ktFdistCols;  "?[KT;();1b;(,`n)!,`n]";                            ([]n:1 2 3)];
tv[`ktFupd;       "![KT;();0b;(,`n)!,(*;`n;2)]";                      ([s:`a`b`c]n:2 4 6;m:4 5 6)];
K2:([a:1 1 2;b:`x`y`x]v:10 20 30)
tq[`ktTwoKeys;    "update v:v*2 from K2 where b=`x";                  ([a:1 1 2;b:`x`y`x]v:20 20 60)];

/ a : inside a `symbol, a string or a lambda is not an item's name:expr
TC:([]f:`:a`:b`:a; n:1 2 3)
tq[`colonSymExec;  "exec f=`:b from TC";                              010b];
tqa[`colonSymSel;  "select f=`:b from TC"; {(+x)@*cols x};           010b];
tq[`colonStrItem;  "select n,\"a:b\" from TC";                        ([]n:1 2 3;x:"a:b")];
tqa[`colonSymBy;   "select c:count n by f=`:b from TC"; {value(+value x)`c}; 2 1];
tq[`colonSymExBy;  "exec n by f=`:b from TC";                         01b!(1 3;,2)];
tq[`colonLambda;   "exec {x:1;x+y}[n;n] from TC";                     2 3 4];
tq[`colonNamed;    "exec a:f=`:b from TC";                            (,`a)!,010b];

/ a comma inside ( ), { } or a string does not split an item or a constraint
TM:([]n:1 2 3 4)
tq[`commaParenWh;  "select from TM where n in (1,2)";                 ([]n:1 2)];
tq[`commaParenEx;  "exec (,\"ab\";n) from TM where n>2";               (,"ab";3 4)];
tq[`commaLambda;   "exec {x,y}[n;n] from TM where n>3";               4 4];
tqa[`commaParenBy; "select c:count n by g:(n mod 2) from TM where n in (1,2,3)"; {value(+value x)`c}; 1 2];
TL:([]s:("a,b";"cd"); n:1 2)
tq[`commaStrWh;    "select from TL where s like \"a,b\"";             TL@,0];
tq[`commaStrCas;   "select from TL where s like \"a,b\", n>0";        TL@,0];

/ a backslash outside a string escapes nothing: ,\"ab" is join-scan over a string
TN:([]n:1 2 3)
tq[`bsJoinScan;    "select n,c:count[,\\\"ab\"] from TN where n>1";   ([]n:2 3;c:2 2)];
tq[`bsJoinScanEx;  "exec count[,\\\"ab\"] from TN where n>1";         2];
tq[`bsJoinScanCm;  "exec n from TN where n>count[,\\\"a/\"]  / two";  ,3];

/ a tab next to a keyword is a space; a tab inside a string stays
TT:([]sym:`a`b`a; n:1 2 1)
tq[`tabSelect;     "select\tn from TT";                               ([]n:1 2 1)];
tq[`tabWhere;      "select n from TT\twhere n>1";                     ([]n:,2)];
tq[`tabExec;       "exec\tn from TT\twhere\tsym=`a";                  1 1];
tqa[`tabBy;        "select c:count n\tby sym from TT"; {value(+value x)`c}; 2 1];
tq[`tabInStrBs;    "exec (\"a\\\\\";\"x\ty\";\"b\\\\\") from TT";      ("a\\";"x\ty";"b\\")];
tq[`tabInString;   "exec \"x\ty\" from TT";                           "x\ty"];
t[`tabSel;         .[sel;,"select\tn from TT where\tn>1";{x}];       ([]n:,2)];

/ a bracket or ; inside a "string" does not end a bare query at the prompt or in a file (qendx)
TQS:([]s:("a;b";"cd"); n:1 2)
tq[`strParen;      "select n,\")\" from TQS";                           ([]n:1 2;x:"))")];
tq[`strBrack;      "select n,\"]\" from TQS";                           ([]n:1 2;x:"]]")];
tq[`strBrace;      "select n,\"}\" from TQS";                           ([]n:1 2;x:"}}")];
tq[`strSemi;       "select from TQS where s like \"a;b\"";              1#TQS];
tq[`strEscQuote;   "exec \"a\\\")\" from TQS";                          "a\")"];
tq[`strEnd;        "(exec s from TQS where s like \"a;b\"),,\"x)\"";    ("a;b";"x)")];

/ an unparenthesised query as the source takes the where-clause after it, as q's
TNQ:([]sym:`a`b`a;n:1 2 3)
tq[`nestWhereExec; "exec n from update c0:n from TNQ where sym=`b";     1 2 3];
tq[`nestWhereSel;  "select n from update c0:n from TNQ where sym=`b";   ([]n:1 2 3)];
tq[`nestWhereUpd;  "update m:n from select from TNQ where sym=`a";      ([]sym:`a`a;n:1 3;m:1 3)];
tq[`nestWhereDel;  "delete from select from TNQ where sym=`a";          0#TNQ];
tq[`nestWhereParen; "exec n from (update c0:n from TNQ) where sym=`b";  ,2];

/ a by-key that is a function is q's 'rank on a table of any length; a global that is data is a key
tqx:{[tag;q;want]e:.[{(1b;. qrw x)};,q;{(0b;*"\n"\x)}];hpush[tag;(~e 0)&want~e 1;"FAIL want the error ",want,"  <-  ",q]}
TFK:([]n:,0)
QG:42
tqx[`byFnKey;      "select count i by sym from (select c0:n from TFK)";"'rank"];
tqx[`byFnKeyAll;   "select by sym from TFK";"'rank"];
tqx[`byFnKey2;     "select count i by sym from ([]n:1 2)";"'rank"];
tqx[`byFnKeyUpd;   "update c:sum n by sym from TFK";"'rank"];
tqx[`byFnKeyExec;  "exec count i by sym from TFK";"'rank"];
tq[`byDataGlobal;  "select count i by QG from TN";  (+(,`QG)!,,42)!+(,`x)!,,3];

/ a where whose value is an atom keeps every row (1b) or none (0b), as q: it is not a one-item mask
tq[`whAtom1b;      "select from trades where 1b";                       trades];
tq[`whAtom1bI;     "select i from trades where 1b";                     ([]x:!6)];
tq[`whAtom0b;      "select from trades where 0b";                       0#trades];
tq[`whAtomAnd;     "select from trades where 1b,0b";                    0#trades];
tq[`whAtomExec;    "exec sz from trades where 1b";                      1 2 3 4 5 6];
tq[`whAtomBy;      "exec count sz by sym from trades where 1b";         `a`b`c!3 2 1];
tq[`whAtomUpd;     "update z:sz from trades where 1b";                  +(+trades),(,`z)!,1 2 3 4 5 6];
tq[`whAtomDel;     "delete from trades where 0b";                       trades];
t[`whAtomFunc;     ?[trades;,1b;0b;()];                                 trades]
t[`whAtomFuncLater; ?[trades;((>;`px;15);1b);0b;()];                    1_trades]
t[`whAtomFuncDel;  ![trades;,0b;0b;0#`];                                trades]
/ any other atom is not a boolean: 'type, as q (it was every row, the first row, or 'domain), the error pinned
tqx[`whAtomNull;   "select from trades where 0N"; "'type"];
tqx[`whAtomFloat;  "select from trades where 1.5"; "'type"];
tqx[`whAtomSym;    "select from trades where `a"; "'type"];
tqx[`whAtomChar;   "exec sz from trades where \"a\""; "'type"];
tqx[`whAtomFNull;  "update z:0 from trades where 0n"; "'type"];
tqx[`whAtomCasFloat; "select from trades where px>15, 1.5, sz>avg sz"; "'type"];
tqx[`whAtomCasSym;   "select from trades where px>15, `a, sz>avg sz"; "'type"];
tqx[`whAtomCasChar;  "select from trades where px>15, \"a\", sz>avg sz"; "'type"];
tqx[`whAtomCasFNull; "select from trades where px>15, 0n, sz>avg sz"; "'type"];
tqe[`whAtomCascade;  "select from trades where px>15, 0N, sz>avg sz"];
tqx[`whAtomFuncNull;  "?[trades;((>;`px;15);0N);0b;()]"; "'type"];
/ a functional where over a simple column
TH:([]a:(1 2;3 4;1 1);b:1 2 3;s:("ab";"cd";"ab"))
t[`whFuncSimple;      ?[TH;,(>;`b;1);0b;()];                               1_TH]
/ like on a char column reads it as one string: an atom, so every row or none, as the text form; a temporal column
TC:([]c:"abc";n:1 2 3)
TT2:([]time:09:30:00.000+60000*0 1 6 11;n:1 2 3 4)
t[`whFuncLikeChar;    ?[TC;,(like;`c;"a*");0b;()];                          TC]
t[`whFuncLikeCharE;   ?[0#TC;,(like;`c;"");0b;()];                          0#TC]
t[`whFuncLikeCharU;   (+![TC;,(like;`c;"a*");0b;(,`n)!,0])`n;               0 0 0]
t[`whFuncTime;        ?[TT2;,(>;`time;09:35:00.000);0b;()];                 2_TT2]
/ an int atom where other than 0 or 1 is 'type, as q (#83 Q13); a list is passed on as before
tqe[`whIntAtom;      "select from trades where 2"];
tq[`whZeroOne;       "select from trades where (px>15)&sz<5";        ([]sym:`b`a`b; px:20 30 40.0; sz:2 3 4)];
tq[`whNotLeft;       "select from trades where ~sz<3";               2_trades];
/ an empty list as a where's value (no rows, a cascade that kept none, an each over none) keeps no row, as before
TE:([]n:1 2 3 4;s:("ab";"b";"ac";"d"))
tq[`whEmptySel;      "select from (0#TE) where s like \"a*\"";           0#TE];
tq[`whEmptyCas;      "select from TE where n>10,s like \"a*\",n=max n";   0#TE];
tqa[`whEmptyUpd;     "update z:1 from TE where n>10,s like \"a*\",n=max n"; {(+x)`z}; 0N 0N 0N 0N];
tq[`whEmptyDel;      "delete from TE where n>10,s like \"a*\",n=max n";   TE];
tqa[`whEmptyBy;      "select c:count n by n from TE where n>10,s like \"a*\",n=max n"; NR; 0];
tq[`whEmptyEach;     "select from TE where n>10,{x>1}'n";                 0#TE];
tq[`whEmptyCols;     "select from ([]a:();b:()) where a=1";               ([]a:();b:())];
tq[`whEmptyFunc;     "?[TE;((>;`n;10);(like;`s;\"a*\"));0b;()]";        0#TE];

/ a later constraint that reads other rows sees only the rows the earlier ones kept, as q: indexing by a column
/ (n 0, m n), in with a column on its right, and match all read other rows, though they look like n>1
TL:([]n:1 2 3 4 5 6;m:6 2 3 1 5 4)
tq[`casIndex;     "select from TL where n>1,n>n 0";                     TL 2 3 4 5];
tq[`casIndexCol;  "select from TL where n>1,m>m n";                     TL 1 4 5];
tq[`casInCol;     "select from TL where n>3,n in m";                    TL 3 4];
tq[`casMatch;     "select from TL where n>2,n~3 4 5 6";                 TL 2 3 4 5];
tq[`casInLit;     "select from TL where n>2,n in 3 4";                  TL 2 3];
tq[`casGlobalIx;  "select from TL where n>2,(10 20 30 40 50 60 70 m)>35"; TL 2 4 5];
/ and what still keeps to its row: a monadic ~, a , between literals
tq[`casMonoNot;   "select from TL where n>1,~n=4";                      TL 1 2 4 5];
tq[`casLitJoin;   "select from TL where n>1,n in (2,5)";                TL 1 4];
/ qloc keeps its answers in globals; a peach worker may not assign one, and does not keep them
t[`casPeach;      .[{peach[x;1 2 3 4]};,{#sel"from TL where n>1,n<4"};{x}];        2 2 2 2]
t[`casPeachCas;   .[{peach[x;1 2 3 4]};,{#sel"from TL where n>1,n>avg n"};{x}];     2 2 2 2]

/ a by-expression that reads other rows sees only the rows the where kept, as q: avg px is 45, not 35
tq[`byAfterWhere;     "select s:sum sz by px>avg px from trades where px>25"; (+(,`px)!,0 1)!+(,`s)!,7 11];
tq[`byAfterWhereNm;   "select s:sum sz by g:px>avg px from trades where px>25"; (+(,`g)!,0 1)!+(,`s)!,7 11];
tq[`byAfterWhereExec; "exec sum sz by px>avg px from trades where px>25";   0 1!7 11];
tq[`byAfterWhereTwo;  "select s:sum sz by sym,px>avg px from trades where sz>1"; (+`sym`px!(`a`a`b`c;0 1 0 1))!+(,`s)!,3 6 6 5];
tq[`byAfterWhereI;    "select j:i by i>avg i from trades where sz>2";       (+(,`x)!,0 1)!+(,`j)!,(2 3;4 5)];

/ the same for by-items that index or use in with a column (the key's name aside: values only)
KV:{(*. +!x;*. +. x)}
tqa[`byAfterWhereIx;   "select c:count m by n 0 from TL where n>2"; KV;  (,3;,4)];
tqa[`byAfterWhereIn;   "select c:count m by n in m from TL where n>3"; KV; (0 1;1 2)];
tqa[`byAfterWhereColIx; "select c:count n by m n from TL where n>2"; KV;  (0N 4;3 1)];
/ a by-item that starts with a digit but is not a number reads other rows too: 0+|n is read on the kept rows, as q
TU:([]n:1 2 3 4;m:4 1 3 2)
tqa[`byAfterWhereDigit; "select c:count i by 0+|n from TU where n>2"; KV; (3 4;1 1)];
tq[`byAfterWhereDigitX;  "exec n by 0+|n from TU where n>2";              3 4!(,4;,3)];
t[`byAfterWherePeach; .[{peach[x;1 2 3 4]};,{#sel"select c:count m by n mod 2 from TL where n>1"};{x}]; 2 2 2 2]

/ a : inside a time or timestamp literal is not an item's name:expr, as q (an unnamed item holding one was split there)
TT:([]time:09:30:00.000+60000*0 1 6 11;n:1 2 3 4)
TV:([]ts:2026.01.02D09:00 2026.01.02D09:30 2026.01.02D10:00 2026.01.02D10:30;n:1 2 3 4)
tqa[`tlitBy;        "select c:count n by time>09:35:00.000 from TT"; KV;       (0 1;2 2)];
tqa[`tlitByShort;   "select c:count n by time>09:35 from TT"; KV;              (0 1;2 2)];
tqa[`tlitByLeft;    "select c:count n by 09:35:00<time from TT"; KV;           (0 1;2 2)];
tqa[`tlitByWithin;  "select c:count n by time within 09:31 09:40 from TT"; KV; (0 1;2 2)];
tqa[`tlitByNamed;   "select c:count n by g:time>09:35 from TT"; KV;            (0 1;2 2)];
tqa[`tlitSel;       "select b:time>09:35, time>09:35:00.000 from TT"; {. +x};  (0 0 1 1;0 0 1 1)];
tq[`tlitExec;       "exec time>09:35:00.000 from TT";                         0 0 1 1];
tq[`tlitExecBy;     "exec count n by time>09:35 from TT";                     0 1!2 2];
tqa[`tlitUpdBy;     "update c:count n by time>09:35 from TT"; {(+x)`c};        2 2 2 2];
tqa[`tlitStampBy;   "select c:count n by ts>2026.01.02D09:30 from TV"; KV;     (0 1;2 2)];
tq[`tlitStampExec;  "exec ts>2026.01.02D09:30:00.000000000 from TV";          0 0 1 1];
/ such an item takes Amber's name, the first name in it (x if it starts with a literal), not q's x: pinned on purpose
tqa[`tlitSelName;   "select time>09:35 from TT"; CL;                          ,`time];
tqa[`tlitSelNameX;  "select 09:35<time from TT"; CL;                          ,`x];
tqa[`tlitUpdName;   "update time>09:35 from TT"; CL;                          `time`n];

hreport"AMBER qSQL MATRIX"
$[FAILN;`exit 1;]
