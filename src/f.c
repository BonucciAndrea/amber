#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include"arena.h"
I rnk(A x/*0*/){X(RA(I v=rnk(xx);P(v<0,v)F(xn,P(v-rnk(xa),-1))v+1)RmM(rnk(xy))RT_A(1)R_(0))}//-1 for mixed rank
Z U urnk(A x/*0*/){X(RA(urnk(xx)+1)RmM(urnk(xy))RT_A(1)R_(0))}//assuming unirank

  U fG(CO G*a,U n,G v)_(U i=0;W(i<n&&a[i]!=v,i++)i)
Z U fH(CO H*a,U n,H v)_(U i=0;W(i<n&&a[i]!=v,i++)i)
  U fI(CO I*a,U n,I v)_(U i=0;W(i<n&&a[i]!=v,i++)i)
  U fL(CO L*a,U n,L v)_(U i=0;W(i<n&&a[i]!=v,i++)i)

Z L fGL(CO V*a,U n,L v)_(P(v!=(G)v,NL)U i=fG(a,n,v);i<n?i:NL)
Z L fHL(CO V*a,U n,L v)_(P(v!=(H)v,NL)U i=fH(a,n,v);i<n?i:NL)
Z L fIL(CO V*a,U n,L v)_(P(v!=(I)v,NL)U i=fI(a,n,v);i<n?i:NL)
Z L fLL(CO V*a,U n,L v)_(             U i=fL(a,n,v);i<n?i:NL)

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
Z A fndL(A x,A y,B srt)_(
 P(srt,0)
 P(!(xtH||xtI||xtL),0)
 U wx=xw-3,wy=yw-3,m=xn,n=yn;
 P(wx>3||wy>3||!m||!n,0)
 CO V*a=xV;CO V*b=yV;
 L lo=RD(wx,a,0),hi=lo;
 F(m,L v=RD(wx,a,i);I(v<lo,lo=v)I(v>hi,hi=v))
 P(hi<lo,0)
 W rg=(W)hi-(W)lo+1,nn=(W)n,mm=(W)m,scan=nn*mm;
 B useL=rg<=LUTDOM&&scan>rg+2*(nn+mm);   /* flat table worth its memset?     */
 B useH=!useL&&scan>8*(nn+mm);           /* else: hash worth its build?      */
 P(!useL&&!useH,0)
 A z=aL(n);L*RES r=zV;
 I(useL,
   I*lut=amal((N)rg*SZ(I));P(!lut,mr(z);0)
   MS(lut,0xff,(N)rg*SZ(I));
   for(U i=m;i--;)lut[(W)RD(wx,a,i)-(W)lo]=(I)i;
   My(F(n,W d=(W)RD(wy,b,i)-(W)lo;I k=d<rg?lut[d]:-1;r[i]=k<0?NL:k))
   free(lut);)
 E(W cap=16,need=2*mm;U lg;W(cap<need,cap<<=1;)
   {W c=cap;lg=0;W(c>1,c>>=1;lg++)}
   U sh=64-lg;W msk=cap-1;
   L*hk=amal((N)cap*SZ(L));I*hv=hk?amal((N)cap*SZ(I)):0;
   P(!hv,I(hk,free(hk))mr(z);0)
   MS(hv,0xff,(N)cap*SZ(I));
   for(U i=m;i--;){L v=RD(wx,a,i);W j=((W)v*GOLD)>>sh;
     W(1,B(hv[j]<0,hk[j]=v;hv[j]=(I)i)B(hk[j]==v,hv[j]=(I)i)j=(j+1)&msk)}
   My(F(n,L v=RD(wy,b,i);
     W j=((W)v*GOLD)>>sh;L k=NL;
     W(1,B(hv[j]<0,)B(hk[j]==v,k=hv[j])j=(j+1)&msk)r[i]=k))
   free(hk);free(hv);)
 z)

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

Z A unqLUT(A x,L lo,W rg,U wx){
 U n=xn;CO V*a=xV;
 UC*seen=amal((N)rg);
 if(!seen)return 0;
 MS(seen,0,(N)rg);
 A z=an(n,xt);V*r=zV;U m=0;
 for(U i=0;i<n;i++){L v=RD(wx,a,i);W s=(W)v-(W)lo;
  if(!seen[s]){seen[s]=1;UNQW(r,m,v,wx);m++;}}
 free(seen);
 return AN(m,z);}

