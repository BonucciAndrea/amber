#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include"arena.h"
#include"simd.h"
#include"parallel.h"
I rnk(A x/*0*/){X(RA(I v=rnk(xx);P(v<0,v)F(xn,P(v-rnk(xa),-1))v+1)RmM(rnk(xy))RT_A(1)R_(0))}//-1 for mixed rank
U urnk(A x/*0*/){X(RA(urnk(xx)+1)RmM(urnk(xy))RT_A(1)R_(0))}//assuming unirank

  U fG(CO G*a,U n,G v)_(U i=0;W(i<n&&a[i]!=v,i++)i)
Z U fH(CO H*a,U n,H v)_(U i=0;W(i<n&&a[i]!=v,i++)i)
  U fI(CO I*a,U n,I v)_(U i=0;W(i<n&&a[i]!=v,i++)i)
  U fL(CO L*a,U n,L v)_(U i=0;W(i<n&&a[i]!=v,i++)i)

Z L fGL(CO V*a,U n,L v)_(P(v!=(G)v,NL)U i=fG(a,n,v);i<n?i:NL)
Z L fHL(CO V*a,U n,L v)_(P(v!=(H)v,NL)U i=fH(a,n,v);i<n?i:NL)
Z L fIL(CO V*a,U n,L v)_(P(v!=(I)v,NL)U i=fI(a,n,v);i<n?i:NL)
Z L fLL(CO V*a,U n,L v)_(             U i=fL(a,n,v);i<n?i:NL)

//amber 2.2: find on FLOATS went through fLL, which compares the raw 64-bit
// words -- so -0.0 and 0.0 were two different values, while `=`, `~`, `in` and
// `=` (group) all call them equal, and so does q. That also made `?x`
// (distinct) depend on LENGTH rather than on content: o.c reaches find only for
// short vectors and `~':` (match, which calls them equal) for long ones, so
// `#?(-0.0 0.0)` was 2 while `#?(300#-0.0 0.0)` was 1 -- the same values, a
// different answer. Normalising the one double that has two spellings fixes
// both, and costs a compare and a cmov per element.
// The sign-bit-only word, compared UNSIGNED: converting 0x8000..ull to a signed
// L is implementation-defined, and this file is meant to build warning-clean
// on any conforming compiler, not just the two that make it LLONG_MIN.
#define AMNZ(w) ((W)(w)==0x8000000000000000ull?(L)0:(w))
// NaN has many spellings too, and comparing them by word was the same bug:
// `0n` is the positive quiet NaN while a computed NaN is the FPU's default NaN,
// which on x86-64 has the sign bit set, so `0n 1.0?0%0` missed there, and
// `?(0n;0%0)` kept both (a short vector through find; a long one through fcanon's keys,
// fixed in the next commit). `~`, `=` and
// group call every NaN one value, and so does q (`(0n;1f)?neg 0n` is 0), so
// each NaN word is mapped to 0n's: shifting out the sign, a NaN is above inf.
#define AMNF(w) (((W)(w)<<1)>0xffe0000000000000ull?(L)0x7ff8000000000000ll:AMNZ(w))
// Only zero and NaN have more than one spelling, so an ordinary needle is found by its word
// (fL, as before); a zero or NaN needle scans for any of its spellings.
#define AMNW(w) (((W)(w)<<1)>0xffe0000000000000ull)
Z U fFs(CO L*a,U n,L v)_(U i=0;P(!v,W(i<n&&AMNZ(a[i]),i++)i)P(AMNW(v),W(i<n&&!AMNW(a[i]),i++)i)fL(a,n,v))
// fFL pinned to 64 bytes: its loop ran at about half speed where it crossed a 4 KB boundary (Apple cores), so a
// change anywhere before f.c moved float find and distinct by up to 88% (digest #82)
Z L fFL(CO V*,U,L)__attribute__((aligned(64)));
Z L fFL(CO V*a,U n,L v)_(             U i=fFs(a,n,AMNF(v));i<n?i:NL)

//amber: binary search on a sorted(`s#) vector -> O(log n) find; returns index or NL
Z L bGL(CO V*a,U n,L v)_(P(v!=(G)v,NL)CO G*p=a;U lo=0,hi=n;W(lo<hi,U m=lo+hi>>1;I(p[m]<(G)v,lo=m+1)E(hi=m))lo<n&&p[lo]==(G)v?(L)lo:NL)
Z L bHL(CO V*a,U n,L v)_(P(v!=(H)v,NL)CO H*p=a;U lo=0,hi=n;W(lo<hi,U m=lo+hi>>1;I(p[m]<(H)v,lo=m+1)E(hi=m))lo<n&&p[lo]==(H)v?(L)lo:NL)
Z L bIL(CO V*a,U n,L v)_(P(v!=(I)v,NL)CO I*p=a;U lo=0,hi=n;W(lo<hi,U m=lo+hi>>1;I(p[m]<(I)v,lo=m+1)E(hi=m))lo<n&&p[lo]==(I)v?(L)lo:NL)
Z L bLL(CO V*a,U n,L v)_(             CO L*p=a;U lo=0,hi=n;W(lo<hi,U m=lo+hi>>1;I(p[m]<   v,lo=m+1)E(hi=m))lo<n&&p[lo]==   v ?(L)lo:NL)


// ---- amber 1.9.2: O(n+m) integer `?` (direct LUT + compact hash) ----------
// x?y was answered by fLL/fIL/..., a LINEAR SCAN of x per element of y, i.e.
// O(#x * #y). The comparative suite's inner join (1M left keys probed against
// 1000 sparse right keys) therefore ran 500M comparisons -- 211 ms, 126x the C
// baseline and by far Amber's worst cell.
//
// Both replacements below build an index over x ONCE and then answer each probe
// in O(1), preserving `?`'s first-occurrence semantics exactly by filling
// BACKWARDS so the lowest index wins -- the same trick the existing 256-entry
// byte path (fndGx) already uses for tG/tC.
//
//   DIRECT LUT, when the key RANGE is small (<= 64K entries = 256 KB, so the
//   table stays L2-resident): lut[v-lo] = index, no hashing at all.
//
//   COMPACT HASH, otherwise. A direct table is the wrong shape for a SPARSE
//   domain: the benchmark's 1000 keys span a ~1e6 range, so a flat table is
//   4 MB and every probe is an L3/DRAM miss -- measured at 28 ms, only 7x
//   better than the scan. An open-addressed table sized to the key COUNT
//   (2*m rounded up, so 24 KB here) stays in L1 and probes ~10x faster.
//
// Neither is built unless it is cheaper than the scan it replaces, and `s#
// -sorted x keeps its existing O(log m) binary search. Every other shape falls
// through untouched, so this is a pure fast path.
#define LUTDOM  ((W)1<<16)                 /* direct-LUT domain cap, 64K slots */
#define GOLD    0x9E3779B97F4A7C15ull      /* Fibonacci hashing multiplier     */
/* amber item 9, NOT TAKEN. Software-prefetching the hash probe stream was
   tried here and measured slower: fndL sizes the table to the KEY COUNT
   precisely so it stays cache-resident (24 KB for the benchmark's 1000 keys),
   so there is no miss to hide and the extra hash+prefetch per probe is pure
   cost. Interleaved base/new at 10M probes: 87-95 ms without, 96-97 ms with.
   Gating it on table size still left the per-probe branch and did not recover
   the difference. Prefetching belongs where the table genuinely exceeds L2 --
   it does not here, by construction. */
