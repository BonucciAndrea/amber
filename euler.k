/ ============================================================================
/ big.k  --  arbitrary-precision integers + a Project-Euler toolkit for Amber.
/ Load from the REPL:   \l /full/path/to/big.k
/ A bignum is a little-endian vector of base-1,000,000 limbs (unsigned).
/ Base 10^6 keeps  limb*limb + carry  below 2^53, so all carry math is exact,
/ and decimal I/O is trivial (each limb is exactly 6 decimal digits).
/ Bignums are NON-NEGATIVE; bsub[x;y] assumes x>=y. Primes come from `pri.
/ Functions on bignums start with 'b'; the rest work on machine ints (<2^53).
/ ============================================================================

B:1000000                                          / limb base = 10^6

/ ---- core -----------------------------------------------------------------
bnorm:{$[|/0<x;(1+|/&0<x)#x;,0]}                    / strip high zero limbs
prop:{{(B!x)+0,-1_ _x%B}/x}                         / carry-propagate (converge)
sprop:{{(x+B*x<0)-0,-1_x<0}/x}                      / borrow-propagate (converge)
bint:{$[x<B;,x;(B!x),bint[_x%B]]}                   / machine int >=0 -> bignum

/ ---- decimal I/O ----------------------------------------------------------
p6:{(-6)#"000000",$x}                               / one limb -> 6 padded digits
bstr:{r:|bnorm[x];,/(,$*r),p6'1_r}                  / bignum -> decimal string
bshow:{bstr[x]}
bdig:{(bstr[x])-"0"}                                / bignum -> big-endian digits
bfromdig:{(,0){badd[bmuls[x;10];,y]}/x}             / digit list -> bignum
bfromstr:{bfromdig[x-"0"]}                          / decimal string -> bignum

/ ---- compare --------------------------------------------------------------
bcmp:{$[(#x)>#y;1;(#x)<#y;-1;[d:x-y;$[~|/~d=0;0;$[0<d[|/&~d=0];1;-1]]]]}
beq:{x~y}
blt:{-1=bcmp[x;y]}
bgt:{1=bcmp[x;y]}
ble:{1>bcmp[x;y]}
bge:{-1<bcmp[x;y]}
bmin:{$[bgt[x;y];y;x]}
bmax:{$[blt[x;y];y;x]}
bzero:{x~,0}
bone:{x~,1}

/ ---- add / subtract / multiply --------------------------------------------
badd:{n:1+(#x)|#y;bnorm[prop[(x,(n-#x)#0)+y,(n-#y)#0]]}
bsub:{bnorm[sprop[x-y,((#x)-#y)#0]]}                / needs x>=y
bmuls:{bnorm[prop[(x*y),0]]}                        / bignum * small int (y<B)
bmul:{L:1+(#x)+#y;bnorm[prop[+/{[i;a;b;L]t:(i#0),a[i]*b;t,(L-#t)#0}[;x;y;L]'!#x]]}
bsq:{bmul[x;x]}

/ ---- divide ---------------------------------------------------------------
bdivmods:{[a;d]r:(();0){[d;s;l]c:l+B*s[1];(s[0],_c%d;d!c)}[d]/|a;(bnorm[|r[0]];r[1])}
bdivs:{(bdivmods[x;y])0}                            / bignum div small int
bmods:{(bdivmods[x;y])1}                            / bignum mod small int -> int
bshl1:{bnorm[0,x]}                                  / * B (shift up one limb)
qf:{[b;r]*{1<x[1]-x[0]}{[b;r;s]m:_(s[0]+s[1])%2;$[bge[r;bmuls[b;m]];(m;s[1]);(s[0];m)]}[b;r]/0,B}
bdivmod:{[a;b]s:(();,0){[b;s;l]r:badd[bshl1[s[1]];,l];q:qf[b;r];(s[0],q;bsub[r;bmuls[b;q]])}[b]/|a;(bnorm[|s[0]];s[1])}
bdiv:{(bdivmod[x;y])0}                              / bignum div bignum
bmod:{(bdivmod[x;y])1}                              / bignum mod bignum

/ ---- power / factorial / sums / combinatorics -----------------------------
bexp:{[b;e]$[e=0;,1;[h:bexp[b;_e%2];h:bmul[h;h];$[2!e;bmul[b;h];h]]]}
bpow:{bexp[bint[x];y]}                              / int base ^ int exp
b10pow:{bexp[bint[10];x]}                           / 10 ^ x
bfact:{(,1)bmul/bint'1+!x}                          / x!
bsum:{(,0)badd/x}                                   / sum of a list of bignums
bprod:{(,1)bmul/x}                                  / product of a list of bignums
bcomb:{bdiv[bfact[x];bmul[bfact[y];bfact[x-y]]]}    / n choose k
bperm:{bdiv[bfact[x];bfact[x-y]]}                   / n permute k

/ ---- fibonacci ------------------------------------------------------------
bfib:{[n]*(n{(x[1];badd[x[0];x[1]])}/(,0;,1))}      / n-th Fibonacci (F0=0)
bfibl:{[n]{*x}'(n{(x[1];badd[x[0];x[1]])}\(,0;,1))} / F0..Fn as a list

/ ---- big digit tools & big gcd/lcm ----------------------------------------
bdsum:{+/bdig[x]}                                   / digit sum
bndig:{#bstr[x]}                                    / number of digits
bdrev:{bfromdig[|bdig[x]]}                          / digits reversed -> bignum
bispal:{p~|p:bdig[x]}                               / palindrome?
bgcd:{$[bzero[y];x;bgcd[y;bmod[x;y]]]}
blcm:{bdiv[bmul[x;y];bgcd[x;y]]}

/ ---- machine-int number theory (built on `pri) ----------------------------
isqrt:{f:_%x;f:f-x<f*f;f+~x<(f+1)*f+1}              / exact floor(sqrt x)
primes:{`pri x}                                     / primes below x
isprime:{$[x<2;0;[m:1+isqrt[x];p:`pri m;$[0=#p;1;~|/0=p!'x]]]}
spf:{m:1+isqrt[x];p:`pri m;c:p@&0=p!'x;$[#c;*c;x]}  / smallest prime factor
pfactor:{$[x<2;();[d:spf[x];d,pfactor[_x%d]]]}      / prime factors w/ multiplicity
pfe:{c:pfactor[x];(?c)!{+/y=x}[c]'?c}               / prime -> exponent dict
divisors:{s:1+!isqrt[x];s:s@&0=s!'x;d:?s,_x%s;d@<d} / all divisors, ascending
ndivisors:{#divisors[x]}                            / number of divisors
sigma:{+/divisors[x]}                               / sum of divisors
aliquot:{(sigma[x])-x}                              / sum of PROPER divisors
totient:{x{x-_x%y}/?pfactor[x]}                     / Euler phi
gcd:{$[y=0;x;gcd[y;y!x]]}
lcm:{y*_x%gcd[x;y]}

/ ---- machine-int digit tools ----------------------------------------------
digits:{10\x}                                       / decimal digits (big-endian)
digitsum:{+/10\x}                                   / digit sum
numrev:{10/|10\x}                                   / integer, digits reversed
ispalin:{p~|p:10\x}                                 / palindrome?

/ ---- xexp override: whole-number power ------------------------------------
/ Loading this library REDEFINES the built-in float xexp so that, for an
/ integer base and a non-negative integer exponent, x xexp y returns the FULL
/ exact number (as a decimal string) instead of a float. Fractional/negative
/ exponents fall back to the float form. Restart the REPL to get plain xexp.
xexp:{[x;y]$[(x=_x)&(y=_y)&~y<0;bstr[bpow[x;y]];`exp y*`ln x]}

/ ---- assorted PE helpers --------------------------------------------------
collatz:{-1+#{x>1}{$[2!x;1+3*x;_x%2]}\x}            / Collatz chain length
tri:{_(x*x+1)%2}                                    / x-th triangular number
pent:{_(x*-1+3*x)%2}                                / x-th pentagonal number
hexn:{x*-1+2*x}                                     / x-th hexagonal number
