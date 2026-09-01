/ ============================================================================
/ euler.k  --  arbitrary-precision integers + a Project-Euler toolkit for Amber.
/ Load from the REPL:   \l euler.k
/ 
/ WHAT YOU PASS IN AND WHAT YOU GET BACK
/ Every b* verb takes an ordinary integer, a decimal STRING, or the internal
/ limb form -- mix them freely -- and hands back an ordinary integer when the
/ answer fits a machine int (below 10^18), otherwise the exact decimal string:
/     b10pow 12      ->  1000000000000
/     b10pow 100     ->  "10000000000...0000"        (101 digits)
/     bfact 20       ->  2432902008176640000
/     bfact 25       ->  "15511210043330985984000000"
/     bmul[bfact 25;7]  ->  "108578470303316901888000000"   (chains fine)
/ so a result prints as a NUMBER, never as the raw limb vector.  (An earlier
/ version returned the limb vector from every verb, which made a correct
/ answer like 10^100 read as `0 0 0 ... 10000`.)
/ 
/ The raw kernels are the z* verbs: they take and return the internal form --
/ a little-endian vector of base-10^6 limbs -- and skip the conversion, which
/ is what you want inside a tight loop.  Base 10^6 keeps limb*limb + carry
/ below 2^53, so every carry is exact, and decimal I/O is trivial (one limb is
/ exactly six digits).  Bignums are NON-NEGATIVE; zsub[x;y] assumes x>=y.
/ Primes come from the built-in `pri.
/ ============================================================================

B:1000000                                          / limb base = 10^6

/ ---- raw limb kernels (internal form in, internal form out) ---------------
znorm:{$[|/0<x;(1+|/&0<x)#x;,0]}                    / strip high zero limbs
zprop:{{(B!x)+0,-1_ _x%B}/x}                        / carry-propagate (converge)
zsprop:{{(x+B*x<0)-0,-1_x<0}/x}                     / borrow-propagate (converge)
zint:{$[x<0;`err"euler: bignums are non-negative";x<B;,x;(B!x),zint[_x%B]]}
p6:{(-6)#"000000",$x}                               / one limb -> 6 padded digits
zstr:{r:|znorm[x];,/(,$*r),p6'1_r}                  / limbs -> decimal string
zdig:{(zstr[x])-"0"}                                / limbs -> big-endian digits
zadd:{n:1+(#x)|#y;znorm[zprop[(x,(n-#x)#0)+y,(n-#y)#0]]}
zsub:{znorm[zsprop[x-y,((#x)-#y)#0]]}               / needs x>=y
zmuls:{znorm[zprop[(x*y),0]]}                       / limbs * small int (y<B)
zmul:{L:1+(#x)+#y;znorm[zprop[+/{[i;a;b;L]t:(i#0),a[i]*b;t,(L-#t)#0}[;x;y;L]'!#x]]}
zfromdig:{(,0){zadd[zmuls[x;10];,y]}/x}             / digit list -> limbs
zfromstr:{zfromdig[x-"0"]}                          / decimal string -> limbs
zcmp:{$[(#x)>#y;1;(#x)<#y;-1;[d:x-y;$[~|/~d=0;0;$[0<d[|/&~d=0];1;-1]]]]}
zdivmods:{[a;d]r:(();0){[d;s;l]c:l+B*s[1];(s[0],_c%d;d!c)}[d]/|a;(znorm[|r[0]];r[1])}
zshl1:{znorm[0,x]}                                  / * B (shift up one limb)
qf:{[b;r]*{1<x[1]-x[0]}{[b;r;s]m:_(s[0]+s[1])%2;$[-1<zcmp[r;zmuls[b;m]];(m;s[1]);(s[0];m)]}[b;r]/0,B}
zdivmod:{[a;b]s:(();,0){[b;s;l]r:zadd[zshl1[s[1]];,l];q:qf[b;r];(s[0],q;zsub[r;zmuls[b;q]])}[b]/|a;(znorm[|s[0]];s[1])}
zexp:{[b;e]$[e=0;,1;[h:zexp[b;_e%2];h:zmul[h;h];$[2!e;zmul[b;h];h]]]}
zfact:{(,1)zmul/zint'1+!x}
zgcd:{$[(znorm y)~,0;znorm x;zgcd[y;(zdivmod[x;y])1]]}
zfib:{[n]*(n{(x[1];zadd[x[0];x[1]])}/(,0;,1))}

/ ---- the two conversions every public verb goes through -------------------
/ bx accepts an int atom, a decimal string, a single char, or the limb form.
/ An int VECTOR is already the limb form, so `@x` (uppercase = vector) is the
/ test; a lowercase type symbol means an atom, which needs converting.
bx:{$[`C=@x;zfromstr x;`c=@x;zfromstr[,x];"a"<*$@x;zint[_x];x]}   / NB zint[_x]: `zint _x` would parse as dyadic DROP
/ Three limbs top out at 999999999999999999, comfortably inside a machine int,
/ so anything wider has to come back as a string to stay exact.
bv:{r:znorm x;$[4>#r;B/|r;(4=#r)&(r 3)<9;B/|r;zstr r]}
bnorm:{znorm x}                                     / kept: strip high zero limbs

/ ---- decimal I/O ----------------------------------------------------------
bint:{bx x}                       / explicit -> internal limb form (rarely needed)
bstr:{zstr bx x}                  / -> decimal string, whatever the size
bshow:{zstr bx x}
bdig:{zdig bx x}                  / -> big-endian decimal digits
bfromdig:{bv zfromdig x}
bfromstr:{bv zfromstr x}

/ ---- compare --------------------------------------------------------------
bcmp:{zcmp[bx x;bx y]}            / -1 / 0 / 1
beq:{0=zcmp[bx x;bx y]}           / compares VALUES, so 5 ~ "5" ~ ,5
blt:{-1=bcmp[x;y]}
bgt:{1=bcmp[x;y]}
ble:{1>bcmp[x;y]}
bge:{-1<bcmp[x;y]}
bmin:{$[bgt[x;y];bv bx y;bv bx x]}
bmax:{$[blt[x;y];bv bx y;bv bx x]}
bzero:{0=zcmp[bx x;,0]}
bone:{0=zcmp[bx x;,1]}

/ ---- add / subtract / multiply --------------------------------------------
badd:{bv zadd[bx x;bx y]}
bsub:{bv zsub[bx x;bx y]}         / needs x>=y
bmuls:{bv zmuls[bx x;y]}          / bignum * small int (y<10^6)
bmul:{bv zmul[bx x;bx y]}
bsq:{a:bx x;bv zmul[a;a]}

/ ---- divide ---------------------------------------------------------------
bdivmods:{[x;y]r:zdivmods[bx x;y];(bv r 0;r 1)}     / -> (quotient;remainder)
bdivs:{bv (zdivmods[bx x;y])0}                      / bignum div small int
bmods:{(zdivmods[bx x;y])1}                         / bignum mod small int -> int
bdivmod:{[x;y]r:zdivmod[bx x;bx y];(bv r 0;bv r 1)}
bdiv:{bv (zdivmod[bx x;bx y])0}
bmod:{bv (zdivmod[bx x;bx y])1}
bshl1:{bv zshl1 bx x}                               / * B (shift up one limb)

/ ---- power / factorial / sums / combinatorics -----------------------------
bexp:{[x;y]bv zexp[bx x;y]}
bpow:{[x;y]bv zexp[bx x;y]}                         / base ^ exp, exp>=0
b10pow:{bv zexp[zint 10;x]}                         / 10 ^ x
bfact:{bv zfact x}                                  / x!
bsum:{bv (,0)zadd/bx'x}                             / sum of a list
bprod:{bv (,1)zmul/bx'x}                            / product of a list
bcomb:{bv (zdivmod[zfact x;zmul[zfact y;zfact x-y]])0}   / n choose k
bperm:{bv (zdivmod[zfact x;zfact x-y])0}                 / n permute k

/ ---- fibonacci ------------------------------------------------------------
bfib:{bv zfib x}                                    / n-th Fibonacci (F0=0)
bfibl:{bv'{*x}'(x{(x[1];zadd[x[0];x[1]])}\(,0;,1))} / F0..Fn as a list

/ ---- big digit tools & big gcd/lcm ----------------------------------------
bdsum:{+/zdig bx x}                                 / digit sum
bndig:{#zstr bx x}                                  / number of digits
bdrev:{bv zfromdig[|zdig bx x]}                     / digits reversed
bispal:{p~|p:zdig bx x}                             / palindrome?
bgcd:{bv zgcd[bx x;bx y]}
blcm:{a:bx x;b:bx y;g:zgcd[a;b];$[g~,0;0;bv (zdivmod[zmul[a;b];g])0]}

/ ---- machine-int number theory (built on `pri) ----------------------------
/ isqrt corrects the float sqrt in BOTH directions and iterates, because past
/ 2^52 a double sqrt can land more than one away from the true root.
isqrt:{f:0|-3+_%x;{[x;f]$[~x<(f+1)*f+1;f+1;f]}[x]/[8;f]}   / start below the float sqrt and walk up (k has no >=, so ~x< ); one +-1 correction is not enough past 2^52
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
lcm:{$[(x=0)|y=0;0;y*_x%gcd[x;y]]}

/ ---- machine-int digit tools ----------------------------------------------
digits:{10\x}                                       / decimal digits (big-endian)
digitsum:{+/10\x}                                   / digit sum
numrev:{10/|10\x}                                   / integer, digits reversed
ispalin:{p~|p:10\x}                                 / palindrome?

/ ---- xexp override: exact whole-number power ------------------------------
/ Loading this library REDEFINES xexp so that an integer base with a
/ non-negative integer exponent is computed EXACTLY: an ordinary integer when
/ it fits, otherwise the full decimal string -- 2 xexp 10 is 1024, not 1024.0,
/ and 2 xexp 100 is the whole 31-digit number rather than a rounded double.
/ Fractional or negative exponents fall back to the float form.
xexp:{[x;y]$[(x=_x)&(y=_y)&(~y<0)&~x<0;bv zexp[zint[_x];_y];`exp y*`ln x]}

/ ---- assorted PE helpers --------------------------------------------------
collatz:{-1+#{x>1}{$[2!x;1+3*x;_x%2]}\x}            / Collatz chain length
tri:{_(x*x+1)%2}                                    / x-th triangular number
pent:{_(x*-1+3*x)%2}                                / x-th pentagonal number
hexn:{x*-1+2*x}                                     / x-th hexagonal number