V free(V*);
I posix_memalign(V**,N,N);
Z V*amal(N b)_(V*p=0;P(posix_memalign(&p,64,b),(V*)0)p)
#define RD(w,p,i) ((w)==0?(L)((CO G*)(p))[i]:(w)==1?(L)((CO H*)(p))[i]:(w)==2?(L)((CO I*)(p))[i]:((CO L*)(p))[i])
// amber 2.3: the probe loops are specialised per (probe width, result width),
// so no width is re-tested per element, and the result is written at the
// narrowest width that holds an index into x -- 2 bytes for a 1000-key table,
// where it used to be 8. A miss is written as the sentinel -1 (no index is
// negative); only if one occurred is the result rewritten at 64 bits with 0N
// there, which is exactly the vector the old code built. With no miss the
// values are identical and only the storage width differs, as everywhere else
// Amber narrows integers.
// Amber 2.5 (exp): the probe loop runs over [s,e), so it can be split across threads (fndrun below)
#define FNDP(PT,RT,LOOK) {CO PT*RES pb=(CO PT*)b;RT*RES rr=(RT*)z;for(U i=s;i<e;i++){L v=(L)pb[i];L k;LOOK;rr[i]=(RT)k;miss|=k<0;}}
#define FNDW(LOOK) S4(wy,S4(wz,FNDP(G,G,LOOK),FNDP(G,H,LOOK),FNDP(G,I,LOOK),FNDP(G,L,LOOK)),S4(wz,FNDP(H,G,LOOK),FNDP(H,H,LOOK),FNDP(H,I,LOOK),FNDP(H,L,LOOK)), \
                                S4(wz,FNDP(I,G,LOOK),FNDP(I,H,LOOK),FNDP(I,I,LOOK),FNDP(I,L,LOOK)),S4(wz,FNDP(L,G,LOOK),FNDP(L,H,LOOK),FNDP(L,I,LOOK),FNDP(L,L,LOOK)))
// A probe job: the table (lut, or hk/hv), y's data b, the result z, and one miss flag per slice.
// Workers only read the table and write their own [s,e) of z: no allocation, no refcounts.
TD struct{CO V*b;V*z;U n,wy,wz,nt;I*lut;L lo;W rg;L*hk;I*hv;U sh;W msk;I miss[PAR_MAX_THREADS];}FJ;
Z V fndL_lut(V*c_,int t){FJ*c=c_;CO V*b=c->b;V*z=c->z;U wy=c->wy,wz=c->wz,s=(U)((W)c->n*t/c->nt),e=(U)((W)c->n*(t+1)/c->nt);
 I*lut=c->lut;L lo=c->lo;W rg=c->rg;I miss=0;FNDW(W d=(W)v-(W)lo;k=d<rg?lut[d]:-1)c->miss[t]=miss;}
Z V fndL_hash(V*c_,int t){FJ*c=c_;CO V*b=c->b;V*z=c->z;U wy=c->wy,wz=c->wz,s=(U)((W)c->n*t/c->nt),e=(U)((W)c->n*(t+1)/c->nt);
 L*hk=c->hk;I*hv=c->hv;U sh=c->sh;W msk=c->msk;I miss=0;FNDW(W j=((W)v*GOLD)>>sh;k=-1;W(1,B(hv[j]<0,)B(hk[j]==v,k=hv[j])j=(j+1)&msk))c->miss[t]=miss;}
#define PFND_MIN (1u<<15)   //Below this many probes thread start-up costs more than it saves
Z I fndrun(FJ*c,V(*fn)(V*,int))_(int nt=c->n<PFND_MIN?1:par_thread_count(c->n);c->nt=(U)nt;I(nt<2,fn(c,0))E(par_run(nt,fn,c))I m=0;F(nt,m|=c->miss[i])m)
Z A fndL(A x,A y,B srt)_(
 P(srt,0)
 P(!(xtH||xtI||xtL||xtS),0)   // amber 2.1: symbol vectors too (interned 32-bit ids, compared by value)
 U wx=xw-3,wy=yw-3,m=xn,n=yn;
 P(wx>3||wy>3||!m||!n,0)
 CO V*a=xV;CO V*b=yV;
 L lo=RD(wx,a,0),hi=lo;
 F(m,L v=RD(wx,a,i);I(v<lo,lo=v)I(v>hi,hi=v))
 P(hi<lo,0)
 W rg=(W)hi-(W)lo+1,nn=(W)n,mm=(W)m,scan=nn*mm;
 // rg==0: the keys span the whole 64-bit line and hi-lo+1 wrapped (amber 2.3: this
 // used to pass as a zero-slot LUT and write past it). Sparse, so hash, as unqL does.
 B useL=rg&&rg<=LUTDOM&&scan>rg+2*(nn+mm);   /* flat table worth its memset?     */
 B useH=!useL&&scan>8*(nn+mm);           /* else: hash worth its build?      */
 P(!useL&&!useH,0)
 C tz=tZ((L)m-1);U wz=tz-tG;A z=an(n,tz);I miss=0;  // tZ(m-1): the width every index fits
 I(useL,
   I*lut=amal((N)rg*SZ(I));P(!lut,mr(z);0)
   MS(lut,0xff,(N)rg*SZ(I));
   for(U i=m;i--;)lut[(W)RD(wx,a,i)-(W)lo]=(I)i;
   FJ c={.b=b,.z=zV,.n=n,.wy=wy,.wz=wz,.lut=lut,.lo=lo,.rg=rg};miss=fndrun(&c,fndL_lut);
   free(lut);)
 E(W cap=16,need=2*mm;U lg;W(cap<need,cap<<=1;)
   {W c=cap;lg=0;W(c>1,c>>=1;lg++)}
   U sh=64-lg;W msk=cap-1;
   L*hk=amal((N)cap*SZ(L));I*hv=hk?amal((N)cap*SZ(I)):0;
   P(!hv,I(hk,free(hk))mr(z);0)
   MS(hv,0xff,(N)cap*SZ(I));
   for(U i=m;i--;){L v=RD(wx,a,i);W j=((W)v*GOLD)>>sh;
     W(1,B(hv[j]<0,hk[j]=v;hv[j]=(I)i)B(hk[j]==v,hv[j]=(I)i)j=(j+1)&msk)}
   FJ c={.b=b,.z=zV,.n=n,.wy=wy,.wz=wz,.hk=hk,.hv=hv,.sh=sh,.msk=msk};miss=fndrun(&c,fndL_hash);
   free(hk);free(hv);)
 y(0);
 P(!miss,z)
 // a miss: the 64-bit vector with 0N where -1 was written
 A u=aL(n);L*RES r=uV;S4(wz,F(n,G k=_G(z)[i];r[i]=k<0?NL:k),F(n,H k=_H(z)[i];r[i]=k<0?NL:k),F(n,I k=_I(z)[i];r[i]=k<0?NL:k),F(n,L k=_L(z)[i];r[i]=k<0?NL:k))
 mr(z);u)