Z A unqHASH(A x,U wx){
 U n=xn;CO V*a=xV;
 W cap=16,need=2*(W)n;U lg;
 while(cap<need)cap<<=1;
 {W c=cap;lg=0;while(c>1){c>>=1;lg++;}}
 U sh=64-lg;W msk=cap-1;
 // slot 0 == empty, so keys are stored biased by +1 on the unsigned line;
 // the bias wraps for v==-1 only, which lands on 0 and is handled by the
 // explicit `has0` flag rather than by the table.
 W*tab=amal((N)cap*SZ(W));
 if(!tab)return 0;
 MS(tab,0,(N)cap*SZ(W));
 A z=an(n,xt);V*r=zV;U m=0;B has0=0;
 for(U i=0;i<n;i++){L v=RD(wx,a,i);W k=(W)v+1;
  if(!k){if(!has0){has0=1;UNQW(r,m,v,wx);m++;}continue;}
  W j=(k*GOLD)>>sh;
  while(tab[j]&&tab[j]!=k)j=(j+1)&msk;
  if(!tab[j]){tab[j]=k;UNQW(r,m,v,wx);m++;}}
 free(tab);
 return AN(m,z);}

// amber: the same value-based idea as v.c's cntrangeF, applied to DISTINCT.
// unqL below rejected every float outright (`if(!(xtH||xtI||xtL))return 0;`), so
// `?x` on a float column fell all the way to the K-level sort-and-dedupe.
// Measured at 10M elements holding 1000 distinct values: 10 ms as int32 against
// 423 ms as float64 -- 42x apart for identical data, decided purely by a type
// test.  A tick feed's prices, sizes and ids are all integral floats.
//
// Same exclusions as the sort side.  NEGATIVE ZERO in particular must be left to
// the old path: `?` separates -0.0 from 0.0 (`?(0.0;-0.0)` returns both) while
// the integer key cannot, so admitting it would silently drop an element.
Z NI B unqrangeF(A x,L*lo,W*rg){
 U n=xn;
 if(!xtF||n<2)return 0;
 CO F*RES a=(CO F*)xV;
 L mn=0,mx=0;
 for(U i=0;i<n;i++){F u=a[i];
  if(!(u>=-9007199254740992.0&&u<=9007199254740992.0))return 0;   // NaN, +-inf, >2^53
  L k=(L)u;
  if((F)k!=u)return 0;                                            // not integral
  if(u==0.0&&__builtin_signbit(u))return 0;                       // -0.0 is distinct from 0.0
  if(!i){mn=mx=k;}else{if(k<mn)mn=k;if(k>mx)mx=k;}}
 W r=(W)mx-(W)mn+1;
 if(!r)return 0;
 *lo=mn;*rg=r;return 1;}

// Both float modes key on the integer VALUE but emit the original double, so
// first-appearance order -- the contract stated above -- is preserved exactly.
Z NI A unqLUTF(A x,L lo,W rg){
 U n=xn;CO F*RES a=(CO F*)xV;
 UC*seen=amal((N)rg);
 if(!seen)return 0;
 MS(seen,0,(N)rg);
 A z=an(n,xt);F*RES r=(F*)zV;U m=0;
 for(U i=0;i<n;i++){W s=(W)(L)a[i]-(W)lo;
  if(!seen[s]){seen[s]=1;r[m++]=a[i];}}
 free(seen);
 return AN(m,z);}

