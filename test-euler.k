/ ============================================================================
/ test-euler.k  -  tests for euler.k (arbitrary-precision integers + PE toolkit).
/ run:  ./amber test-euler.k
/ Loaded LAST in the suite list because euler.k deliberately redefines xexp.
/ Values below are ground truth from an independent bignum implementation.
/ ============================================================================
\l amber.k
\l std.k
\l euler.k

r:()
/ every b* verb returns an int when the answer fits a machine int and the exact
/ decimal string when it does not, so compare against the decimal either way
s:{$[`C=@x;x;`c=@x;,x;$x]}
t:{r,:,($x;$[(s y)~s z;"ok";"FAIL got<",(`k y),"> exp<",(`k z),">"])}

/ ---- the shape of a result: a NUMBER, never the raw limb vector ----
t[`b10pow12;b10pow 12;"1000000000000"]
t[`b10pow100;b10pow 100;"10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"]
t[`fact20int;bfact 20;"2432902008176640000"]
t[`fact21str;bfact 21;"51090942171709440000"]
t[`fact20type;@bfact 20;`i]        / fits a machine int -> integer
t[`fact21type;@bfact 21;`C]        / past it -> exact decimal string
/ ---- inputs may be int, decimal string, or the internal limb form ----
t[`mixInt;badd[123456789;987654321];"1111111110"]
t[`mixStr;badd["123456789";"987654321"];"1111111110"]
t[`mixBoth;badd[123456789;"987654321"];"1111111110"]
t[`mixLimb;badd[bint 123456789;987654321];"1111111110"]
t[`chain;bmul[bfact 25;7];"108578470303316901888000000"]
t[`chain2;badd[bmul[b10pow 20;3];bfact 20];"302432902008176640000"]
/ ---- arithmetic against known values ----
t[`add;badd["123456789012345678901234567890";"98765432109876543210"];"123456789111111111011111111100"]
t[`sub;bsub["123456789012345678901234567890";"98765432109876543210"];"123456788913580246791358024680"]
t[`mul;bmul["123456789012345678901234567890";"98765432109876543210"];"12193263113702179522496570642237463801111263526900"]
t[`div;bdiv["123456789012345678901234567890";"98765432109876543210"];"1249999988"]
t[`mod;bmod["123456789012345678901234567890";"98765432109876543210"];"60185185207253086410"]
t[`sq;bsq["98765432109876543210"];"9754610579850632525677488187778997104100"]
t[`carry;badd["999999999999999999999999";1];"1000000000000000000000000"]
t[`borrow;bsub["1000000000000000000000000";1];"999999999999999999999999"]
t[`muls;bmuls["123456789012345678901234567890";999999];"123456665555556666555555666655432110"]
t[`divs;bdivs["123456789012345678901234567890";999999];"123456912469258148159382"]
t[`mods;bmods["123456789012345678901234567890";999999];"727272"]
/ ---- power / factorial / combinatorics ----
t[`exp2x10;bexp[2;10];"1024"]
t[`xexp2x10;2 xexp 10;"1024"]
t[`exp2x64;bexp[2;64];"18446744073709551616"]
t[`xexp2x64;2 xexp 64;"18446744073709551616"]
t[`exp2x100;bexp[2;100];"1267650600228229401496703205376"]
t[`xexp2x100;2 xexp 100;"1267650600228229401496703205376"]
t[`exp3x5;bexp[3;5];"243"]
t[`xexp3x5;3 xexp 5;"243"]
t[`exp7x31;bexp[7;31];"157775382034845806615042743"]
t[`xexp7x31;7 xexp 31;"157775382034845806615042743"]
t[`exp10x0;bexp[10;0];"1"]
t[`xexp10x0;10 xexp 0;"1"]
t[`fact100;bfact 100;"93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000"]
t[`comb;bcomb[100;50];"100891344545564193334812497256"]
t[`perm;bperm[52;5];"311875200"]
t[`sumList;bsum[(1;"2";3)];"6"]
t[`sumBig;bsum[("99999999999999999999";1)];"100000000000000000000"]
t[`prodList;bprod[(2;3;"4")];"24"]
t[`fib200;bfib 200;"280571172992510140037611932413038677189525"]
t[`fib100;bfib 100;"354224848179261915075"]
/ ---- digits / compare / gcd ----
t[`dsum;bdsum bfact 100;"648"]
t[`ndig;bndig bexp[2;1000];"302"]
t[`drev;bdrev "123456789012345678901";"109876543210987654321"]
t[`pal;bispal 12321;"1"]
t[`gcd;bgcd["123456789012345678901234567890";"98765432109876543210"];"90"]
t[`lcm;blcm[21;6];"42"]
t[`cmpEq;bcmp[5;"5"];0]           / compares VALUES across representations
t[`cmpGt;bcmp["9";5];1]
t[`cmpLt;bcmp[5;"9"];-1]
t[`beqMix;beq[12345;"12345"];1]
/ ---- machine-int number theory ----
t[`isqrt0;isqrt 0;"0"]
t[`isqrt1;isqrt 1;"1"]
t[`isqrt2;isqrt 2;"1"]
t[`isqrt3;isqrt 3;"1"]
t[`isqrt4;isqrt 4;"2"]
t[`isqrt8;isqrt 8;"2"]
t[`isqrt9;isqrt 9;"3"]
t[`isqrt15;isqrt 15;"3"]
t[`isqrt16;isqrt 16;"4"]
t[`isqrt99;isqrt 99;"9"]
t[`isqrt100;isqrt 100;"10"]
t[`isqrt999999;isqrt 999999;"999"]
t[`isqrt1000000000000;isqrt 1000000000000;"1000000"]
t[`isqrt1000000000000000;isqrt 1000000000000000;"31622776"]
t[`isqrt1125899906842624;isqrt 1125899906842624;"33554432"]
t[`isqrt999999999999999999;isqrt 999999999999999999;"999999999"]
t[`isprimeT;isprime 999983;1]
t[`isprimeF;isprime 999985;0]
t[`pfactor;pfactor 360;2 2 2 3 3 5]
t[`divisors;divisors 36;1 2 3 4 6 9 12 18 36]
t[`sigma;sigma 28;56]
t[`totient;totient 36;12]
t[`collatz;collatz 27;111]
t[`numrev;numrev 1230;321]

fails:r@&{"FAIL"~4#x}'r[;1]
`0:"";`0:"============== EULER / BIGINT SUITE =============="
`0:(($#r)," tests run, ",($#fails)," failures")
$[#fails;{`0:"  ",(x 0),": ",x 1}'fails;`0:"ALL EULER TESTS PASSED"];
`0:"================================================="