#undef FNDW
#undef FNDP

// ---- amber: O(n) integer `?x` (distinct) -----------------------------------
// unq's tH/tI/tL arm in src/o.c fell out of C into the K expression
//     {x@i@<i@:&@[;0;:;1]@~~':x@i:<x}
// -- TWO full grades and THREE materialised permutations of an n-element vector
// to answer a question one hash pass answers. Measured 2963 ms at n=50e6.
//
// The two modes below are the same pair fndL() above already uses, and for the
// same reasons: a DIRECT LUT when the key range is small enough that a flat
// byte table stays cache-resident, and a COUNT-SIZED OPEN-ADDRESSED HASH when
// the domain is sparse (a flat table over a sparse 1e6 domain is megabytes and
// every probe is a DRAM miss, while a table sized to 2n stays much smaller).
//
// FIRST-APPEARANCE ORDER IS THE CONTRACT. `?x` is not `asc ?x`; both modes emit
// on first sight, walking forward, which preserves it exactly.
//
// Returns 0 -- "not handled, use the existing path" -- for any non-integer
// type, a degenerate length, or an allocation failure. Never dies.
#define UNQW(z,i,v,w) S4(w,((G*)(z))[i]=(G)(v),((H*)(z))[i]=(H)(v),((I*)(z))[i]=(I)(v),((L*)(z))[i]=(L)(v))

// amber 2.3: SATURATION. Once as many distinct values have been seen as the
// range [lo,hi] can hold, no later element can be new, and the scan stops -- a
// test on the rare "new value" branch only, free on the hot path. An id or
// category column that uses its whole range (every code 0..999 present) is
// then answered after the first occurrence of its last value instead of after
// all n elements. The loop is also specialised per width (RD re-tested it for
// every element).
#define UNQLP(T) {CO T*RES p=(CO T*)a;T*RES q=(T*)r;for(U i=0;i<n;i++){T v=p[i];W s=(W)(L)v-(W)lo; \
  if(!seen[s]){seen[s]=1;q[m++]=v;if((W)m==rg)break;}}}
Z A unqLUT(A x,L lo,W rg,U wx){
 U n=xn;CO V*a=xV;
 UC*seen=amal((N)rg);
 if(!seen)return 0;
 MS(seen,0,(N)rg);
 A z=an(n,xt);V*r=zV;U m=0;
 S4(wx,UNQLP(G),UNQLP(H),UNQLP(I),UNQLP(L))
 free(seen);
 return AN(m,z);}
#undef UNQLP

// amber 2.1: a BITMAP over the key range whenever it fits in 8 MB (2^26 keys),
// which covers every id/category column in practice: one bit test per element
// against an L2-resident table, no hashing. Beyond that a hash table that
// starts small and GROWS with the number of distinct keys seen (it used to be
// sized to 2n slots up front -- 256 MB for a 10M-element input, random-access
// bound even when the input held a few thousand distinct values).
#define BMDOM ((W)1<<26)
Z A unqBM(A x,L lo,W rg,U wx){
 U n=xn;CO V*a=xV;N nb=(N)((rg+63)>>6);
 W*bm=amal(nb*SZ(W));
 if(!bm)return 0;
 MS(bm,0,nb*SZ(W));
 A z=an(n,xt);V*r=zV;U m=0;
 for(U i=0;i<n;i++){L v=RD(wx,a,i);W s=(W)v-(W)lo;W b=1ull<<(s&63);W*w=bm+(s>>6);
  if(!(*w&b)){*w|=b;UNQW(r,m,v,wx);m++;if((W)m==rg)break;}}   // saturated: see unqLUT
 free(bm);
 return AN(m,z);}
Z A unqHASH(A x,U wx){
 U n=xn;CO V*a=xV;
 W cap=1024;U lg=10;
 W*tab=amal((N)cap*SZ(W));
 if(!tab)return 0;
 MS(tab,0,(N)cap*SZ(W));
 A z=an(n,xt);V*r=zV;U m=0,used=0;B has0=0;
 for(U i=0;i<n;i++){L v=RD(wx,a,i);W k=(W)v+1;
  if(!k){if(!has0){has0=1;UNQW(r,m,v,wx);m++;}continue;}
  if((W)used*2>=cap){                         // grow: rehash every stored key
   W ncap=cap<<1;U nlg=lg+1;W*nt=amal((N)ncap*SZ(W));if(!nt){free(tab);mr(z);return 0;}
   MS(nt,0,(N)ncap*SZ(W));
   for(W s=0;s<cap;s++)if(tab[s]){W kk=tab[s],j=(kk*GOLD)>>(64-nlg);while(nt[j])j=(j+1)&(ncap-1);nt[j]=kk;}
   free(tab);tab=nt;cap=ncap;lg=nlg;}
  W j=(k*GOLD)>>(64-lg),msk=cap-1;
  while(tab[j]&&tab[j]!=k)j=(j+1)&msk;
  if(!tab[j]){tab[j]=k;used++;UNQW(r,m,v,wx);m++;}}
 free(tab);
 return AN(m,z);}

