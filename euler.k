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
sqr:{x*x}                                          / x-th square
hept:{_(x*-3+5*x)%2}                               / x-th heptagonal number
oct:{x*-2+3*x}                                     / x-th octagonal number
istri:{s:isqrt 1+8*x;(1+8*x)=s*s}                  / is x triangular?
ispent:{$[x<1;0;[t:1+24*x;s:isqrt t;$[~t=s*s;0;0=6!1+s]]]}   / is x pentagonal?
ishex:{$[x<1;0;[t:1+8*x;s:isqrt t;$[~t=s*s;0;0=4!1+s]]]}     / is x hexagonal?

/ ---- more machine-int number theory (built on `pri, pfactor, divisors) ----
ipow:{$[y=0;1;*/y#x]}                               / x^y for small y (machine int, may overflow past 2^63)
nextprime:{{~isprime x}{x+1}/x+1}                   / smallest prime > x
prevprime:{{~isprime x}{x-1}/x-1}                   / largest prime < x (x>2)
primepi:{#`pri x+1}                                 / number of primes <= x
nthprime:{$[x<1;'`err;[l:`ln x;b:$[x<6;13;3+_x*l+`ln l];(`pri 1+b)x-1]]}   / the x-th prime, 1-indexed
omega:{$[x<2;0;#?pfactor x]}                        / number of DISTINCT prime factors
bigomega:{$[x<2;0;#pfactor x]}                      / number of prime factors WITH multiplicity
radical:{$[x<2;1;*/?pfactor x]}                     / product of the distinct prime factors
issquarefree:{$[x<1;0;f:pfactor x;(#f)=#?f]}        / is x square-free?
mobius:{$[x=1;1;f:pfactor x;$[(#f)>#?f;0;$[2!#f;-1;1]]]}   / Moebius mu(x)
iscoprime:{1=gcd[x;y]}                              / gcd(x;y)=1 ?
sigmak:{[k;n]+/ipow[;k]'divisors n}                 / sum of k-th powers of divisors (k=0 -> ndivisors, k=1 -> sigma)
isperfect:{x=aliquot x}                             / is x a perfect number?
isabundant:{x<aliquot x}                            / is x abundant?
isdeficient:{(aliquot x)<x}                         / is x deficient?
amicable:{a:aliquot x;$[(~a=x)&x=aliquot a;a;0]}    / the amicable partner of x, or 0 if none

/ ---- modular arithmetic (machine int; keep the modulus below ~3e9) --------
modpow:{[b;e;m]*{0<x 2}{[m;s](m!s[0]*$[2!s 2;s 1;1];m!s[1]*s 1;_s[2]%2)}[m]/(1;m!b;e)}   / b^e mod m
egcd:{[a;b]$[b=0;(a;1;0);[e:egcd[b;b!a];(e 0;e 2;(e 1)-(_a%b)*e 2)]]}   / extended gcd -> (g;s;t): a*s+b*t=g
modinv:{[a;m]e:egcd[m!a;m];$[1=e 0;m!m+m!e 1;'`err"no modular inverse"]}   / inverse of a modulo m
mulorder:{[a;n]({[a;n;x]~1=modpow[a;x;n]}[a;n]){x+1}/1}   / multiplicative order of a mod n (needs gcd(a;n)=1)
crt:{[rs;ms]*{[s;p]r1:s 0;m1:s 1;r2:p 0;m2:p 1;M:m1*m2;(M!r1+m1*m2!(r2-r1)*modinv[m1;m2];M)}/[(rs 0;ms 0);+(1_rs;1_ms)]}   / Chinese remainder: solve x = rs[i] (mod ms[i]), coprime moduli
lam1:{[p;e]$[p=2;$[e=1;1;e=2;2;ipow[2;e-2]];(ipow[p;e-1])*p-1]}   / Carmichael lambda of the prime power p^e
carmichael:{$[x<2;1;[d:pfe x;ps:!d;{lcm[x;y]}/lam1'[ps;d ps]]]}   / Carmichael function lambda(x)

/ ---- partitions ----------------------------------------------------------
partinto:{[n;parts]P:parts@&parts<1+n;*|(1,n#0){[n;w;p]w{[p;w;j]@[w;j;+;w j-p]}[p]/p+!1+n-p}[n]/P}   / # partitions of n using the given part sizes (coins: PE31/76/77)
partitions:{$[x<0;0;partinto[x;1+!x]]}              / p(n): number of unrestricted partitions of n
primepart:{partinto[x;`pri x+1]}                    / number of partitions of n into primes (PE77)
partmod:{[n;m]K:2+isqrt 2*n;os:,/{(_(x*-1+3*x)%2;_(x*1+3*x)%2)}'1+!K;sg:,/{$[2!x;1 1;-1 -1]}'1+!K;*|(n{[m;os;sg;p]j:#p;i:&os<1+j;p,m!m++/(sg@i)*p@j-os@i}[m;os;sg]/,1)}   / p(n) mod m, fast Euler pentagonal recurrence (PE78)

/ ---- continued fractions -------------------------------------------------
cfsqrt:{[n]a0:isqrt n;$[n=a0*a0;,a0;({[a0;s]~(s 2)=2*a0}[a0]{[n;a0;s]m:((s 1)*s 2)-s 0;d:_(n-m*m)%s 1;a:_(a0+m)%d;(m;d;a;(s 3),a)}[n;a0]/(0;1;a0;,a0))3]}   / CF of sqrt n as [a0; a1..a_period] (PE64)
cfperiod:{-1+#cfsqrt x}                             / length of the repeating block (0 for a perfect square)
cfe:{[n]2,{[p]$[2=3!p;2*_(p+1)%3;1]}'1+!n-1}        / first n terms of the CF of e: 2,1,2,1,1,4,1,1,6,... (PE65)
cfstep:{[st;a]h:zadd[zmuls[st 0;a];st 1];k:zadd[zmuls[st 2;a];st 3];(h;st 0;k;st 2)}   / one convergent recurrence step (internal)
cfconv:{[cf]s:*|((,1;,0;,0;,1)cfstep\cf);(bv s 0;bv s 2)}   / exact (numerator;denominator) of a finite CF list
pell:{[D]$[D=(isqrt D)*isqrt D;'`err"pell: D must be a non-square";[cs:cfsqrt D;per:1_cs;coeffs:(cs 0),(2*#per)#per;sts:1_((,1;,0;,0;,1)cfstep\coeffs);ok:{[D;s]0=zcmp[zmul[s 0;s 0];zadd[zmul[zint D;zmul[s 2;s 2]];,1]]}[D];s:sts@*&ok'sts;(bv s 0;bv s 2)]]}   / fundamental (x;y) solving x^2 - D y^2 = 1, exact (PE66)

/ ---- base & digit tools --------------------------------------------------
tobase:{[b;n]$[n=0;,0;b\n]}                         / n -> base-b digits (big-endian)
frombase:{[b;ds]b/ds}                               / base-b digits -> n
basedigitsum:{[b;n]+/b\n}                           / digit sum of n written in base b
numcat:{10/(10\x),10\y}                             / concatenate the decimal digits of x and y
isperm:{d:10\x;e:10\y;(d@<d)~e@<e}                  / do x and y share the same multiset of digits?
ispandigital:{d:10\x;(d@<d)~1+!#d}                  / does x use the digits 1..k exactly once (k = #digits)?
rotations:{d:10\x;10/'{[d;k]d@(#d)!k+!#d}[d]'!#d}   / all cyclic digit rotations of x (PE35)
iscircular:{&/isprime'rotations x}                  / are all digit rotations of x prime?

/ ---- bignum extras -------------------------------------------------------
bmodpow:{[b;e;m]mm:bx m;s:{0<x 2}{[mm;s]((zdivmod[zmul[s 0;$[2!s 2;s 1;,1]];mm])1;(zdivmod[zmul[s 1;s 1];mm])1;_s[2]%2)}[mm]/(,1;(zdivmod[bx b;mm])1;e);bv s 0}   / b^e mod m, exact (bignum b and m, machine-int e)
bcatalan:{bdiv[bcomb[2*x;x];1+x]}                   / n-th Catalan number, exact
zpstep:{[os;sg;p]j:#p;i:&os<1+j;pv:p@j-os@i;gi:sg@i;sp:(,0)zadd/pv@&gi>0;sn:(,0)zadd/pv@&gi<0;p,,zsub[sp;sn]}   / one exact-partition DP step (internal)
bpartition:{[n]K:2+isqrt 2*n;os:,/{(_(x*-1+3*x)%2;_(x*1+3*x)%2)}'1+!K;sg:,/{$[2!x;1 1;-1 -1]}'1+!K;p:n zpstep[os;sg]/,,1;bv[*|p]}   / exact p(n) as a bignum