Z NI A unqHASHF(A x){
 U n=xn;CO F*RES a=(CO F*)xV;
 W cap=16,need=2*(W)n;U lg;
 while(cap<need)cap<<=1;
 {W c=cap;lg=0;while(c>1){c>>=1;lg++;}}
 U sh=64-lg;W msk=cap-1;
 W*tab=amal((N)cap*SZ(W));
 if(!tab)return 0;
 MS(tab,0,(N)cap*SZ(W));
 A z=an(n,xt);F*RES r=(F*)zV;U m=0;B has0=0;
 for(U i=0;i<n;i++){L v=(L)a[i];W k=(W)v+1;
  if(!k){if(!has0){has0=1;r[m++]=a[i];}continue;}
  W j=(k*GOLD)>>sh;
  while(tab[j]&&tab[j]!=k)j=(j+1)&msk;
  if(!tab[j]){tab[j]=k;r[m++]=a[i];}}
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

A unqL(A x){
 if(!(xtH||xtI||xtL))return xtF?unqF(x):0;
 U n=xn,wx=xw-3;
 if(n<2||wx>3)return 0;
 CO V*a=xV;
 L lo=RD(wx,a,0),hi=lo;
 for(U i=1;i<n;i++){L v=RD(wx,a,i);if(v<lo)lo=v;if(v>hi)hi=v;}
 W rg=(W)hi-(W)lo+1;
 // rg==0 means the span wrapped the whole 64-bit line (lo=LLONG_MIN,
 // hi=LLONG_MAX); treat that as "sparse", never as a 0-slot table.
 return rg&&rg<=LUTDOM?unqLUT(x,lo,rg,wx):unqHASH(x,wx);}

Z A1(fN,A y=_R(cn[tl]);x(xtt?y:rsz(xN,y)))

Z AM_TLS_IE L t[256];// per-call char/byte find scratch; thread-local for peach workers
Z X1(fndGx,
 R_(fN(x))
 RmMA(e1f(fndGx,x))
 Rilc(L v=gl(x);az(v==(C)v?t[(UC)v]:NL))
 RE(fndGx(gZ(x)))
 RB(fndGx(cG(x)))
 RGHILC(U m=xn;A y=aL(m);
  S4(xw-3,F(m,yl=t[(UC)xc]),
   F(m,H v=xh;yl=v==(C)v?t[(UC)v]:NL),
   F(m,I v=xi;yl=v==(C)v?t[(UC)v]:NL),
   F(m,L v=xl;yl=v==(C)v?t[(UC)v]:NL))
  x(0);sqzZ(y)))
X2(fnd,
 R_(et(y))
 Rm(i1(xx,N(fnd(xy,y))))
 RM(en(y))
 RE(x=gZ(xR);x(fnd(x,y)))
 RA(U k=urnk(x),l=urnk(y);P(k<l+1,r2f(fnd,x,y))P(k>l+1,er(y))
  X(RA(F(xn,P(mtc_(xa,y),y(az(i))))y(az(NL)))
    R_(F(xN,A z=ii(x,i);I m=mtc_(z,y);z(0);P(m,y(az(i))))y(az(NL))))0)
 RB(x=cG(xR);x(fnd(x,y)))
 RGC(F(256,t[i]=NL)UC*a=xV;U n=xn;F(n,t[a[n-1-i]]=n-1-i)fndGx(y))
 R5(tH,tI,tL,tF,tS,
  YmMA(r2f(fnd,x,y))
  YE(fnd(x,gZ(y)))
  P(xt==TT[yt]||xtZ&&ytzZ,
   B srt=!_tP(x)&&xt!=tF&&xt!=tS&&(_at(x)==1||_at(x)==3);TY(fGL)*f=(srt?G(&bGL,bHL,bIL,bLL):G(&fGL,fHL,fIL,fLL))[xw-3];V*a=xV;U m=xn;
   Yt(az(f(a,m,gl(y))))
   A zl_=fndL(x,y,srt);P(zl_,zl_)
   U n=yn;A z=aL(n);My(S4(yw-3,F(n,zl=f(a,m,yg)),F(n,zl=f(a,m,yh)),F(n,zl=f(a,m,yi)),F(n,zl=f(a,m,yl))))z)
  fN(y)))
X2(que,Rs(Z CO C s[][4]={"j","k","hex"};G(&js0,val,unh,ed)[fI((V*)s,L(s),xv)](y))Ril(rnd(gl_(x),y))R_(fnd(x,y)))

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
 Rt(YU(ed(y))fir(N(binZ(x,enl(y)))))
 RmMA(r2f(binZ,x,y))
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

Z Y2(binF,RF(x=of1(xR);x(binZ(x,of1(y))))REBGHILC(binF(x,N(cF(y))))Rt(YU(ed(y))fir(N(binF(x,enl(y)))))RmMA(r2f(binF,x,y))R_(ed(y)))
X2(bin,REBGHILC(binZ(x,y))RF(binF(x,y))Rm(_1(xx,N(bin(xy,y))))R_(et(y)))

//amber: exponential moving average kernel.  y is a float vector (caller-owned);
//returns fresh float vector  z[0]=y[0]; z[i]=a*y[i]+(1-a)*z[i-1].  O(n) single sweep.
Z A emaF(F a,A y)_(U n=yn;A z=aF(n);CO F*RES p=AL(yV);F*RES r=AL(zV);F b=1-a;I(n,F s=r[0]=p[0];for(U i=1;i<n;i++)r[i]=s=a*p[i]+b*s;)z)
//amber: `ema(a;x) -> C-kernel EMA.  a=smoothing factor in (0,1], x=numeric vector.
A1(emaC,P(_t(x)-tA||_n(x)-2,et(x))F a=gf(N(ii(x,0)));A y=N(cF(N(ii(x,1))));A z=emaF(a,y);mr(y);x(z))
//amber: native temporal atom constructors.  `mkd d -> date(days), `mkt m -> time(ms), `mkp n -> timestamp(ns).
A1(mkdt,L v=gl_(x);mr(x);adt((I)v))
A1(mktm,L v=gl_(x);mr(x);atm((I)v))
A1(mknp,L v=gl_(x);mr(x);antp(v))