// amber: the same value-based idea as v.c's cntrangeF, applied to DISTINCT.
// unqL below rejected every float outright (`if(!(xtH||xtI||xtL))return 0;`), so
// `?x` on a float column fell all the way to the K-level sort-and-dedupe.
// Measured at 10M elements holding 1000 distinct values: 10 ms as int32 against
// 423 ms as float64 -- 42x apart for identical data, decided purely by a type
// test.  A tick feed's prices, sizes and ids are all integral floats.
//
// NaN is excluded: (L)NaN is undefined and it is not in any integer range.
//
// amber 2.2: NEGATIVE ZERO is no longer excluded. It used to be, on the
// grounds that `?` separates -0.0 from 0.0 and the integer key cannot, so
// admitting it would drop an element. That premise was the bug: `?` only
// separated them here and in the short generic path, while the long generic
// path (`~':`, i.e. match) merged them -- so distinct's answer depended on the
// vector's length. -0.0 and 0.0 are one value everywhere else in Amber and in
// q, so the integer key collapsing them is now the CORRECT behaviour, and this
// path no longer has to be declined for a column that holds a signed zero.
Z NI B unqrangeF(A x,L*lo,W*rg){
 U n=xn;
 if(!xtF||n<2)return 0;
 F mn,mx;int so=0,sp=0;
 simd_frange0_f64((CO F*)xV,n,&mn,&mx,&so,&sp);           // vectorised scan (src/simd.c)
 if(sp&SIMD_FR_NAN)return 0;
 if(!simd_fintegral_f64((CO F*)xV,n))return 0;
 W r=(W)(L)mx-(W)(L)mn+1;
 if(!r)return 0;
 *lo=(L)mn;*rg=r;return 1;}

// Both float modes key on the integer VALUE but emit the original double, so
// first-appearance order -- the contract stated above -- is preserved exactly.
Z NI A unqLUTF(A x,L lo,W rg){
 U n=xn;CO F*RES a=(CO F*)xV;
 UC*seen=amal((N)rg);
 if(!seen)return 0;
 MS(seen,0,(N)rg);
 A z=an(n,xt);F*RES r=(F*)zV;U m=0;
 for(U i=0;i<n;i++){W s=(W)(L)a[i]-(W)lo;
  if(!seen[s]){seen[s]=1;r[m++]=a[i];if((W)m==rg)break;}}   // saturated: see unqLUT
 free(seen);
 return AN(m,z);}

// amber 2.2: this table GROWS with the number of distinct keys, exactly as
// unqHASH above already did. It used to be sized to 2n slots before the first
// probe, which is 16*2n bytes -- 320 MB allocated and memset for a 10M-element
// float column -- even when the column holds a few thousand distinct values.
// That is the same defect the integer path was fixed for in 2.1; the float
// path, sitting a few lines below it, was missed.
Z NI A unqHASHF(A x){
 U n=xn;CO F*RES a=(CO F*)xV;
 W cap=1024;U lg=10;
 W*tab=amal((N)cap*SZ(W));
 if(!tab)return 0;
 MS(tab,0,(N)cap*SZ(W));
 A z=an(n,xt);F*RES r=(F*)zV;U m=0,used=0;B has0=0;
 for(U i=0;i<n;i++){L v=(L)a[i];W k=(W)v+1;
  if(!k){if(!has0){has0=1;r[m++]=a[i];}continue;}
  if((W)used*2>=cap){                         // grow: rehash every stored key
   W ncap=cap<<1;U nlg=lg+1;W*nt=amal((N)ncap*SZ(W));if(!nt){free(tab);mr(z);return 0;}
   MS(nt,0,(N)ncap*SZ(W));
   for(W s=0;s<cap;s++)if(tab[s]){W kk=tab[s],j=(kk*GOLD)>>(64-nlg);while(nt[j])j=(j+1)&(ncap-1);nt[j]=kk;}
   free(tab);tab=nt;cap=ncap;lg=nlg;}
  W j=(k*GOLD)>>(64-lg),msk=cap-1;
  while(tab[j]&&tab[j]!=k)j=(j+1)&msk;
  if(!tab[j]){tab[j]=k;used++;r[m++]=a[i];}}
 free(tab);
 return AN(m,z);}

// The float branch is deliberately LAST and behind a noinline call.  Putting it
// first, inline, cost the INTEGER path 53% (12.0 -> 18.4 ms on 10M) purely
// through code layout -- the hot integer loop stopped being inlined once unqL
// grew.  Measured, not guessed; the ordering here is load-bearing.
Z NI A unqF(A x){
 L lo;W rg;
 if(!unqrangeF(x,&lo,&rg))return 0;
 return rg<=LUTDOM?unqLUTF(x,lo,rg):unqHASHF(x);}

