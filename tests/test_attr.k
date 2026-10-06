/ ============================================================================
/ tests/test_attr.k  -  attributes, case by case against kdb+/q 4.1.
/ GNU AGPLv3 - see LICENSE and NOTICE.
/ ----------------------------------------------------------------------------
/ Every line is one q expression and the answer q.exe gave for it: an attribute, a value or an error name. The
/ same text ran through kdb+ 4.1 and Amber: `s#x `u#x `p#x `g#x and `#x on every list type, atoms, tables, dicts,
/ keyed tables, update, and what each operation keeps. The one place Amber differs on purpose: an unparted `p#
/ is 'p-fail here, where q says 'u-fail.
/ Run it from anywhere:  ./amber tests/test_attr.k
/ ============================================================================
DIR:{$[#p:"/"/-1_"/"\x;p;"."]}`argv 1
LOAD:{{. x^(`c$13)}'0:x;}
LOAD DIR,"/../amber.k"
LOAD DIR,"/../std.k"
LOAD DIR,"/../qsql.k"
LOAD DIR,"/../temporal.k"
LOAD DIR,"/harness.k"

/ The answer as the probe printed it: a symbol as `name, a number as text, anything else as `k, an error as ERR name
nf:{$[`s~@x;"`",$x;in[@x;`i`f];$x;`k x]}
ae:{[e]s:$[(@e)~`C;e;`k e];s:$[(#s)&"'"=*s;1_s;s];w:&in[s;" \n"];"ERR ",$[#w;(*w)#s;s]}
ta:{[tag;s;want]r:.[{[s]nf[. qrw s]};,s;ae];hpush[tag;(r,"")~want,"";"FAIL got<",r,"> want<",want,">  <-  ",s]}

ta[`a0;"attr `s#1 2 3";"`s"]   / set `s# int sorted
ta[`a1;"(`#`s#1 2 3)~`#1 2 3";"1"]   / data `s# int sorted
ta[`a2;"attr `s#3 1 2";"ERR s-fail"]   / set `s# int unsorted
ta[`a3;"(`#`s#3 1 2)~`#3 1 2";"ERR s-fail"]   / data `s# int unsorted
ta[`a4;"attr `s#1 1 2";"`s"]   / set `s# int dup sorted
ta[`a5;"(`#`s#1 1 2)~`#1 1 2";"1"]   / data `s# int dup sorted
ta[`a6;"attr `s#0N 1 2";"`s"]   / set `s# int null first
ta[`a7;"(`#`s#0N 1 2)~`#0N 1 2";"1"]   / data `s# int null first
ta[`a8;"attr `s#1 2 0N";"ERR s-fail"]   / set `s# int null last
ta[`a9;"(`#`s#1 2 0N)~`#1 2 0N";"ERR s-fail"]   / data `s# int null last
ta[`a10;"attr `s#0#0";"`s"]   / set `s# int empty
ta[`a11;"(`#`s#0#0)~`#0#0";"1"]   / data `s# int empty
ta[`a12;"attr `s#enlist 5";"`s"]   / set `s# int one
ta[`a13;"(`#`s#enlist 5)~`#enlist 5";"1"]   / data `s# int one
ta[`a14;"attr `s#1 2 3000000000";"`s"]   / set `s# long big
ta[`a15;"(`#`s#1 2 3000000000)~`#1 2 3000000000";"1"]   / data `s# long big
ta[`a16;"attr `s#3 2 1";"ERR s-fail"]   / set `s# int desc
ta[`a17;"(`#`s#3 2 1)~`#3 2 1";"ERR s-fail"]   / data `s# int desc
ta[`a18;"attr `s#1 1 2 2";"`s"]   / set `s# int parted
ta[`a19;"(`#`s#1 1 2 2)~`#1 1 2 2";"1"]   / data `s# int parted
ta[`a20;"attr `s#2 2 1 1";"ERR s-fail"]   / set `s# int parted desc
ta[`a21;"(`#`s#2 2 1 1)~`#2 2 1 1";"ERR s-fail"]   / data `s# int parted desc
ta[`a22;"attr `s#1 2 1";"ERR s-fail"]   / set `s# int not parted
ta[`a23;"(`#`s#1 2 1)~`#1 2 1";"ERR s-fail"]   / data `s# int not parted
ta[`a24;"attr `s#7 7 7";"`s"]   / set `s# int same
ta[`a25;"(`#`s#7 7 7)~`#7 7 7";"1"]   / data `s# int same
ta[`a26;"attr `s#-3 -1 0 2";"`s"]   / set `s# int neg
ta[`a27;"(`#`s#-3 -1 0 2)~`#-3 -1 0 2";"1"]   / data `s# int neg
ta[`a28;"attr `s#1.0 2.0 3.0";"`s"]   / set `s# float sorted
ta[`a29;"(`#`s#1.0 2.0 3.0)~`#1.0 2.0 3.0";"1"]   / data `s# float sorted
ta[`a30;"attr `s#2.0 1.0";"ERR s-fail"]   / set `s# float unsorted
ta[`a31;"(`#`s#2.0 1.0)~`#2.0 1.0";"ERR s-fail"]   / data `s# float unsorted
ta[`a32;"attr `s#0n 1.0";"`s"]   / set `s# float nan first
ta[`a33;"(`#`s#0n 1.0)~`#0n 1.0";"1"]   / data `s# float nan first
ta[`a34;"attr `s#1.0 0n";"ERR s-fail"]   / set `s# float nan last
ta[`a35;"(`#`s#1.0 0n)~`#1.0 0n";"ERR s-fail"]   / data `s# float nan last
ta[`a36;"attr `s#-0w 0 0w";"`s"]   / set `s# float inf
ta[`a37;"(`#`s#-0w 0 0w)~`#-0w 0 0w";"1"]   / data `s# float inf
ta[`a38;"attr `s#1.0 1.0";"`s"]   / set `s# float dup
ta[`a39;"(`#`s#1.0 1.0)~`#1.0 1.0";"1"]   / data `s# float dup
ta[`a40;"attr `s#-0.0 0.0";"`s"]   / set `s# float negzero
ta[`a41;"(`#`s#-0.0 0.0)~`#-0.0 0.0";"1"]   / data `s# float negzero
ta[`a42;"attr `s#`a`b`c";"`s"]   / set `s# sym sorted
ta[`a43;"(`#`s#`a`b`c)~`#`a`b`c";"1"]   / data `s# sym sorted
ta[`a44;"attr `s#`b`a";"ERR s-fail"]   / set `s# sym unsorted
ta[`a45;"(`#`s#`b`a)~`#`b`a";"ERR s-fail"]   / data `s# sym unsorted
ta[`a46;"attr `s#`a`a`b";"`s"]   / set `s# sym dup sorted
ta[`a47;"(`#`s#`a`a`b)~`#`a`a`b";"1"]   / data `s# sym dup sorted
ta[`a48;"attr `s#`b`a`b";"ERR s-fail"]   / set `s# sym not parted
ta[`a49;"(`#`s#`b`a`b)~`#`b`a`b";"ERR s-fail"]   / data `s# sym not parted
ta[`a50;"attr `s#`a`a";"`s"]   / set `s# sym same
ta[`a51;"(`#`s#`a`a)~`#`a`a";"1"]   / data `s# sym same
ta[`a52;"attr `s#``a`b";"`s"]   / set `s# sym empty-name first
ta[`a53;"(`#`s#``a`b)~`#``a`b";"1"]   / data `s# sym empty-name first
ta[`a54;"attr `s#\"abc\"";"`s"]   / set `s# char sorted
ta[`a55;"(`#`s#\"abc\")~`#\"abc\"";"1"]   / data `s# char sorted
ta[`a56;"attr `s#\"cba\"";"ERR s-fail"]   / set `s# char unsorted
ta[`a57;"(`#`s#\"cba\")~`#\"cba\"";"ERR s-fail"]   / data `s# char unsorted
ta[`a58;"attr `s#\"aab\"";"`s"]   / set `s# char dup
ta[`a59;"(`#`s#\"aab\")~`#\"aab\"";"1"]   / data `s# char dup
ta[`a60;"attr `s#011b";"`s"]   / set `s# bool sorted
ta[`a61;"(`#`s#011b)~`#011b";"1"]   / data `s# bool sorted
ta[`a62;"attr `s#101b";"ERR s-fail"]   / set `s# bool unsorted
ta[`a63;"(`#`s#101b)~`#101b";"ERR s-fail"]   / data `s# bool unsorted
ta[`a64;"attr `s#2026.01.01 2026.01.02";"`s"]   / set `s# date sorted
ta[`a65;"(`#`s#2026.01.01 2026.01.02)~`#2026.01.01 2026.01.02";"1"]   / data `s# date sorted
ta[`a66;"attr `s#2026.01.02 2026.01.01";"ERR s-fail"]   / set `s# date unsorted
ta[`a67;"(`#`s#2026.01.02 2026.01.01)~`#2026.01.02 2026.01.01";"ERR s-fail"]   / data `s# date unsorted
ta[`a68;"attr `s#2026.01.01 2026.01.01";"`s"]   / set `s# date dup
ta[`a69;"(`#`s#2026.01.01 2026.01.01)~`#2026.01.01 2026.01.01";"1"]   / data `s# date dup
ta[`a70;"attr `s#09:30:00.000 09:31:00.000";"`s"]   / set `s# time sorted
ta[`a71;"(`#`s#09:30:00.000 09:31:00.000)~`#09:30:00.000 09:31:00.000";"1"]   / data `s# time sorted
ta[`a72;"attr `s#09:31:00.000 09:30:00.000";"ERR s-fail"]   / set `s# time unsorted
ta[`a73;"(`#`s#09:31:00.000 09:30:00.000)~`#09:31:00.000 09:30:00.000";"ERR s-fail"]   / data `s# time unsorted
ta[`a74;"attr `s#(1 2;3 4)";"`s"]   / set `s# general ints
ta[`a75;"(`#`s#(1 2;3 4))~`#(1 2;3 4)";"1"]   / data `s# general ints
ta[`a76;"attr `s#(1;`a)";"`s"]   / set `s# general mixed
ta[`a77;"(`#`s#(1;`a))~`#(1;`a)";"1"]   / data `s# general mixed
ta[`a78;"attr `s#()";"`s"]   / set `s# general empty
ta[`a79;"(`#`s#())~`#()";"1"]   / data `s# general empty
ta[`a80;"attr `s#5";"ERR type"]   / set `s# atom int
ta[`a81;"(`#`s#5)~`#5";"ERR type"]   / data `s# atom int
ta[`a82;"attr `s#`a";"ERR type"]   / set `s# atom sym
ta[`a83;"(`#`s#`a)~`#`a";"ERR type"]   / data `s# atom sym
ta[`a84;"attr `s#1.5";"ERR type"]   / set `s# atom float
ta[`a85;"(`#`s#1.5)~`#1.5";"ERR type"]   / data `s# atom float
ta[`a86;"attr `u#1 2 3";"`u"]   / set `u# int sorted
ta[`a87;"(`#`u#1 2 3)~`#1 2 3";"1"]   / data `u# int sorted
ta[`a88;"attr `u#3 1 2";"`u"]   / set `u# int unsorted
ta[`a89;"(`#`u#3 1 2)~`#3 1 2";"1"]   / data `u# int unsorted
ta[`a90;"attr `u#1 1 2";"ERR u-fail"]   / set `u# int dup sorted
ta[`a91;"(`#`u#1 1 2)~`#1 1 2";"ERR u-fail"]   / data `u# int dup sorted
ta[`a92;"attr `u#0N 1 2";"`u"]   / set `u# int null first
ta[`a93;"(`#`u#0N 1 2)~`#0N 1 2";"1"]   / data `u# int null first
ta[`a94;"attr `u#1 2 0N";"`u"]   / set `u# int null last
ta[`a95;"(`#`u#1 2 0N)~`#1 2 0N";"1"]   / data `u# int null last
ta[`a96;"attr `u#0#0";"`u"]   / set `u# int empty
ta[`a97;"(`#`u#0#0)~`#0#0";"1"]   / data `u# int empty
ta[`a98;"attr `u#enlist 5";"`u"]   / set `u# int one
ta[`a99;"(`#`u#enlist 5)~`#enlist 5";"1"]   / data `u# int one
ta[`a100;"attr `u#1 2 3000000000";"`u"]   / set `u# long big
ta[`a101;"(`#`u#1 2 3000000000)~`#1 2 3000000000";"1"]   / data `u# long big
ta[`a102;"attr `u#3 2 1";"`u"]   / set `u# int desc
ta[`a103;"(`#`u#3 2 1)~`#3 2 1";"1"]   / data `u# int desc
ta[`a104;"attr `u#1 1 2 2";"ERR u-fail"]   / set `u# int parted
ta[`a105;"(`#`u#1 1 2 2)~`#1 1 2 2";"ERR u-fail"]   / data `u# int parted
ta[`a106;"attr `u#2 2 1 1";"ERR u-fail"]   / set `u# int parted desc
ta[`a107;"(`#`u#2 2 1 1)~`#2 2 1 1";"ERR u-fail"]   / data `u# int parted desc
ta[`a108;"attr `u#1 2 1";"ERR u-fail"]   / set `u# int not parted
ta[`a109;"(`#`u#1 2 1)~`#1 2 1";"ERR u-fail"]   / data `u# int not parted
ta[`a110;"attr `u#7 7 7";"ERR u-fail"]   / set `u# int same
ta[`a111;"(`#`u#7 7 7)~`#7 7 7";"ERR u-fail"]   / data `u# int same
ta[`a112;"attr `u#-3 -1 0 2";"`u"]   / set `u# int neg
ta[`a113;"(`#`u#-3 -1 0 2)~`#-3 -1 0 2";"1"]   / data `u# int neg
ta[`a114;"attr `u#1.0 2.0 3.0";"`u"]   / set `u# float sorted
ta[`a115;"(`#`u#1.0 2.0 3.0)~`#1.0 2.0 3.0";"1"]   / data `u# float sorted
ta[`a116;"attr `u#2.0 1.0";"`u"]   / set `u# float unsorted
ta[`a117;"(`#`u#2.0 1.0)~`#2.0 1.0";"1"]   / data `u# float unsorted
ta[`a118;"attr `u#0n 1.0";"`u"]   / set `u# float nan first
ta[`a119;"(`#`u#0n 1.0)~`#0n 1.0";"1"]   / data `u# float nan first
ta[`a120;"attr `u#1.0 0n";"`u"]   / set `u# float nan last
ta[`a121;"(`#`u#1.0 0n)~`#1.0 0n";"1"]   / data `u# float nan last
ta[`a122;"attr `u#-0w 0 0w";"`u"]   / set `u# float inf
ta[`a123;"(`#`u#-0w 0 0w)~`#-0w 0 0w";"1"]   / data `u# float inf
ta[`a124;"attr `u#1.0 1.0";"ERR u-fail"]   / set `u# float dup
ta[`a125;"(`#`u#1.0 1.0)~`#1.0 1.0";"ERR u-fail"]   / data `u# float dup
ta[`a126;"attr `u#-0.0 0.0";"ERR u-fail"]   / set `u# float negzero
ta[`a127;"(`#`u#-0.0 0.0)~`#-0.0 0.0";"ERR u-fail"]   / data `u# float negzero
ta[`a128;"attr `u#`a`b`c";"`u"]   / set `u# sym sorted
ta[`a129;"(`#`u#`a`b`c)~`#`a`b`c";"1"]   / data `u# sym sorted
ta[`a130;"attr `u#`b`a";"`u"]   / set `u# sym unsorted
ta[`a131;"(`#`u#`b`a)~`#`b`a";"1"]   / data `u# sym unsorted
ta[`a132;"attr `u#`a`a`b";"ERR u-fail"]   / set `u# sym dup sorted
ta[`a133;"(`#`u#`a`a`b)~`#`a`a`b";"ERR u-fail"]   / data `u# sym dup sorted
ta[`a134;"attr `u#`b`a`b";"ERR u-fail"]   / set `u# sym not parted
ta[`a135;"(`#`u#`b`a`b)~`#`b`a`b";"ERR u-fail"]   / data `u# sym not parted
ta[`a136;"attr `u#`a`a";"ERR u-fail"]   / set `u# sym same
ta[`a137;"(`#`u#`a`a)~`#`a`a";"ERR u-fail"]   / data `u# sym same
ta[`a138;"attr `u#``a`b";"`u"]   / set `u# sym empty-name first
ta[`a139;"(`#`u#``a`b)~`#``a`b";"1"]   / data `u# sym empty-name first
ta[`a140;"attr `u#\"abc\"";"`u"]   / set `u# char sorted
ta[`a141;"(`#`u#\"abc\")~`#\"abc\"";"1"]   / data `u# char sorted
ta[`a142;"attr `u#\"cba\"";"`u"]   / set `u# char unsorted
ta[`a143;"(`#`u#\"cba\")~`#\"cba\"";"1"]   / data `u# char unsorted
ta[`a144;"attr `u#\"aab\"";"ERR u-fail"]   / set `u# char dup
ta[`a145;"(`#`u#\"aab\")~`#\"aab\"";"ERR u-fail"]   / data `u# char dup
ta[`a146;"attr `u#011b";"ERR u-fail"]   / set `u# bool sorted
ta[`a147;"(`#`u#011b)~`#011b";"ERR u-fail"]   / data `u# bool sorted
ta[`a148;"attr `u#101b";"ERR u-fail"]   / set `u# bool unsorted
ta[`a149;"(`#`u#101b)~`#101b";"ERR u-fail"]   / data `u# bool unsorted
ta[`a150;"attr `u#2026.01.01 2026.01.02";"`u"]   / set `u# date sorted
ta[`a151;"(`#`u#2026.01.01 2026.01.02)~`#2026.01.01 2026.01.02";"1"]   / data `u# date sorted
ta[`a152;"attr `u#2026.01.02 2026.01.01";"`u"]   / set `u# date unsorted
ta[`a153;"(`#`u#2026.01.02 2026.01.01)~`#2026.01.02 2026.01.01";"1"]   / data `u# date unsorted
ta[`a154;"attr `u#2026.01.01 2026.01.01";"ERR u-fail"]   / set `u# date dup
ta[`a155;"(`#`u#2026.01.01 2026.01.01)~`#2026.01.01 2026.01.01";"ERR u-fail"]   / data `u# date dup
ta[`a156;"attr `u#09:30:00.000 09:31:00.000";"`u"]   / set `u# time sorted
ta[`a157;"(`#`u#09:30:00.000 09:31:00.000)~`#09:30:00.000 09:31:00.000";"1"]   / data `u# time sorted
ta[`a158;"attr `u#09:31:00.000 09:30:00.000";"`u"]   / set `u# time unsorted
ta[`a159;"(`#`u#09:31:00.000 09:30:00.000)~`#09:31:00.000 09:30:00.000";"1"]   / data `u# time unsorted
ta[`a160;"attr `u#(1 2;3 4)";"`u"]   / set `u# general ints
ta[`a161;"(`#`u#(1 2;3 4))~`#(1 2;3 4)";"1"]   / data `u# general ints
ta[`a162;"attr `u#(1;`a)";"`u"]   / set `u# general mixed
ta[`a163;"(`#`u#(1;`a))~`#(1;`a)";"1"]   / data `u# general mixed
ta[`a164;"attr `u#()";"`u"]   / set `u# general empty
ta[`a165;"(`#`u#())~`#()";"1"]   / data `u# general empty
ta[`a166;"attr `u#5";"ERR type"]   / set `u# atom int
ta[`a167;"(`#`u#5)~`#5";"ERR type"]   / data `u# atom int
ta[`a168;"attr `u#`a";"ERR type"]   / set `u# atom sym
ta[`a169;"(`#`u#`a)~`#`a";"ERR type"]   / data `u# atom sym
ta[`a170;"attr `u#1.5";"ERR type"]   / set `u# atom float
ta[`a171;"(`#`u#1.5)~`#1.5";"ERR type"]   / data `u# atom float
ta[`a172;"attr `p#1 2 3";"`p"]   / set `p# int sorted
ta[`a173;"(`#`p#1 2 3)~`#1 2 3";"1"]   / data `p# int sorted
ta[`a174;"attr `p#3 1 2";"`p"]   / set `p# int unsorted
ta[`a175;"(`#`p#3 1 2)~`#3 1 2";"1"]   / data `p# int unsorted
ta[`a176;"attr `p#1 1 2";"`p"]   / set `p# int dup sorted
ta[`a177;"(`#`p#1 1 2)~`#1 1 2";"1"]   / data `p# int dup sorted
ta[`a178;"attr `p#0N 1 2";"`p"]   / set `p# int null first
ta[`a179;"(`#`p#0N 1 2)~`#0N 1 2";"1"]   / data `p# int null first
ta[`a180;"attr `p#1 2 0N";"`p"]   / set `p# int null last
ta[`a181;"(`#`p#1 2 0N)~`#1 2 0N";"1"]   / data `p# int null last
ta[`a182;"attr `p#0#0";"`p"]   / set `p# int empty
ta[`a183;"(`#`p#0#0)~`#0#0";"1"]   / data `p# int empty
ta[`a184;"attr `p#enlist 5";"`p"]   / set `p# int one
ta[`a185;"(`#`p#enlist 5)~`#enlist 5";"1"]   / data `p# int one
ta[`a186;"attr `p#1 2 3000000000";"`p"]   / set `p# long big
ta[`a187;"(`#`p#1 2 3000000000)~`#1 2 3000000000";"1"]   / data `p# long big
ta[`a188;"attr `p#3 2 1";"`p"]   / set `p# int desc
ta[`a189;"(`#`p#3 2 1)~`#3 2 1";"1"]   / data `p# int desc
ta[`a190;"attr `p#1 1 2 2";"`p"]   / set `p# int parted
ta[`a191;"(`#`p#1 1 2 2)~`#1 1 2 2";"1"]   / data `p# int parted
ta[`a192;"attr `p#2 2 1 1";"`p"]   / set `p# int parted desc
ta[`a193;"(`#`p#2 2 1 1)~`#2 2 1 1";"1"]   / data `p# int parted desc
ta[`a194;"attr `p#1 2 1";"ERR p-fail"]   / set `p# int not parted
ta[`a195;"(`#`p#1 2 1)~`#1 2 1";"ERR p-fail"]   / data `p# int not parted
ta[`a196;"attr `p#7 7 7";"`p"]   / set `p# int same
ta[`a197;"(`#`p#7 7 7)~`#7 7 7";"1"]   / data `p# int same
ta[`a198;"attr `p#-3 -1 0 2";"`p"]   / set `p# int neg
ta[`a199;"(`#`p#-3 -1 0 2)~`#-3 -1 0 2";"1"]   / data `p# int neg
ta[`a200;"attr `p#1.0 2.0 3.0";"`p"]   / set `p# float sorted
ta[`a201;"(`#`p#1.0 2.0 3.0)~`#1.0 2.0 3.0";"1"]   / data `p# float sorted
ta[`a202;"attr `p#2.0 1.0";"`p"]   / set `p# float unsorted
ta[`a203;"(`#`p#2.0 1.0)~`#2.0 1.0";"1"]   / data `p# float unsorted
ta[`a204;"attr `p#0n 1.0";"`p"]   / set `p# float nan first
ta[`a205;"(`#`p#0n 1.0)~`#0n 1.0";"1"]   / data `p# float nan first
ta[`a206;"attr `p#1.0 0n";"`p"]   / set `p# float nan last
ta[`a207;"(`#`p#1.0 0n)~`#1.0 0n";"1"]   / data `p# float nan last
ta[`a208;"attr `p#-0w 0 0w";"`p"]   / set `p# float inf
ta[`a209;"(`#`p#-0w 0 0w)~`#-0w 0 0w";"1"]   / data `p# float inf
ta[`a210;"attr `p#1.0 1.0";"`p"]   / set `p# float dup
ta[`a211;"(`#`p#1.0 1.0)~`#1.0 1.0";"1"]   / data `p# float dup
ta[`a212;"attr `p#-0.0 0.0";"`p"]   / set `p# float negzero
ta[`a213;"(`#`p#-0.0 0.0)~`#-0.0 0.0";"1"]   / data `p# float negzero
ta[`a214;"attr `p#`a`b`c";"`p"]   / set `p# sym sorted
ta[`a215;"(`#`p#`a`b`c)~`#`a`b`c";"1"]   / data `p# sym sorted
ta[`a216;"attr `p#`b`a";"`p"]   / set `p# sym unsorted
ta[`a217;"(`#`p#`b`a)~`#`b`a";"1"]   / data `p# sym unsorted
ta[`a218;"attr `p#`a`a`b";"`p"]   / set `p# sym dup sorted
ta[`a219;"(`#`p#`a`a`b)~`#`a`a`b";"1"]   / data `p# sym dup sorted
ta[`a220;"attr `p#`b`a`b";"ERR p-fail"]   / set `p# sym not parted
ta[`a221;"(`#`p#`b`a`b)~`#`b`a`b";"ERR p-fail"]   / data `p# sym not parted
ta[`a222;"attr `p#`a`a";"`p"]   / set `p# sym same
ta[`a223;"(`#`p#`a`a)~`#`a`a";"1"]   / data `p# sym same
ta[`a224;"attr `p#``a`b";"`p"]   / set `p# sym empty-name first
ta[`a225;"(`#`p#``a`b)~`#``a`b";"1"]   / data `p# sym empty-name first
ta[`a226;"attr `p#\"abc\"";"`p"]   / set `p# char sorted
ta[`a227;"(`#`p#\"abc\")~`#\"abc\"";"1"]   / data `p# char sorted
ta[`a228;"attr `p#\"cba\"";"`p"]   / set `p# char unsorted
ta[`a229;"(`#`p#\"cba\")~`#\"cba\"";"1"]   / data `p# char unsorted
ta[`a230;"attr `p#\"aab\"";"`p"]   / set `p# char dup
ta[`a231;"(`#`p#\"aab\")~`#\"aab\"";"1"]   / data `p# char dup
ta[`a232;"attr `p#011b";"`p"]   / set `p# bool sorted
ta[`a233;"(`#`p#011b)~`#011b";"1"]   / data `p# bool sorted
ta[`a234;"attr `p#101b";"ERR p-fail"]   / set `p# bool unsorted
ta[`a235;"(`#`p#101b)~`#101b";"ERR p-fail"]   / data `p# bool unsorted
ta[`a236;"attr `p#2026.01.01 2026.01.02";"`p"]   / set `p# date sorted
ta[`a237;"(`#`p#2026.01.01 2026.01.02)~`#2026.01.01 2026.01.02";"1"]   / data `p# date sorted
ta[`a238;"attr `p#2026.01.02 2026.01.01";"`p"]   / set `p# date unsorted
ta[`a239;"(`#`p#2026.01.02 2026.01.01)~`#2026.01.02 2026.01.01";"1"]   / data `p# date unsorted
ta[`a240;"attr `p#2026.01.01 2026.01.01";"`p"]   / set `p# date dup
ta[`a241;"(`#`p#2026.01.01 2026.01.01)~`#2026.01.01 2026.01.01";"1"]   / data `p# date dup
ta[`a242;"attr `p#09:30:00.000 09:31:00.000";"`p"]   / set `p# time sorted
ta[`a243;"(`#`p#09:30:00.000 09:31:00.000)~`#09:30:00.000 09:31:00.000";"1"]   / data `p# time sorted
ta[`a244;"attr `p#09:31:00.000 09:30:00.000";"`p"]   / set `p# time unsorted
ta[`a245;"(`#`p#09:31:00.000 09:30:00.000)~`#09:31:00.000 09:30:00.000";"1"]   / data `p# time unsorted
ta[`a246;"attr `p#(1 2;3 4)";"ERR type"]   / set `p# general ints
ta[`a247;"(`#`p#(1 2;3 4))~`#(1 2;3 4)";"ERR type"]   / data `p# general ints
ta[`a248;"attr `p#(1;`a)";"ERR type"]   / set `p# general mixed
ta[`a249;"(`#`p#(1;`a))~`#(1;`a)";"ERR type"]   / data `p# general mixed
ta[`a250;"attr `p#()";"ERR type"]   / set `p# general empty
ta[`a251;"(`#`p#())~`#()";"ERR type"]   / data `p# general empty
ta[`a252;"attr `p#5";"ERR type"]   / set `p# atom int
ta[`a253;"(`#`p#5)~`#5";"ERR type"]   / data `p# atom int
ta[`a254;"attr `p#`a";"ERR type"]   / set `p# atom sym
ta[`a255;"(`#`p#`a)~`#`a";"ERR type"]   / data `p# atom sym
ta[`a256;"attr `p#1.5";"ERR type"]   / set `p# atom float
ta[`a257;"(`#`p#1.5)~`#1.5";"ERR type"]   / data `p# atom float
ta[`a258;"attr `g#1 2 3";"`g"]   / set `g# int sorted
ta[`a259;"(`#`g#1 2 3)~`#1 2 3";"1"]   / data `g# int sorted
ta[`a260;"attr `g#3 1 2";"`g"]   / set `g# int unsorted
ta[`a261;"(`#`g#3 1 2)~`#3 1 2";"1"]   / data `g# int unsorted
ta[`a262;"attr `g#1 1 2";"`g"]   / set `g# int dup sorted
ta[`a263;"(`#`g#1 1 2)~`#1 1 2";"1"]   / data `g# int dup sorted
ta[`a264;"attr `g#0N 1 2";"`g"]   / set `g# int null first
ta[`a265;"(`#`g#0N 1 2)~`#0N 1 2";"1"]   / data `g# int null first
ta[`a266;"attr `g#1 2 0N";"`g"]   / set `g# int null last
ta[`a267;"(`#`g#1 2 0N)~`#1 2 0N";"1"]   / data `g# int null last
ta[`a268;"attr `g#0#0";"`g"]   / set `g# int empty
ta[`a269;"(`#`g#0#0)~`#0#0";"1"]   / data `g# int empty
ta[`a270;"attr `g#enlist 5";"`g"]   / set `g# int one
ta[`a271;"(`#`g#enlist 5)~`#enlist 5";"1"]   / data `g# int one
ta[`a272;"attr `g#1 2 3000000000";"`g"]   / set `g# long big
ta[`a273;"(`#`g#1 2 3000000000)~`#1 2 3000000000";"1"]   / data `g# long big
ta[`a274;"attr `g#3 2 1";"`g"]   / set `g# int desc
ta[`a275;"(`#`g#3 2 1)~`#3 2 1";"1"]   / data `g# int desc
ta[`a276;"attr `g#1 1 2 2";"`g"]   / set `g# int parted
ta[`a277;"(`#`g#1 1 2 2)~`#1 1 2 2";"1"]   / data `g# int parted
ta[`a278;"attr `g#2 2 1 1";"`g"]   / set `g# int parted desc
ta[`a279;"(`#`g#2 2 1 1)~`#2 2 1 1";"1"]   / data `g# int parted desc
ta[`a280;"attr `g#1 2 1";"`g"]   / set `g# int not parted
ta[`a281;"(`#`g#1 2 1)~`#1 2 1";"1"]   / data `g# int not parted
ta[`a282;"attr `g#7 7 7";"`g"]   / set `g# int same
ta[`a283;"(`#`g#7 7 7)~`#7 7 7";"1"]   / data `g# int same
ta[`a284;"attr `g#-3 -1 0 2";"`g"]   / set `g# int neg
ta[`a285;"(`#`g#-3 -1 0 2)~`#-3 -1 0 2";"1"]   / data `g# int neg
ta[`a286;"attr `g#1.0 2.0 3.0";"`g"]   / set `g# float sorted
ta[`a287;"(`#`g#1.0 2.0 3.0)~`#1.0 2.0 3.0";"1"]   / data `g# float sorted
ta[`a288;"attr `g#2.0 1.0";"`g"]   / set `g# float unsorted
ta[`a289;"(`#`g#2.0 1.0)~`#2.0 1.0";"1"]   / data `g# float unsorted
ta[`a290;"attr `g#0n 1.0";"`g"]   / set `g# float nan first
ta[`a291;"(`#`g#0n 1.0)~`#0n 1.0";"1"]   / data `g# float nan first
ta[`a292;"attr `g#1.0 0n";"`g"]   / set `g# float nan last
ta[`a293;"(`#`g#1.0 0n)~`#1.0 0n";"1"]   / data `g# float nan last
ta[`a294;"attr `g#-0w 0 0w";"`g"]   / set `g# float inf
ta[`a295;"(`#`g#-0w 0 0w)~`#-0w 0 0w";"1"]   / data `g# float inf
ta[`a296;"attr `g#1.0 1.0";"`g"]   / set `g# float dup
ta[`a297;"(`#`g#1.0 1.0)~`#1.0 1.0";"1"]   / data `g# float dup
ta[`a298;"attr `g#-0.0 0.0";"`g"]   / set `g# float negzero
ta[`a299;"(`#`g#-0.0 0.0)~`#-0.0 0.0";"1"]   / data `g# float negzero
ta[`a300;"attr `g#`a`b`c";"`g"]   / set `g# sym sorted
ta[`a301;"(`#`g#`a`b`c)~`#`a`b`c";"1"]   / data `g# sym sorted
ta[`a302;"attr `g#`b`a";"`g"]   / set `g# sym unsorted
ta[`a303;"(`#`g#`b`a)~`#`b`a";"1"]   / data `g# sym unsorted
ta[`a304;"attr `g#`a`a`b";"`g"]   / set `g# sym dup sorted
ta[`a305;"(`#`g#`a`a`b)~`#`a`a`b";"1"]   / data `g# sym dup sorted
ta[`a306;"attr `g#`b`a`b";"`g"]   / set `g# sym not parted
ta[`a307;"(`#`g#`b`a`b)~`#`b`a`b";"1"]   / data `g# sym not parted
ta[`a308;"attr `g#`a`a";"`g"]   / set `g# sym same
ta[`a309;"(`#`g#`a`a)~`#`a`a";"1"]   / data `g# sym same
ta[`a310;"attr `g#``a`b";"`g"]   / set `g# sym empty-name first
ta[`a311;"(`#`g#``a`b)~`#``a`b";"1"]   / data `g# sym empty-name first
ta[`a312;"attr `g#\"abc\"";"`g"]   / set `g# char sorted
ta[`a313;"(`#`g#\"abc\")~`#\"abc\"";"1"]   / data `g# char sorted
ta[`a314;"attr `g#\"cba\"";"`g"]   / set `g# char unsorted
ta[`a315;"(`#`g#\"cba\")~`#\"cba\"";"1"]   / data `g# char unsorted
ta[`a316;"attr `g#\"aab\"";"`g"]   / set `g# char dup
ta[`a317;"(`#`g#\"aab\")~`#\"aab\"";"1"]   / data `g# char dup
ta[`a318;"attr `g#011b";"`g"]   / set `g# bool sorted
ta[`a319;"(`#`g#011b)~`#011b";"1"]   / data `g# bool sorted
ta[`a320;"attr `g#101b";"`g"]   / set `g# bool unsorted
ta[`a321;"(`#`g#101b)~`#101b";"1"]   / data `g# bool unsorted
ta[`a322;"attr `g#2026.01.01 2026.01.02";"`g"]   / set `g# date sorted
ta[`a323;"(`#`g#2026.01.01 2026.01.02)~`#2026.01.01 2026.01.02";"1"]   / data `g# date sorted
ta[`a324;"attr `g#2026.01.02 2026.01.01";"`g"]   / set `g# date unsorted
ta[`a325;"(`#`g#2026.01.02 2026.01.01)~`#2026.01.02 2026.01.01";"1"]   / data `g# date unsorted
ta[`a326;"attr `g#2026.01.01 2026.01.01";"`g"]   / set `g# date dup
ta[`a327;"(`#`g#2026.01.01 2026.01.01)~`#2026.01.01 2026.01.01";"1"]   / data `g# date dup
ta[`a328;"attr `g#09:30:00.000 09:31:00.000";"`g"]   / set `g# time sorted
ta[`a329;"(`#`g#09:30:00.000 09:31:00.000)~`#09:30:00.000 09:31:00.000";"1"]   / data `g# time sorted
ta[`a330;"attr `g#09:31:00.000 09:30:00.000";"`g"]   / set `g# time unsorted
ta[`a331;"(`#`g#09:31:00.000 09:30:00.000)~`#09:31:00.000 09:30:00.000";"1"]   / data `g# time unsorted
ta[`a332;"attr `g#(1 2;3 4)";"`g"]   / set `g# general ints
ta[`a333;"(`#`g#(1 2;3 4))~`#(1 2;3 4)";"1"]   / data `g# general ints
ta[`a334;"attr `g#(1;`a)";"`g"]   / set `g# general mixed
ta[`a335;"(`#`g#(1;`a))~`#(1;`a)";"1"]   / data `g# general mixed
ta[`a336;"attr `g#()";"`g"]   / set `g# general empty
ta[`a337;"(`#`g#())~`#()";"1"]   / data `g# general empty
ta[`a338;"attr `g#5";"ERR type"]   / set `g# atom int
ta[`a339;"(`#`g#5)~`#5";"ERR type"]   / data `g# atom int
ta[`a340;"attr `g#`a";"ERR type"]   / set `g# atom sym
ta[`a341;"(`#`g#`a)~`#`a";"ERR type"]   / data `g# atom sym
ta[`a342;"attr `g#1.5";"ERR type"]   / set `g# atom float
ta[`a343;"(`#`g#1.5)~`#1.5";"ERR type"]   / data `g# atom float
ta[`a344;"attr `#1 2 3";"`"]   / remove int sorted
ta[`a345;"attr `#3 1 2";"`"]   / remove int unsorted
ta[`a346;"attr `#1 1 2";"`"]   / remove int dup sorted
ta[`a347;"attr `#0N 1 2";"`"]   / remove int null first
ta[`a348;"attr `#1 2 0N";"`"]   / remove int null last
ta[`a349;"attr `#0#0";"`"]   / remove int empty
ta[`a350;"attr `#enlist 5";"`"]   / remove int one
ta[`a351;"attr `#1 2 3000000000";"`"]   / remove long big
ta[`a352;"attr `#3 2 1";"`"]   / remove int desc
ta[`a353;"attr `#1 1 2 2";"`"]   / remove int parted
ta[`a354;"attr `#2 2 1 1";"`"]   / remove int parted desc
ta[`a355;"attr `#1 2 1";"`"]   / remove int not parted
ta[`a356;"attr `#7 7 7";"`"]   / remove int same
ta[`a357;"attr `#-3 -1 0 2";"`"]   / remove int neg
ta[`a358;"attr `#1.0 2.0 3.0";"`"]   / remove float sorted
ta[`a359;"attr `#2.0 1.0";"`"]   / remove float unsorted
ta[`a360;"attr `#0n 1.0";"`"]   / remove float nan first
ta[`a361;"attr `#1.0 0n";"`"]   / remove float nan last
ta[`a362;"attr `#-0w 0 0w";"`"]   / remove float inf
ta[`a363;"attr `#1.0 1.0";"`"]   / remove float dup
ta[`a364;"attr `#-0.0 0.0";"`"]   / remove float negzero
ta[`a365;"attr `#`a`b`c";"`"]   / remove sym sorted
ta[`a366;"attr `#`b`a";"`"]   / remove sym unsorted
ta[`a367;"attr `#`a`a`b";"`"]   / remove sym dup sorted
ta[`a368;"attr `#`b`a`b";"`"]   / remove sym not parted
ta[`a369;"attr `#`a`a";"`"]   / remove sym same
ta[`a370;"attr `#``a`b";"`"]   / remove sym empty-name first
ta[`a371;"attr `#\"abc\"";"`"]   / remove char sorted
ta[`a372;"attr `#\"cba\"";"`"]   / remove char unsorted
ta[`a373;"attr `#\"aab\"";"`"]   / remove char dup
ta[`a374;"attr `#011b";"`"]   / remove bool sorted
ta[`a375;"attr `#101b";"`"]   / remove bool unsorted
ta[`a376;"attr `#2026.01.01 2026.01.02";"`"]   / remove date sorted
ta[`a377;"attr `#2026.01.02 2026.01.01";"`"]   / remove date unsorted
ta[`a378;"attr `#2026.01.01 2026.01.01";"`"]   / remove date dup
ta[`a379;"attr `#09:30:00.000 09:31:00.000";"`"]   / remove time sorted
ta[`a380;"attr `#09:31:00.000 09:30:00.000";"`"]   / remove time unsorted
ta[`a381;"attr `#(1 2;3 4)";"`"]   / remove general ints
ta[`a382;"attr `#(1;`a)";"`"]   / remove general mixed
ta[`a383;"attr `#()";"`"]   / remove general empty
ta[`a384;"attr `#5";"ERR type"]   / remove atom int
ta[`a385;"attr `#`a";"ERR type"]   / remove atom sym
ta[`a386;"attr `#1.5";"ERR type"]   / remove atom float
ta[`a387;"attr `#`s#1 2 3";"`"]   / remove s
ta[`a388;"attr `#`u#`a`b";"`"]   / remove u
ta[`a389;"attr `#1 2 3";"`"]   / remove from plain
ta[`a390;"attr `u#`s#1 2 3";"`u"]   / reset s to u
ta[`a391;"attr `s#`u#1 2 3";"`s"]   / reset u to s
ta[`a392;"attr `p#`g#1 1 2";"`p"]   / reset g to p
ta[`a393;"(`s#1 2 3)~1 2 3";"1"]   / match ignores attr
ta[`a394;"(`u#`a`b)~`a`b";"1"]   / match ignores attr u
ta[`a395;"attr `x#1 2 3";"ERR type"]   / unknown attr
ta[`a396;"attr `sorted#1 2 3";"ERR type"]   / unknown attr 2
ta[`a397;"attr 1 2 3";"`"]   / attr plain
ta[`a398;"attr 5";"`"]   / attr atom
ta[`a399;"attr `a";"`"]   / attr sym atom
ta[`a400;"attr asc 3 1 2";"`s"]   / attr asc
ta[`a401;"attr desc 1 2 3";"`"]   / attr desc
ta[`a402;"attr til 5";"`"]   / attr til
ta[`a403;"attr distinct 1 2 2";"`"]   / attr distinct
ta[`a404;"attr iasc 3 1 2";"`"]   / attr iasc
ta[`a405;"attr key group 1 2 1";"`"]   / attr group keys
ta[`a406;"attr asc `b`a";"`s"]   / attr asc syms
ta[`a407;"attr asc 2.0 1.0";"`s"]   / attr asc floats
ta[`a408;"attr asc \"ba\"";"`s"]   / attr asc chars
ta[`a409;"attr \"abc\"";"`"]   / attr string
ta[`a410;"attr `a`b!1 2";"`"]   / attr dict
ta[`a411;"attr ([]a:1 2)";"`"]   / attr table
ta[`a412;"attr (`s#1 2 3),4";"`"]   / keep s append bigger
ta[`a413;"attr (`s#1 2 3),0";"`"]   / keep s append smaller
ta[`a414;"attr (`s#1 2 3),4 5";"`"]   / keep s append list
ta[`a415;"attr 2#`s#1 2 3";"`"]   / keep s take
ta[`a416;"attr 1_`s#1 2 3";"`"]   / keep s drop
ta[`a417;"attr -1_`s#1 2 3";"`"]   / keep s drop end
ta[`a418;"attr reverse `s#1 2 3";"`"]   / keep s reverse
ta[`a419;"attr 1+`s#1 2 3";"`"]   / keep s add
ta[`a420;"attr neg `s#1 2 3";"`"]   / keep s neg
ta[`a421;"attr (`s#1 2 3) 0 1";"`"]   / keep s index
ta[`a422;"attr (`s#1 2 3) where 101b";"`"]   / keep s where
ta[`a423;"attr @[`s#1 2 3;0;:;0]";"`"]   / keep s amend
ta[`a424;"attr distinct `s#1 1 2";"`s"]   / keep s distinct
ta[`a425;"attr asc `s#1 2 3";"`s"]   / keep s asc
ta[`a426;"attr raze (`s#1 2;`s#3 4)";"`"]   / keep s raze
ta[`a427;"attr string `s#`a`b";"`"]   / keep s string
ta[`a428;"attr (`u#1 2 3),4";"`"]   / keep u append
ta[`a429;"attr (`u#1 2 3),1";"`"]   / keep u append dup
ta[`a430;"attr 2#`u#1 2 3";"`"]   / keep u take
ta[`a431;"attr reverse `u#1 2 3";"`"]   / keep u reverse
ta[`a432;"attr distinct `u#1 2 3";"`u"]   / keep u distinct
ta[`a433;"attr (`p#1 1 2),2";"`"]   / keep p append
ta[`a434;"attr 2#`p#1 1 2";"`"]   / keep p take
ta[`a435;"attr (`g#1 2 1),3";"`"]   / keep g append
ta[`a436;"attr 2#`g#1 2 1";"`"]   / keep g take
ta[`a437;"attr reverse `g#1 2 1";"`"]   / keep g reverse
ta[`a438;"attr (`g#1 2 1) 0 2";"`"]   / keep g index
ta[`a439;"attr (`s#([]a:1 2 3;b:3 2 1))`a";"`p"]   / table s
ta[`a440;"attr (`s#([]a:1 2 3;b:3 2 1))`b";"`"]   / table s col b
ta[`a441;"attr (`s#([]a:3 1 2;b:1 2 3))`a";"ERR s-fail"]   / table s unsorted
ta[`a442;"attr (`u#([]a:1 2;b:2 2))`a";"ERR type"]   / table u
ta[`a443;"attr (`u#([]a:1 1;b:2 2))`a";"ERR type"]   / table u dup
ta[`a444;"attr (`p#([]a:1 1 2;b:1 2 3))`a";"ERR type"]   / table p
ta[`a445;"attr (`g#([]a:2 1 2;b:1 2 3))`a";"ERR type"]   / table g
ta[`a446;"count `s#([]a:1 2 3;b:3 2 1)";"3"]   / table s keeps rows
ta[`a447;"attr (`s#([]a:`a`b`c))`a";"`s"]   / table s sym
ta[`a448;"(`s#([]a:1 2 3;b:3 2 1))~([]a:1 2 3;b:3 2 1)";"1"]   / table s data
ta[`a449;"attr `s#([]a:1 2 3)";"`s"]   / attr of table after s
ta[`a450;"attr `s#`a`b!1 2";"`s"]   / dict s
ta[`a451;"attr key `s#`a`b!1 2";"`s"]   / dict s keys
ta[`a452;"attr `s#`b`a!1 2";"ERR s-fail"]   / dict s unsorted
ta[`a453;"attr value `s#`a`b!1 2";"`"]   / dict s value
ta[`a454;"(`s#`a`b!1 2)`a";"1"]   / dict s lookup
ta[`a455;"attr key `u#`a`b!1 2";"ERR type"]   / dict u
ta[`a456;"attr key `u#`a`a!1 2";"ERR type"]   / dict u dup
ta[`a457;"(`s#1 3 5!`a`b`c) 4";"`b"]   / dict s step
ta[`a458;"(1 3 5!`a`b`c) 4";"`"]   / dict plain step
ta[`a459;"attr (key `s#([a:1 2]b:3 4))`a";"`s"]   / keyed s
ta[`a460;"attr (key `u#([a:1 2]b:3 4))`a";"ERR type"]   / keyed u
ta[`a461;"attr (key `s#([a:2 1]b:3 4))`a";"ERR s-fail"]   / keyed s unsorted
ta[`a462;"attr exec a from update `g#a from ([]a:2 1 2)";"`g"]   / update g
ta[`a463;"attr exec a from update `s#a from ([]a:1 2 3)";"`s"]   / update s
ta[`a464;"attr exec a from update `s#a from ([]a:3 1 2)";"ERR s-fail"]   / update s fail
ta[`a465;"attr exec a from update `p#a from ([]a:1 1 2)";"`p"]   / update p
ta[`a466;"(value meta `s#([]x:1 2 3))`a";",`s"]   / meta a
ta[`a467;"attr exec a from `a xasc ([]a:3 1 2)";"`s"]   / xasc sets s
ta[`a468;"attr exec b from `a`b xasc ([]a:1 1 2;b:2 1 3)";"`"]   / xasc 2 cols
ta[`a469;"attr (`s#([]a:1.0 2.0))`a";"`s"]   / table s float
ta[`a470;"attr (`s#([]a:1 1 2))`a";"`s"]   / table s dup ints
ta[`a471;"attr (`s#([]a:2026.01.01 2026.01.02))`a";"`s"]   / table s date
ta[`a472;"attr (`s#([]a:\"ab\"))`a";"`s"]   / table s char
ta[`a473;"attr (`s#([]a:1 2 3000000000))`a";"`s"]   / table s long
ta[`a474;"attr (`s#([]a:1 1 2;b:2 1 3))`a";"ERR s-fail"]   / table s ties
ta[`a475;"attr (`s#([]a:011b))`a";"`s"]   / table s bool
ta[`a476;"attr (`s#([]a:09:30:00.000 09:31:00.000))`a";"`s"]   / table s time
ta[`a477;"attr `#`s#([]a:1 2)";"`"]   / table remove
ta[`a478;"attr (`#`s#([]a:1 2))`a";"`s"]   / table remove col
ta[`a479;"attr `s#([]a:0#0)";"`s"]   / table s empty
ta[`a480;"attr `s#(3 4;1 2)";"ERR s-fail"]   / general s unsorted
ta[`a481;"attr `s#(`b;1)";"ERR s-fail"]   / general s unsorted 2
ta[`a482;"attr `u#(1 2;1 2)";"ERR u-fail"]   / general u dup
ta[`a483;"attr `g#(1 2;1 2)";"`g"]   / general g
ta[`a484;"(`s#1 3 5!`a`b`c) 0";"`"]   / dict s below
ta[`a485;"(`s#1 3 5!`a`b`c) 0 2 6";"``a`c"]   / dict s vec
ta[`a486;"(`s#1 3 5!10 20 30) 4";"20"]   / dict s ints
ta[`a487;"(`s#1 3 5!10 20 30) 3";"20"]   / dict s exact
ta[`a488;"(`s#`a`c!1 2) `b";"1"]   / dict s sym step
ta[`a489;"attr `#`s#`a`b!1 2";"`"]   / dict remove
ta[`a490;"attr `p#`a`b!1 2";"ERR type"]   / dict p
ta[`a491;"attr `g#`a`b!1 2";"ERR type"]   / dict g
ta[`a492;"(`s#1.0 2.0!`a`b) 1.5";"`a"]   / dict s float keys
ta[`a493;"attr exec a from update `u#a from ([]a:1 2)";"`u"]   / update u
ta[`a494;"attr exec a from update `u#a from ([]a:1 1)";"ERR u-fail"]   / update u fail
ta[`a495;"attr `a xasc ([]a:3 1 2)";"`"]   / xasc table attr
ta[`a496;"attr (`s#`a`b!1 2),`c!3";"ERR type"]   / s then append dict
ta[`a497;"attr (select a from ([]a:`s#1 2 3))`a";"`s"]   / select keeps
ta[`a498;"attr (select a from ([]a:`s#1 2 3) where a>1)`a";"`"]   / select where keeps
ta[`a499;"attr exec a from ([]a:`s#1 2 3)";"`s"]   / exec keeps
ta[`a500;"attr ([]a:`s#1 2 3)`a";"`s"]   / col of table keeps
ta[`a501;"attr (key `a xkey ([]a:1 2;b:3 4))`a";"`"]   / xkey sets
ta[`a502;"attr asc 101b";"`s"]   / asc sets bool
ta[`a503;"(`s#1 2 3 4)?3";"2"]   / find s
ta[`a504;"3 in `s#1 2 3";"1"]   / in s
ta[`a505;"(`u#`a`b`c)?`c";"2"]   / find u
ta[`a506;"(`g#1 2 1)?1";"0"]   / find g
ta[`a507;"(`p#1 1 2)?2";"2"]   / find p
ta[`a508;"attr asc (1 2;3 4)";"`s"]   / asc general list
ta[`a509;"attr asc ()";"`s"]   / asc empty general list
ta[`a510;"attr asc enlist 1 2";"`s"]   / asc one-item general list
ta[`a511;"attr asc (\"ba\";\"a\";\"ab\")";"`s"]   / asc strings
ta[`a512;"r~`s#r:asc (\"ba\";\"a\";\"ab\")";"1"]   / asc strings: `s# agrees
ta[`a513;"{asc x;attr x}()";"`"]   / asc () leaves () alone
ta[`a514;"attr key group `s#1 1 2";"`s"]   / group keys of s
ta[`a515;"attr key group `u#1 2 3";"`u"]   / group keys of u
ta[`a516;"attr key group `g#2 1 2";"`u"]   / group keys of g
ta[`a517;"attr key group `p#2 2 1";"`"]   / group keys of p
ta[`a518;"attr key group `s#`a`a`b";"`s"]   / group keys of s syms
ta[`a519;"attr key group `s#2001.01.01 2001.01.01 2001.01.02";"`s"]   / group keys of s dates
ta[`a520;"(key group `s#1 1 2)~1 2";"1"]   / group keys of s data
ta[`a521;"attr value group `s#1 1 2";"`"]   / group values of s
ta[`a522;"attr key group `s#1 2!4 3";"`"]   / group of s dict
ta[`a572;"attr key group `a`b`c!`s#1 1 2";"`s"]   / group of dict, s values
ta[`a573;"attr key group `a`b`c!`g#2 1 2";"`u"]   / group of dict, g values

hexpect[525]
hreport"AMBER ATTRIBUTES vs q"
$[FAILN;`exit 1;]
