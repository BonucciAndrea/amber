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
/ KNOWN LENIENCY (see docs/MISSING.md): an unknown by-key does not raise --
/ Amber groups by nulls instead of rejecting the query the way kdb+ does.
tqk[`unknownByKey;  "select t:sum px by nosuchkey from trades"];

/ Guard against a silently skipped block (see tests/harness.k): this is the
/ exact number of assertions this file is expected to record. Bump it when
/ you add or remove cases -- if it ever disagrees, some of them stopped running.
hexpect[93]
hreport"AMBER qSQL MATRIX"
$[FAILN;`exit 1;]