// ---- amber 2.1: `memb (x;y) -- x in y, one pass, one byte per element -------
// amber.k's `in` was ~^y?x: a full index vector (8 bytes per element), a null
// test and a not -- three passes for a boolean. Here the set y is indexed once
// (a bitmap over its range when that fits 8 MB, else a hash sized to the key
// COUNT) and each element of x costs one probe writing one byte. Integer and
// symbol keys only; anything else returns () and amber.k falls back.
// Amber 2.5 (exp): membC's probe loops over [s,e), split across threads like fndL's (fndrun)
TD struct{CO V*a;G*r;U n,wv,nt;W*bm;L lo;W rg;W*tab;U sh;W msk;B has0;}MJ;
#define MSLICE MJ*c=c_;CO V*a=c->a;G*RES r=c->r;U wv=c->wv,s=(U)((W)c->n*t/c->nt),e=(U)((W)c->n*(t+1)/c->nt);
#define MBM(T) {CO T*RES p=(CO T*)a;for(U i=s;i<e;i++){W q=(W)(L)p[i]-(W)lo;r[i]=q<rg&&((bm[q>>6]>>(q&63))&1);}}   //One loop per width: no width test per element
Z V memb_bm(V*c_,int t){MSLICE W*bm=c->bm;L lo=c->lo;W rg=c->rg;S4(wv,MBM(G),MBM(H),MBM(I),MBM(L))}
#define MHS(T) {CO T*RES p=(CO T*)a;for(U i=s;i<e;i++){W k=(W)(L)p[i]+1;if(!k){r[i]=has0;continue;}W j=(k*GOLD)>>sh;while(tab[j]&&tab[j]!=k)j=(j+1)&msk;r[i]=!!tab[j];}}
Z V memb_hash(V*c_,int t){MSLICE W*tab=c->tab;U sh=c->sh;W msk=c->msk;B has0=c->has0;S4(wv,MHS(G),MHS(H),MHS(I),MHS(L))}
#undef MBM
#undef MHS
#undef MSLICE
Z V membrun(MJ*c,V(*fn)(V*,int)){int nt=c->n<PFND_MIN?1:par_thread_count(c->n);c->nt=(U)nt;I(nt<2,fn(c,0))E(par_run(nt,fn,c))}
A1(membC,P(_t(x)-tA||_n(x)-2,et(x))A v=_A(x)[0],y=_A(x)[1];
 B va=_tz(v);UC vt=va?tl:_t(v),yt_=_t(y);
 P(_tP(y)||!(yt_==tH||yt_==tI||yt_==tL||yt_==tS)||!(va||vt==tH||vt==tI||vt==tL||vt==tS),x(emp(tA)))
 U wy=_w(y)-3,m=_n(y),n=va?1:_n(v),wv=va?3:_w(v)-3;CO V*b=_V(y);L sv=va?gl_(v):0;CO V*a=va?&sv:_V(v);
 I(!m,I(va,return x(ai(0)))A z=an(n,tG);MS(_V(z),0,n);return x(z);)
 L lo=RD(wy,b,0),hi=lo;
 F(m,L t=RD(wy,b,i);I(t<lo,lo=t)I(t>hi,hi=t))
 W rg=(W)hi-(W)lo+1;
 A z=an(n,tG);G*RES r=_V(z);
 I(rg&&rg<=BMDOM,N nb=(N)((rg+63)>>6);W*bm=amal(nb*SZ(W));P(!bm,mr(z);x(emp(tA)))MS(bm,0,nb*SZ(W));
  F(m,W s=(W)RD(wy,b,i)-(W)lo;bm[s>>6]|=1ull<<(s&63))
  {MJ c={.a=a,.r=r,.n=n,.wv=wv,.bm=bm,.lo=lo,.rg=rg};membrun(&c,memb_bm);}
  free(bm);)
 E(W cap=16,need=2*(W)m;U lg;W(cap<need,cap<<=1;){W c=cap;lg=0;W(c>1,c>>=1;lg++)}U sh=64-lg;W msk=cap-1;
  W*tab=amal((N)cap*SZ(W));P(!tab,mr(z);x(emp(tA)))MS(tab,0,(N)cap*SZ(W));B has0=0;
  F(m,L t=RD(wy,b,i);W k=(W)t+1;I(!k,has0=1;continue)W j=(k*GOLD)>>sh;W(tab[j]&&tab[j]!=k,j=(j+1)&msk)tab[j]=k)
  {MJ c={.a=a,.r=r,.n=n,.wv=wv,.tab=tab,.sh=sh,.msk=msk,.has0=has0};membrun(&c,memb_hash);}
  free(tab);)
 x(va?({A r_=ai(r[0]);mr(z);r_;}):z))

#include <stdlib.h>   // malloc calloc qsort: undeclared they return a cut-off int
#include <string.h>
// ---- 2.5 (exp): parallel distinct of an int vector, the serial answer exactly ----------------------------
// Per thread, the first index of each value in its slice: a uint32 table over [lo,hi] when the range is small
// (PDQ_LUT), else a hash map capped at PDQ_CAP keys (more and the whole call gives up: return 0, serial runs).
// The minimum over the threads is the value's first index in the vector; the values are written out in the
// order of those indices (a bitmap over n for the table, a sort of the few keys for the hash), which is the
// first-seen order the serial unqLUT/unqBM/unqHASH give.
#define PDQ_MIN (1u<<18)
#define PDQ_LUT (1u<<20)
#define PDQ_CAP (1u<<16)
typedef struct{const void*pa;uint32_t pn,wx;int nt;int64_t lo,mn[64],mx[64];uint64_t rg;uint32_t*fi;
 uint64_t*hk[64];uint32_t*hv[64];uint32_t hcnt[64],h0[64];int stop;}PDQ;
static inline int64_t pdq_rd(const void*p,uint32_t w,uint64_t i){
 return w==1?(int64_t)((const int16_t*)p)[i]:w==2?(int64_t)((const int32_t*)p)[i]:((const int64_t*)p)[i];}
static void pdq_rng(uint32_t n,int nt,int i,uint64_t*lo,uint64_t*hi){uint64_t ch=n/(uint64_t)nt;*lo=i*ch;*hi=i==nt-1?n:*lo+ch;}
#define PDQ_LOOP(T,BODY) {const T*p=(const T*)c->pa;for(uint64_t j=lo;j<hi;j++){int64_t v=(int64_t)p[j];BODY}}
#define PDQ_W(BODY) switch(c->wx){case 1:PDQ_LOOP(int16_t,BODY)break;case 2:PDQ_LOOP(int32_t,BODY)break;default:PDQ_LOOP(int64_t,BODY)}
static void pdq_mm(void*c_,int i){PDQ*c=c_;uint64_t lo,hi;pdq_rng(c->pn,c->nt,i,&lo,&hi);
 int64_t a0=pdq_rd(c->pa,c->wx,lo),b0=a0;PDQ_W(if(v<a0)a0=v;if(v>b0)b0=v;)c->mn[i]=a0;c->mx[i]=b0;}
static void pdq_lut(void*c_,int i){PDQ*c=c_;uint64_t lo,hi;pdq_rng(c->pn,c->nt,i,&lo,&hi);
 uint32_t*f=c->fi+(uint64_t)i*c->rg;memset(f,0xff,c->rg*4);uint64_t seen=0,rg=c->rg;int64_t base=c->lo;
 PDQ_W(uint64_t s=(uint64_t)(v-base);if(f[s]==0xffffffffu){f[s]=(uint32_t)j;if(++seen==rg)break;})}
