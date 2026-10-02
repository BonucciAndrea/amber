/ ============================================================================
/ tests/test_globals.k  -  the table of globals: 65536 names, then 'limit.
/ GNU AGPLv3 - see LICENSE and NOTICE.
/ ----------------------------------------------------------------------------
/     ./amber tests/test_globals.k       (exits non-zero on any failure)
/ ----------------------------------------------------------------------------
/ A suite of its own because it fills the table: nothing after it in the same
/ process could define a global.
/ ============================================================================
DIR:{$[#p:"/"/-1_"/"\x;p;"."]}`argv 1
LOAD:{{. x^(`c$13)}'0:x;}
LOAD DIR,"/harness.k"
/ ---- the table of globals holds 65536; past that, 'limit, not the end of the process (issue #9) ----
GE:{(x?"\n")#x}
t[`globalsMany;#{.("gv",($x),":1")}'!5000;5000]
t[`globalsFull;.[{{.("gw",($x),":1")}'!70000};,0;GE];"'limit"]
t[`globalsFullCompile;.[.:;,"{gzz+1}";GE];"'limit"]
t[`globalsFullAmend;.[{.[`gzz;();:;1]};,0;GE];"'limit"]
t[`globalsStillThere;(gv4999;gw1);1 1]

hexpect[5]
hreport"AMBER GLOBALS"
$[FAILN;`exit 1;]
