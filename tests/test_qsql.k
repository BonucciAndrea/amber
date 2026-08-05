\l amber.k
\l std.k
\l qsql.k
\l tests/harness.k
/ ============================================================================
/ tests/test_qsql.k  -  qSQL query matrix.
/ GNU AGPLv3 - see LICENSE and NOTICE.
/ ----------------------------------------------------------------------------
/ Covers the full clause lattice of  select / exec / update / delete:
/   with and without  where , with and without  by , single vs multiple by
/   keys, aggregations over empty / single-row / heavily-duplicated tables,
/   and the bare-qSQL rewriter (qrw) the REPL puts every input line through.
/ Every malformed query is asserted to raise a clean runtime error rather
/ than crash -- see te[] in tests/harness.k.
/ ============================================================================

trades:([]sym:`a`b`a`b`c`a; px:10 20 30 40 50 60.0; sz:1 2 3 4 5 6)
one:([]sym:,`a; px:,1.0; sz:,1)
none:0#trades
dup:([]sym:1000#`a`b; px:1000#1.0 2.0; sz:1000#1 2)

/ ============================ 1. select: clause lattice =====================
tv[`selAll;"#sel\"select from trades\"";6];
tv[`selStar;"#sel\"select * from trades\"";6];
tv[`selCols;"cols sel\"select sym,px from trades\"";`sym`px];
tv[`selWhere;"#sel\"select from trades where px>25\"";4];
tv[`selWhereCols;"cols sel\"select px from trades where px>25\"";,`px];
tv[`selBy;"#sel\"select by sym from trades\"";3];
tv[`selByAgg;"(value sel\"select tot:sum px by sym from trades\")`tot";100 60 50.0];
tv[`selByWhere;"#sel\"select tot:sum px by sym from trades where px>25\"";3];
tv[`selAgg;"(sel\"select tot:sum px from trades\")`tot";,210.0];
tv[`selTwoAgg;"cols sel\"select a:sum px, b:max sz from trades\"";`a`b];
tv[`selExpr;"(sel\"select v:sum px*sz from trades\")`v";,+/(10 20 30 40 50 60.0)*1 2 3 4 5 6];
tv[`selRename;"cols sel\"select p:px from trades\"";,`p];
tv[`selWhereAnd;"#sel\"select from trades where px>15, sz<5\"";3];

/ ---- multiple by keys ------------------------------------------------------
mt:([]a:`x`x`y`y`x; b:1 2 1 2 1; v:10 20 30 40 50)
tv[`byTwoKeys;"#sel\"select s:sum v by a,b from mt\"";4];
tv[`byTwoKeyCols;"cols value sel\"select s:sum v by a,b from mt\"";,`s];
tv[`byTwoKeyVals;"asc(value sel\"select s:sum v by a,b from mt\")`s";asc 60 20 30 40];
tv[`byKeyIsKeyed;"iskeyed sel\"select s:sum v by a from mt\"";1b];

/ ======================== 2. empty / single-row tables =======================
tv[`emptySelect;"#sel\"select from none\"";0];
tv[`emptySelectCols;"cols sel\"select from none\"";`sym`px`sz];
tv[`emptyWhere;"#sel\"select from none where px>0\"";0];
tv[`emptyBy;"#sel\"select t:sum px by sym from none\"";0];
tv[`emptyAgg;"#sel\"select t:sum px from none\"";1];
tv[`oneSelect;"#sel\"select from one\"";1];
tv[`oneWhereHit;"#sel\"select from one where px>0\"";1];
tv[`oneWhereMiss;"#sel\"select from one where px>99\"";0];
tv[`oneBy;"#sel\"select t:sum px by sym from one\"";1];
tv[`whereNoRows;"#sel\"select from trades where px>1000\"";0];
tv[`whereAllRows;"#sel\"select from trades where px>0\"";6];

/ ==================== 3. heavily duplicated key sets ========================
tv[`dupBy;"#sel\"select t:sum sz by sym from dup\"";2];
tv[`dupByTotal;"+/(value sel\"select t:sum sz by sym from dup\")`t";+/dup`sz];
tv[`dupCount;"+/(value sel\"select c:#:px by sym from dup\")`c";1000];
tv[`dupWhereBy;"#sel\"select t:sum sz by sym from dup where sz>1\"";1];

/ ============================ 4. exec ========================================
tv[`execCol;"exq\"exec px from trades\"";10 20 30 40 50 60.0];
tv[`execAgg;"exq\"exec sum px from trades\"";210.0];
tv[`execWhere;"exq\"exec px from trades where px>35\"";40 50 60.0];
tv[`execEmpty;"#exq\"exec px from none\"";0];

/ ============================ 5. update ======================================
tv[`updNew;"cols upd\"update d:px*2 from trades\"";`sym`px`sz`d];
tv[`updVals;"(upd\"update d:px*2 from trades\")`d";20 40 60 80 100 120.0];
tv[`updReplace;"(upd\"update px:px+1 from trades\")`px";11 21 31 41 51 61.0];
tv[`updWhere;"(upd\"update px:0.0 from trades where px>35\")`px";10 20 30 0 0 0.0];
tv[`updBy;"#upd\"update t:sum px by sym from trades\"";6];
tv[`updEmpty;"#upd\"update d:px*2 from none\"";0];

/ ============================ 6. delete ======================================
tv[`delRows;"#del\"delete from trades where px>35\"";3];
tv[`delNone;"#del\"delete from trades where px>1000\"";6];
tv[`delAll;"#del\"delete from trades where px>0\"";0];
tv[`delCol;"cols del\"delete sz from trades\"";`sym`px];
tv[`delColsTwo;"cols del\"delete px,sz from trades\"";,`sym];

/ ==================== 7. functional form (qexec) ============================
tv[`qexecPlain;"#qexec[trades;();();()]";6];
tv[`qexecWhere;"#qexec[trades;trades[`px]>25;();()]";4];
tv[`qexecBy;"#qexec[trades;();`sym;(,`t)!,{sum x`px}]";3];
tv[`qexecAgg;"(qexec[trades;();();(,`t)!,{sum x`px}])`t";,210.0];
tv[`qwhereMask;"#qwhere[trades;trades[`px]>25]";4];
tv[`qselectDict;"cols qselect[trades;(,`t)!,{sum x`px}]";,`t];

/ ==================== 8. bare-qSQL rewriter (qrw) ===========================
tv[`qrwSelect;"qrw\"select from trades\"";"sel\"select from trades\""];
tv[`qrwPassThru;"qrw\"1+1\"";"1+1"];
tv[`qrwNoFrom;    "qrw\"select 1\"";                 "select 1"];   / no " from " -> left alone
tv[`qrwInString;"qrw\"\\\"select from t\\\"\"";"\"select from t\""];
tv[`qrwUpdate;"4#qrw\"update a:1 from t\"";"upd\""];
tv[`qrwDelete;"4#qrw\"delete from t\"";"del\""];
tv[`qrwExec;"4#qrw\"exec a from t\"";"exq\""];
tk[`qrwRoundTrip; ". qrw\"select from trades\""];

/ ============ 9. malformed queries: clean error, never a crash ==============
te[`noFrom;         "sel\"select\""];
te[`fromNothing;    "sel\"select from\""];
te[`byNoFrom;       "sel\"select by sym\""];
te[`unknownTable;   "sel\"select from no_such_table\""];
te[`unknownCol;     "sel\"select from trades where nosuchcol>1\""];
/ KNOWN LENIENCY (see docs/MISSING.md): an unknown by-key does not raise --
/ Amber groups by nulls instead of rejecting the query the way kdb+ does.
tk[`unknownByKey;   "sel\"select t:sum px by nosuchkey from trades\""];
te[`whereNoPred;    "sel\"select from trades where\""];
te[`emptyString;    "sel\"\""];
te[`danglingComma;  "sel\"select px, from trades\""];   / trailing comma -> clean error
te[`unbalancedBrk;  "sel\"select sum[px from trades\""];
te[`execNoFrom;     "exq\"exec px\""];
te[`updNoFrom;      "upd\"update a:1\""];
te[`delNoFrom;      "del\"delete a\""];
te[`badAggType;     "sel\"select t:sum sym from trades\""];
tk[`byOnly;         "sel\"select by sym from trades\""];
tk[`starWithWhere;  "sel\"select * from trades where sz>2\""];
tk[`nestedAgg;      "sel\"select t:sum px*sz by sym from trades\""];

hreport"AMBER qSQL MATRIX"