static void pdq_lmin(void*c_,int i){PDQ*c=c_;uint64_t ch=c->rg/(uint64_t)c->nt,lo=i*ch,hi=i==c->nt-1?c->rg:lo+ch;
 for(int k=1;k<c->nt;k++){const uint32_t*g=c->fi+(uint64_t)k*c->rg;for(uint64_t s=lo;s<hi;s++)if(g[s]<c->fi[s])c->fi[s]=g[s];}}
#define PDQ_GOLD 0x9E3779B97F4A7C15ull
static void pdq_hash(void*c_,int i){PDQ*c=c_;uint64_t lo,hi;pdq_rng(c->pn,c->nt,i,&lo,&hi);
 uint32_t cap=PDQ_CAP*2,lg=17,used=0;uint64_t*k_=calloc(cap,8);uint32_t*v_=malloc((size_t)cap*4);c->hk[i]=k_;c->hv[i]=v_;c->h0[i]=0xffffffffu;
 if(!k_||!v_){c->stop=1;return;}
 PDQ_W(uint64_t k=(uint64_t)v+1;if(!k){if(c->h0[i]==0xffffffffu)c->h0[i]=(uint32_t)j;continue;}
  uint64_t s=(k*PDQ_GOLD)>>(64-lg);while(k_[s]&&k_[s]!=k)s=(s+1)&(cap-1);
  if(!k_[s]){if(++used>PDQ_CAP||(used&1023)==0&&__atomic_load_n(&c->stop,__ATOMIC_RELAXED)){c->stop=1;break;}k_[s]=k;v_[s]=(uint32_t)j;})
 c->hcnt[i]=used;}
typedef struct{uint32_t ix;int64_t v;}PDQE;
static int pdq_cmp(const void*a,const void*b){uint32_t p=((const PDQE*)a)->ix,q=((const PDQE*)b)->ix;return p<q?-1:p>q;}
static A pdq_out(A x,uint32_t m,PDQE*e){A z=an(m,xt);void*r=zV;uint32_t w=xw-3;
 for(uint32_t j=0;j<m;j++){int64_t v=e[j].v;if(w==1)((int16_t*)r)[j]=(int16_t)v;else if(w==2)((int32_t*)r)[j]=(int32_t)v;else((int64_t*)r)[j]=v;}return z;}
static A unqP(A x){uint32_t n=xn,wx=xw-3;int nt=par_thread_count(n);if(nt<2||n>=0xffffffffu)return 0;if(nt>64)nt=64;
 PDQ*c=calloc(1,sizeof(PDQ));if(!c)return 0;c->pa=xV;c->pn=n;c->wx=wx;c->nt=nt;
 par_run(nt,pdq_mm,c);int64_t lo=c->mn[0],hi=c->mx[0];for(int k=1;k<nt;k++){if(c->mn[k]<lo)lo=c->mn[k];if(c->mx[k]>hi)hi=c->mx[k];}
 uint64_t rg=(uint64_t)hi-(uint64_t)lo+1;A z=0;c->lo=lo;c->rg=rg;
 if(rg&&rg<=PDQ_LUT&&rg*(uint64_t)nt*4<=((uint64_t)64<<20)){
  c->fi=malloc(rg*(uint64_t)nt*4);
  if(c->fi){par_run(nt,pdq_lut,c);par_run(nt,pdq_lmin,c);
   uint64_t nw=((uint64_t)n+63)>>6;uint64_t*bm=calloc(nw,8);
   if(bm){uint32_t m=0;for(uint64_t s=0;s<rg;s++){uint32_t f=c->fi[s];if(f!=0xffffffffu){bm[f>>6]|=1ull<<(f&63);m++;}}
    z=an(m,xt);void*r=zV;uint32_t k=0;
    for(uint64_t w=0;w<nw;w++){uint64_t b=bm[w];while(b){uint64_t p=(w<<6)|(uint64_t)__builtin_ctzll(b);b&=b-1;int64_t v=pdq_rd(xV,wx,p);
     if(wx==1)((int16_t*)r)[k]=(int16_t)v;else if(wx==2)((int32_t*)r)[k]=(int32_t)v;else((int64_t*)r)[k]=v;k++;}}
    free(bm);}
   free(c->fi);}}
 else{par_run(nt,pdq_hash,c);
  if(!c->stop){uint64_t tot=1;for(int k=0;k<nt;k++)tot+=c->hcnt[k];
   uint64_t gc=16;while(gc<tot*2)gc<<=1;uint32_t glg=__builtin_ctzll(gc);uint64_t*gk=calloc(gc,8);uint32_t*gv=malloc(gc*4);PDQE*e=malloc(tot*sizeof(PDQE));
   if(gk&&gv&&e){uint32_t m=0,z0=0xffffffffu;
    for(int k=0;k<nt;k++){if(c->h0[k]<z0)z0=c->h0[k];uint64_t*hk=c->hk[k];uint32_t*hv=c->hv[k];
     for(uint32_t s=0;s<PDQ_CAP*2;s++)if(hk[s]){uint64_t key=hk[s],g=(key*PDQ_GOLD)>>(64-glg);while(gk[g]&&gk[g]!=key)g=(g+1)&(gc-1);
      if(!gk[g]){gk[g]=key;gv[g]=hv[s];}else if(hv[s]<gv[g])gv[g]=hv[s];}}
    for(uint64_t g=0;g<gc;g++)if(gk[g]){e[m].ix=gv[g];e[m].v=(int64_t)(gk[g]-1);m++;}
    if(z0!=0xffffffffu){e[m].ix=z0;e[m].v=-1;m++;}
    qsort(e,m,sizeof(PDQE),pdq_cmp);z=pdq_out(x,m,e);}
   free(gk);free(gv);free(e);}
  for(int k=0;k<nt;k++){free(c->hk[k]);free(c->hv[k]);}}
 free(c);return z;}
#undef PDQ_W
#undef PDQ_LOOP

A unqL(A x){
 if(!(xtH||xtI||xtL))return xtF?unqF(x):0;
 U n=xn,wx=xw-3;
 if(n<2||wx>3)return 0;
 I(n>=PDQ_MIN,A z_=unqP(x);I(z_,return z_;))   //2.5 (exp): parallel, the same answer; 0 when it declines
 CO V*a=xV;
 L lo=RD(wx,a,0),hi=lo;
 for(U i=1;i<n;i++){L v=RD(wx,a,i);if(v<lo)lo=v;if(v>hi)hi=v;}
 W rg=(W)hi-(W)lo+1;
 // rg==0 means the span wrapped the whole 64-bit line (lo=LLONG_MIN,
 // hi=LLONG_MAX); treat that as "sparse", never as a 0-slot table.
 return rg&&rg<=LUTDOM?unqLUT(x,lo,rg,wx):rg&&rg<=BMDOM?unqBM(x,lo,rg,wx):unqHASH(x,wx);}

Z A1(fN,A y=_R(cn[tl]);x(xtt?y:rsz(xN,y)))

Z AM_TLS_IE L t[256];// per-call char/byte find scratch; thread-local for peach workers
Z A fndGxW(A x)_(U m=xn;A y=aL(m);
  S4(xw-3,F(m,yl=t[(UC)xc]),
   F(m,H v=xh;yl=v==(C)v?t[(UC)v]:NL),
   F(m,I v=xi;yl=v==(C)v?t[(UC)v]:NL),
   F(m,L v=xl;yl=v==(C)v?t[(UC)v]:NL))
  x(0);sqzZ(y))
// amber 2.2: a char/byte needle list against a char/byte haystack ("acgt"?s)
// used to write an 8-byte index per element and then squeeze it -- 9 bytes of
// traffic per input byte for a result that is one byte wide. The table is
// narrowed first; a "not found" is a sentinel that the OR of the outputs
// reveals, and only then does the old wide path run (its result is identical).
Z X1(fndGx,
 R_(fN(x))
 RmMA(e1f(fndGx,x))
 Rilc(L v=gl(x);az(v==(C)v?t[(UC)v]:NL))
 RE(fndGx(gZ(x)))
 RB(fndGx(cG(x)))
 RGC(U m=xn;L mx=-1;F(256,I(t[i]!=NL&&t[i]>mx,mx=t[i]))CO UC*a=xV;
  I(mx<128,UC tb[256];F(256,tb[i]=t[i]==NL?0x80:(UC)t[i])A y=aG(m);UC acc=0;UC*r=yV;F(m,acc|=r[i]=tb[a[i]])I(!(acc&0x80),return x(y);)mr(y);return fndGxW(x);)
  I(mx<32768,UH th[256];F(256,th[i]=t[i]==NL?0x8000:(UH)t[i])A y=an(m,tH);UH acc=0;UH*r=yV;F(m,acc|=r[i]=th[a[i]])I(!(acc&0x8000),return x(y);)mr(y);return fndGxW(x);)
  fndGxW(x))
 R3(tH,tI,tL,fndGxW(x)))
// does a needle (a generic list, at any depth) hold a number for a char haystack (c), or a char for a number one?
Z B fmx(A x,B c)_(P(xtA,F(xn,P(fmx(xa,c),1))0)c?LH(tE,xt,tF)||LH(ti,xt,tf):xtC||xtc)
// amber 2.7: find in an empty general list: nothing is there, so 0N. The rank test below read the empty list's
// placeholder slot and called an atom the wrong rank: ()?`a was 'rank. A list y keeps its old answer, one 0N.
Z A fnd0(A y)_(P(_tt(y),y(az(NL)))U n=_N(y);A z=aL(n);F(n,zL[i]=NL)y(z))
X2(fnd,
 R_(et(y))
 Rm(i1(xx,N(fnd(xy,y))))
 RM(en(y))
 RE(x=gZ(xR);x(fnd(x,y)))
 // The rank of x's first item says whether y is one item or a list of them; when the items differ in rank, y may be
 // one of them all the same, so it is looked for whole first ((1;"ab")?"ab" is 1, it was 0N 0N). An x of one rank
 // never matches there, so it is only a scan more on these two paths (digest #29)
 RA(U k=urnk(x),l=urnk(y);I(k-l-1,F(xn,P(mtc_(xa,y),y(az(i)))))P(k<l+1,r2f(fnd,x,y))P(k>l+1,xn?er(y):fnd0(y))
  X(RA(F(xn,P(mtc_(xa,y),y(az(i))))y(az(NL)))
    R_(F(xN,A z=ii(x,i);I m=mtc_(z,y);z(0);P(m,y(az(i))))y(az(NL))))0)
 RB(x=cG(xR);x(fnd(x,y)))
 // a char haystack with a number, or a number haystack with a char, is 'type, as in q (issue #20, row 12)
 // Digest #7: bits looked for are widened to bytes first (read as ints they gave wrong rows and read past the end)
 RGC(P(fmx(y,xtC),et(y))YB(fnd(x,cG(y)))F(256,t[i]=NL)UC*a=xV;U n=xn;F(n,t[a[n-1-i]]=n-1-i)fndGx(y))
 R5(tH,tI,tL,tF,tS,
  // Digest #30: an empty general list gets what the byte haystack gives it (0#,!0). Each-right asks the
  // empty list's char prototype, and a char against numbers is 'type (issue #20, row 12)
  P(ytA&&!yn,A g=aG(0);A r=fnd(g,y);mr(g);r)
  P(xt!=tS&&(ytC||ytc),et(y))
  YmMA(r2f(fnd,x,y))
  YE(fnd(x,gZ(y)))
  YB(fnd(x,cG(y)))
  P(xt==TT[yt]||xtZ&&ytzZ,
   // amber 2.3: SORTED only. `p (parted) was also sent here, but parted means
   // equal values are contiguous, not that they are ordered: `pa 3 3 1 1 2 found
   // 1 at 0N. A parted vector takes the ordinary hash/LUT path.
   B srt=!_tP(x)&&xt!=tF&&xt!=tS&&_at(x)==1;TY(fGL)*f=xt==tF?fFL:(srt?G(&bGL,bHL,bIL,bLL):G(&fGL,fHL,fIL,fLL))[xw-3];V*a=xV;U m=xn;
   Yt(az(f(a,m,gl(y))))
   A zl_=fndL(x,y,srt);P(zl_,zl_)
   U n=yn;A z=aL(n);My(S4(yw-3,F(n,zl=f(a,m,yg)),F(n,zl=f(a,m,yh)),F(n,zl=f(a,m,yi)),F(n,zl=f(a,m,yl))))z)
  fN(y)))
X2(que,Rs(Z CO C s[][4] __attribute__((aligned(4)))={"j","k","hex"};G(&js0,val,unh,ed)[fI((V*)s,L(s),xv)](y))Ril(rnd(gl_(x),y))R_(fnd(x,y)))

Z A2 binF;
// ---- amber item 5: batched branchless lower_bound --------------------------
// The per-probe loop in binZ below is already branchless (k[v<xl]=i indexes a
// two-slot array instead of jumping), but every probe is a chain of ~log2(m)
// DEPENDENT loads, each an L3/DRAM miss at scale, and nothing overlaps between
// probes. The core can sustain ~10 outstanding misses and this sustains one.
//
// BINB independent searches are advanced in lockstep instead, so the BINB loads
// issued each round are independent and the memory system overlaps them. The
// trip count is FIXED at ceil(log2(m))+1 for every lane -- a "while any lane is
// active" loop would reintroduce exactly the serialisation being removed -- and
// lanes that have already converged are made no-ops by the lo+1<hi guard.
//
// Semantics are bit-identical to the loop below: lo starts at -1, hi at m, and
// the result is the index of the last element <= v (-1 when v precedes all).
#define BINB 8
#define AMBIN(NM,KT)                                                           Z V NM(CO KT*RES h,U m,CO L*RES pr,L*RES o,U n,U steps){                        U i=0;                                                                         for(;i+BINB<=n;i+=BINB){                                                        L lo[BINB],hi[BINB],v[BINB];                                                   for(U b=0;b<BINB;b++){lo[b]=-1;hi[b]=(L)m;v[b]=pr[i+b];}                       for(U s=0;s<steps;s++)                                                          for(U b=0;b<BINB;b++){                                                          L md=(lo[b]+hi[b])>>1;                                                         if(lo[b]+1<hi[b]){KT hv=h[md];if(v[b]<(L)hv)hi[b]=md;else lo[b]=md;}}        for(U b=0;b<BINB;b++)o[i+b]=lo[b];}                                           for(;i<n;i++){                                                                  L l=-1,e=(L)m,v=pr[i];                                                         while(l+1<e){L md=(l+e)>>1;if(v<(L)h[md])e=md;else l=md;}                      o[i]=l;}}
AMBIN(ambinH,H)
AMBIN(ambinI,I)
AMBIN(ambinL,L)

Z Y2(binZ,
 R_(et(y))
 RF(x=cF(xR);x(binF(x,y)))
 Rt(P(yt>=tdt,et(y))YU(ed(y))fir(N(binZ(x,enl(y)))))   //a temporal atom: 'type (enl of it is a generic list, which recursed)
 RmMA(r2f(bin,x,y))   //each item through bin, so a char item of a char list is compared unsigned
 RE(binZ(x,gZ(y)))
 RB(binZ(x,cG(y)))
 RGHILC(
  XE(x=gZ(xR);x(binZ(x,y)))
  P(xn-(I)xn,ez(y))U wx=xw-3,wy=yw-3;P(!wx,wy?K2("{@[x'`c$127&y;&-128>y;:;-1]}",x,y):K2("{(-1+\\@[&256;128+x;+;1])128+y}",x,y))A z=an(yn,tZ(xn-1));I wz=zw-3,k[2];
  // amber item 5: batch the probes. Both the probe read and the result store
  // are width-switched OUTSIDE the search loop, so the inner loop is a single
  // specialised type with no dispatch in it.
  {U nb_=yn,mb_=xn;U steps=(U)(64-CLZ((W)mb_|1))+1;
   ArenaMark mk_=arena_mark();
   L*pb_=(L*)arena_alloc((N)nb_*SZ(L)),*ob_=(L*)arena_alloc((N)nb_*SZ(L));
   I(pb_&&ob_,{
     S4(wy,F(nb_,pb_[i]=yg),F(nb_,pb_[i]=yh),F(nb_,pb_[i]=yi),F(nb_,pb_[i]=yl))
     S4(wx,,ambinH(xV,mb_,pb_,ob_,nb_,steps),ambinI(xV,mb_,pb_,ob_,nb_,steps),ambinL(xV,mb_,pb_,ob_,nb_,steps))
     S4(wz,F(nb_,zg=ob_[i]),F(nb_,zh=ob_[i]),F(nb_,zi=ob_[i]),F(nb_,zl=ob_[i]))
     arena_release(mk_);return y(z);})
   arena_release(mk_);}
  F(yn,L v;S4(wy,v=yg,v=yh,v=yi,v=yl)*k=-1;k[1]=xn;S4(wx,,W(*k+1<k[1],I i=*k+k[1]>>1;k[v<xh]=i),W(*k+1<k[1],I i=*k+k[1]>>1;k[v<xi]=i),W(*k+1<k[1],I i=*k+k[1]>>1;k[v<xl]=i))
       S4(wz,zg=*k,zh=*k,zi=*k,zl=*k))y(z)))

Z Y2(binF,RF(x=of1(xR);x(binZ(x,of1(y))))REBGHILC(binF(x,N(cF(y))))Rt(P(yt>=tdt,et(y))YU(ed(y))fir(N(binF(x,enl(y)))))RmMA(r2f(binF,x,y))R_(ed(y)))
A ucb(A);
X2(bin,RC(P(ytA||ytm,binZ(x,y))A u=ucb(xR);u(binZ(u,ytc||ytC?ucb(y):y)))REBGHIL(binZ(x,y))RF(binF(x,y))Rm(_1(xx,N(bin(xy,y))))R_(et(y)))

//amber: exponential moving average kernel.  y is a float vector (caller-owned);
//returns fresh float vector  z[0]=y[0]; z[i]=a*y[i]+(1-a)*z[i-1].  O(n) single sweep.
Z A emaF(F a,A y)_(U n=yn;A z=aF(n);CO F*RES p=AL(yV);F*RES r=AL(zV);F b=1-a;I(n,F s=r[0]=p[0];for(U i=1;i<n;i++)r[i]=s=a*p[i]+b*s;)z)
//amber: `ema(a;x) -> C-kernel EMA.  a=smoothing factor in (0,1], x=numeric vector.
A1(emaC,P(_t(x)-tA||_n(x)-2,et(x))F a=gf(N(ii(x,0)));A y=N(cF(N(ii(x,1))));A z=emaF(a,y);mr(y);x(z))
//amber: native temporal atom constructors.  `mkd d -> date(days), `mkt m -> time(ms), `mkp n -> timestamp(ns).
A1(mkdt,L v=gl_(x);mr(x);adt((I)v))
A1(mktm,L v=gl_(x);mr(x);atm((I)v))
A1(mknp,L v=gl_(x);mr(x);antp(v))
