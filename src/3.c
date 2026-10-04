#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include"simd.h"
#include"parallel.h"   //2.5 (exp): par_bsum_f64 and friends (undeclared they would return a cut-off int)
#include <stdlib.h>
// ---- amber 1.9.2: vectorised reduction kernels ------------------------------
// The reduction loops below were plain scalar accumulator chains. A float `+/`
// over 10M elements ran at 8.9 ms (~3.5 cycles/element), which is exactly the
// latency of a serialised addsd dependency chain: the compiler may NOT
// auto-vectorise `v += p[i]` because IEEE addition is not associative and
// reassociating it without permission would change results.
//
// Splitting the accumulator into four independent partials breaks that chain
// and lets the vectoriser issue one wide add per group. On the exactly-
// representable integer data the comparative suite specifies, every summation
// order produces bit-identical results (bench/SPEC.md 3), and for general data
// this is pairwise-style summation -- the same trade-off simd.c's existing
// simd_sum_f64() already makes, and typically MORE accurate than a left fold.
//
// AMSIMD/AMPAR expand to OpenMP pragmas only when the compiler actually accepts
// -fopenmp (build.sh probes for it); otherwise they vanish and the four-way
// unrolling alone still does the work, so no build configuration is required.
// AMPARN: element count at and above which fanning a reduction across cores is
// worth the thread startup; the OpenMP `if` clause keeps smaller loops serial.
#define AMPARN 1000000u
#ifdef _OPENMP
 #define AMPRAGMA(x) _Pragma(#x)
 #define AMSIMDR(...) AMPRAGMA(omp simd reduction(__VA_ARGS__))
 // One combined directive: `omp simd` must be immediately followed by the loop,
 // so vectorisation and threading cannot be stacked as two separate pragmas.
 // Variadic, because the four-way kernels below reduce over a LIST of partials
 // (reduction(*:a,b,c,d)): a fixed two-parameter macro cannot carry that, the
 // commas inside the clause get read as extra macro arguments.
 #define AMPFOR(...) AMPRAGMA(omp parallel for simd reduction(__VA_ARGS__) schedule(static))
#else
 #define AMPFOR(...)
 #define AMSIMDR(...)
#endif
// AMRED(n,body,clause...): run `body` threaded when the vector is big enough to
// pay for a team, and as a PLAIN serial loop otherwise. Body comes first because
// the reduction clause contains commas (`*:a,b,c,d`) that only a trailing
// __VA_ARGS__ can absorb.
//
// This replaces `#pragma omp parallel for ... if(n>=AMPARN)`. The `if` clause
// only promises serial *semantics*: GCC still outlines the loop body into an
// _omp_fn and enters a one-thread region, so the sub-threshold path pays region
// setup AND loses the inlining/unrolling the plain loop would have had. Measured
// on addfL (`+/` over longs), if-clause vs. this hard branch:
//    n=1e3  0.6us vs 0.2us (3.3x)   n=1e4  3.5us vs 1.6us (2.2x)
//    n=1e5  32.7us vs 16.2us (2.0x) n=1e6-1 432us vs 237us (1.8x)
// and at/above the threshold the two are identical (within 1%), since the same
// pragma runs. Interactive qSQL aggregates are overwhelmingly sub-threshold, so
// this is the path that matters most.
// amber 2.1: the `parallel for` is GONE. With OMP_NUM_THREADS unset (every
// ordinary user) it spun up a full thread team on every reduction over a
// million elements, and a team of 14 summing 80 MB is SLOWER than one core
// (sum_i measured 8.6 ms threaded against 1.7 ms serial: the data is
// bandwidth-bound and the fork/join is pure cost). Only the `omp simd`
// vectorisation hint remains; multi-core work belongs to peach.
#define AMRED(n,body,...) AMSIMDR(__VA_ARGS__) body
// ---- four-way partial reduction kernels -------------------------------------
// `*/`, `&/` and `|/` were left as single-accumulator scalar chains when `+/`
// was split in 1.9.2, so every element cost one imul (3-cycle latency) or one
// cmov on a serialised dependency chain. Splitting into four independent
// partials breaks the chain -- the same fix, and the same justification, as
// sumF below. Unlike float addition these ARE exactly reassociable: integer
// min/max trivially, and two's-complement multiplication is associative under
// wraparound, so every partitioning gives bit-identical results (verified
// against the pre-patch binary over all four widths x 16 lengths).
//
// MUL4 accumulates in W (unsigned), not L, for the reason src/2.c's integer add
// kernels give: `*/` over longs overflows almost immediately (UBSan flags
// `*/2000000#3` on the old code as "signed integer overflow ... cannot be
// represented in type long long"), and signed overflow is undefined behaviour
// that entitles the optimiser to assume the reassociation away. Unsigned
// wraparound is defined, emits the identical imul, and casts back unchanged.
// Every width accumulates into 64 bits, so no ISA has a vector form -- the
// four-way split is the whole win here (2.2x-6.0x).
#define MUL4(T) CO T*RES p=a;W a0=1,b0=1,c0=1,d0=1;U m=n&~(U)3;\
 AMRED(n,for(U i=0;i<m;i+=4){a0*=(W)p[i];b0*=(W)p[i+1];c0*=(W)p[i+2];d0*=(W)p[i+3];},*:a0,b0,c0,d0)\
 W r=a0*b0*c0*d0;for(U i=m;i<n;i++)r*=(W)p[i];(L)r
// MNM4: four-way min/max. Used for the 64-bit width ONLY. Baseline x86-64 is
// SSE2, which has no 64-bit signed compare, so GCC cannot vectorise a long
// min/max reduction at all and the cmov chain is pure latency -- unrolling wins
// 2.1x-2.5x. At 8/16/32 bits the plain loop DOES vectorise (pcmpgtd + blend),
// and hand-unrolling it measured 2-7x SLOWER because the strided access defeats
// the vectoriser; those widths keep the plain form and take only RES + AMRED.
#define MNM4(T,id,OP,cl) CO T*RES p=a;T a0=id,b0=id,c0=id,d0=id;U m=n&~(U)3;\
 AMRED(n,for(U i=0;i<m;i+=4){a0=OP(a0,p[i]);b0=OP(b0,p[i+1]);c0=OP(c0,p[i+2]);d0=OP(d0,p[i+3]);},cl:a0,b0,c0,d0)\
 T r=OP(OP(a0,b0),OP(c0,d0));for(U i=m;i<n;i++)r=OP(r,p[i]);(L)r
#define MNM1(T,id,OP,cl) CO T*RES p=a;T r=id;AMRED(n,for(U i=0;i<n;i++)r=OP(r,p[i]);,cl:r)(L)r
#define F4(w,n,a,b,c,d) S4(w,F(n,a),F(n,b),F(n,c),F(n,d))
NI A1(inv,x=mut(x);L*p=xL;F(((W)xn<<xw)+255>>8<<2,*p++^=-1)x)

Z A3(___f,/*010*/U i=!y;I(i,y=io(z,0))U n=zn;W(i<n,y=y(x2(y,ii(z,i++)));B(!y))y)
Z A3(dexf,/*010*/A u=las(zR);I(y,y(0))u)
  L addfB(CO V*a,U n)_(CO W*p=a;U r=0;F(n>>6,r+=PC(*p++))n&=63;n?r+PC(*p&~(~0ull<<n)):r)
Z L addfG(CO V*a,U n)_(simd_sum_i8(a,n))
Z L addfH(CO V*a,U n)_(simd_sum_i16(a,n))
Z L addfI(CO V*a,U n)_(simd_sum_i32(a,n))
Z L addfL(CO V*a,U n)_(simd_sum_i64(a,n))
Z L mulfG(CO V*a,U n)_(MUL4(G))
Z L mulfH(CO V*a,U n)_(MUL4(H))
Z L mulfI(CO V*a,U n)_(MUL4(I))
Z L mulfL(CO V*a,U n)_(MUL4(L))
Z L minfG(CO V*a,U n)_(MNM1(G,(G)((1u  << 7)-1),MIN,min))
Z L minfH(CO V*a,U n)_(MNM1(H,(H)((1u  <<15)-1),MIN,min))
Z L minfI(CO V*a,U n)_(MNM1(I,(I)((1u  <<31)-1),MIN,min))
Z L minfL(CO V*a,U n)_(n?MIN((L)((1ull<<63)-1),simd_min_i64(a,n)):(L)((1ull<<63)-1))
Z L maxfG(CO V*a,U n)_(MNM1(G,(G)(1u  << 7),MAX,max))
Z L maxfH(CO V*a,U n)_(MNM1(H,(H)(1u  <<15),MAX,max))
Z L maxfI(CO V*a,U n)_(MNM1(I,(I)(1u  <<31),MAX,max))
Z L maxfL(CO V*a,U n)_(n?MAX((L)(1ull<<63),simd_max_i64(a,n)):(L)(1ull<<63))
  L addfZ(L v,A x/*0*/)_((L)((W)v+(W)G(&addfG,addfH,addfI,addfL)[xw-3](xV,xn)))   //the seed added as the kernels add: wrapping, in W
Z L mulfZ(L v,A x/*0*/)_((L)((W)v*(W)G(&mulfG,mulfH,mulfI,mulfL)[xw-3](xV,xn)))
  L minfZ(L v,A x/*0*/)_(MIN(v,G(&minfG,minfH,minfI,minfL)[xw-3](xV,xn)))
  L maxfZ(L v,A x/*0*/)_(MAX(v,G(&maxfG,maxfH,maxfI,maxfL)[xw-3](xV,xn)))
// sumF: four-way partial float sum (see the header note above).
// amber item 3: the four-way scalar partials are now src/simd.c's kernel, which
// is the same summation order (four independent accumulators) widened to the
// native vector register. Results are bit-identical to the previous four-way
// split, so no benchmark answer moves.
Z F sumF(CO F*RES p,U n)_(par_bsum_f64(p,n))   //2.5 (exp): blocked and parallel from 1M up (parallel.c)
Z A3(admf,/*010*/B i=xv==3;U n=zn;P((y&&ytf)||ztF,F v=y?gf(cF(y)):i;z=cF(zR);CO F*RES q=zV;Mz(I(i,F(n,v*=q[i]))E(v+=sumF(q,n)))af(v))L v=y?gl(y):i;az((i?mulfZ:addfZ)(v,z)))
// -/ reuses the sum: x0-x1-.. is -((-2*x0)+(+/x)), exact for ints (they wrap). Not for
// floats: -2*x0 overflows above 2^1023 and turns an infinite x0 into inf-inf, so `-/0w 1.0`
// was 0n and `-/1e308 1.0` was 0w; and the seed of an empty fold came back negated, as did
// the 0.0 of an empty list. Floats subtract in order instead, as {x-y}/ does.
Z A3(subf,/*010*/P(ztF||y&&ytf,z=cF(zR);CO F*RES q=zV;U i=!y;F v=y?gf(cF(y)):zn?*q:0.0;Mz(W(i<zn,v-=q[i++]))af(v))
 y=y?neg(y):zn?mul(ai(-2),ii(z,0)):ai(0);neg(admf(ADD,y,z)))
// amber item 3: direct float min/max. The general path below folds the whole
// float vector into an order-preserving integer domain with of1(), reduces
// there, and folds back with of0() -- two materialised copies of the vector.
// With no explicit seed and no NaN present, IEEE min/max on the doubles
// themselves gives the identical answer (the implicit seed is -0w/0w, which
// min/max against any real number is a no-op), so take that instead. Any NaN
// at all, or an explicit seed, falls through to the unchanged path: NaN
// ordering is precisely what the of1() domain defines and maxsd does not.
Z A3(mmmf,/*010*/B i=xv==7;
 I(ztF&&!y&&zn,{int nan_=0;CO F*RES q=zV;U nm_=zn;
   F v_=par_mm_f64(q,nm_,i,&nan_);   //2.5 (exp): parallel from 1M up, the serial answer always
   if(!nan_)return af(v_);})
 P((y&&ytf)||ztF,y=y?of1(cF(y)):zn?al(i?NL:WL):of1(aV(tf,1,A((L)((W)i<<63)|WFL)));z=of1(cF(zR));of0(N(z(mmmf(x,y,z)))))
 // |/ of a non-empty int vector starts at 0N, so |/0N 0N is 0N (it was -0W, not an element)
 L v=y?gl(y):i?(zn?NL:-WL):WL;az(zn?(i?maxfZ:minfZ)(v,z):v))
A3(arf,/*010*/Q(xtv)Q(xv<11)Q(!y||ytzfc)Q(ztZFC)
 ZE(P(ztE&&x==ADD&&!y,L i=*zL,j=zL[1];az((j-i)*(j+i-1)/2))z=gZ(zR);z(arf(x,y,z)))
 ZB(z=cG(zR);z(arf(x,y,z)))
 G(&dexf,admf,subf,admf,___f,___f,mmmf,mmmf,___f,___f,___f)[xv](x,y,z))

Z A3(___s,/*010*/U i=!y;A u;I(i,y=ii(z,0);u=enl(yR))E(yR;u=emp(tG))U n=zn;W(i<n,y=y(x2(y,ii(z,i++)));P(!y,u(0))PSH(u,yR))y(u))
Z A3(dexs,/*010*/I(y,y(0))zR)
Z A3(adms,/*010*/L w=y?gl(y):x==MUL;U n=zn;I b=1;L v=w;C t=tG+zw-3;A u=an(n,t);
 //the running total wraps in W, as the folds and K arithmetic do (in L an overflow is undefined behaviour)
 I(x==ADD,F4(zw-3,n,ug=v=(L)((W)v+(W)zg);B(v!=(G)v,b=0),uh=v=(L)((W)v+(W)zh);B(v!=(H)v,b=0),ui=v=(L)((W)v+(W)zi);B(v!=(I)v,b=0),ul=v=(L)((W)v+(W)zl)))
 E(       F4(zw-3,n,ug=v=(L)((W)v*(W)zg);B(v!=(G)v,b=0),uh=v=(L)((W)v*(W)zh);B(v!=(H)v,b=0),ui=v=(L)((W)v*(W)zi);B(v!=(I)v,b=0),ul=v=(L)((W)v*(W)zl)))P(b,u)z=ct(t+1,u(zR));z(adms(x,az(w),z)))
Z A3(subs,/*010*/y=neg(y?y:mul(ai(2),ii(z,0)));neg(adms(ADD,y,z)))
Z A3(mxms,/*010*/P((!y||ytz)&&ztZ,L v=y?gl(y):NL,l=(L)(~0ull<<((1<<zw)-1)),h=~l;U n=zn;I(v<=l||h<=v,P(v>=0,rsz(n,az(v)))v=v<0?l:h)
                                  A u=an(n,zt);F4(zw-3,n,ug=v=MAX(v,zg),uh=v=MAX(v,zh),ui=v=MAX(v,zi),ul=v=MAX(v,zl))u)___s(x,y,z))
Z A3(mnms,/*010*/P((!y||ytz)&&ztZ,z=inv(zR);z(inv(mxms(MXM,y?az(~gl(y)):0,z))))___s(x,y,z))
// ---- amber: float scans ---------------------------------------------------
// ars() accepted only Z (integer) and C (char) data, so every float `+\`, `*\`,
// `&\` and `|\` -- sums/prds/mins/maxs on doubles -- fell through to ___s():
// a boxed loop that allocates an atom and goes through the generic dyadic
// apply once per element. Measured 57 ns/element (maxs over 21600 doubles)
// against 0.69 ns/element for the identical scan on longs. The reduction side
// already had admf/mmmf; these are their scan counterparts.
Z A3(admsf,/*010*/B i=xv==3;U n=zn;A u=an(n,tF);F*RES r=uF;
 F v=y?gf(cF(y)):(F)i;z=cF(zR);CO F*RES q=zV;
 Mz(I(i,F(n,r[i]=v*=q[i]))E(F(n,r[i]=v+=q[i])))u)
// Min/max: reuse the engine's canonical float ordering rather than raw IEEE
// compares, so scan and reduce agree bit-for-bit on NaN and signed zero.
// of1() folds the doubles into an order-preserving integer domain, the fast
// integer scan runs there, of0() folds back -- two extra linear passes, still
// ~15x the boxed path it replaces.
Z A3(mmmsf,/*010*/B i=xv==7;
 // amber 2.3: unseeded, start from the extreme KEY, not from -0w/0w: NaN sorts
 // below -0w here, so `| n 1.0` used to begin -0w instead of 0n.
 y=y?of1(cF(y)):al(i?NL:WL);
 z=of1(cF(zR));
 of0(N(z((i?mxms:mnms)(x,y,z)))))
// float -\: x0, x0-x1, (x0-x1)-x2, ...: in order, as {x-y}\ does (a seed y starts it: y-x0, ...)
Z A3(subsf,/*010*/U n=zn;A u=an(n,tF);F*RES r=uF;F v=y?gf(cF(y)):0;z=cF(zR);CO F*RES q=zV;
 Mz(F(n,r[i]=v=i||y?v-q[i]:q[i]))u)
Z A3(eqls,/*010*/U n=zn,i=!y;L v=gl(y?y:io(z,0)),a=v;A u=aG(n);S4(zw-3,W(i<n,ug=v=v==zg;i++),W(i<n,ug=v=v==zh;i++),W(i<n,ug=v=v==zi;i++),W(i<n,ug=v=v==zl;i++))y||!n?u:a4(u,ai(0),av,az(a)))
A3(ars,/*010*/Q(xtv)Q(xv<11)Q(!y||ytzfc)Q(ztZFC)
 ZE(z=gZ(zR);z(ars(x,y,z)))
 ZB(z=cG(zR);z(ars(x,y,z)))
 P(y&&ytf&&!ztF&&(xv==1||xv==2||xv==3||xv==6||xv==7),z=cF(zR);z(ars(x,y,z)))                     //a float seed: a scan of floats
 P((ztF||y&&ytf)&&xv==10,___s(x,y,z))                                                            //float =\: the generic path (eqls reads ints)
 P(ztF&&xv==2,subsf(x,y,z))                                                                       //float -\: in order (subs is for ints)
 P(ztF&&(xv==1||xv==3),admsf(x,y,z))
 P(ztF&&(xv==6||xv==7),mmmsf(x,y,z))
 G(&dexs,adms,subs,adms,___s,___s,mnms,mxms,___s,___s,eqls)[xv](x,y,z))

// amber 2.4.1: an int seed that fits z's width goes in front of a straight copy of z, one pass,
// where drop-then-join copied z twice (0 :':x is the carry step of every bignum loop)
Z A3(dexp,/*010*/P(!zn,y(zR))
 P(y&&ztZ&&ytz&&tZ(gl_(y))<=zt,L v=gl_(y);U w=zw-3,n=zn;A r=an(n,zt);V*q=_V(r);S4(w,*(G*)q=v,*(H*)q=v,*(I*)q=v,*(L*)q=v);
   MC((C*)q+(1<<w),zV,(n-1)<<w);y(r))
 cat11(y?y:_R(cn[zt]),drp(-1,zR)))
Z A3(___p,/*010*/v2[xv](z,dexp(av,y,z)))
Z A3(modp,/*010*/e2f(mod,z,dexp(av,y,z)))
// amber 2.3: the seed used to be written to z[-1] so the loop could run down to
// j=0 -- i.e. INTO THE HEADER of the input (for int64 storage that is the
// refcount and the length, and only the length was put back): `|':x` on an
// int64 vector corrupted x and crashed later. Element 0 is now done on its own.
// A seed at or past the top of z's width used to answer seed,seed,..: right for |\, not
// for |': (only element 0 sees the seed), so 0W|':3 1 2 and 0N&':x (via mnmp's ~) were
// all seed. Now a seed that fits is just element 0, and one that doesn't widens z.
Z A3(mxmp,/*010*/U w=zw-3;L v=gl(y),l=(L)(~0ull<<(8<<w)-1),h=~l;v=MAX(v,l);N n=zn;P(v>h,A u=cL(zR);u(mxmp(x,az(v),u)))y=an(zn,tG+zw-3);N j=n-1;
 F4(w,n-1,yG[j]=MAX(zG[j],zG[j-1]);j--,yH[j]=MAX(zH[j],zH[j-1]);j--,yI[j]=MAX(zI[j],zI[j-1]);j--,yL[j]=MAX(zL[j],zL[j-1]);j--)
 S4(w,*yG=MAX(*zG,(G)v),*yH=MAX(*zH,(H)v),*yI=MAX(*zI,(I)v),*yL=MAX(*zL,v))y)
Z A3(mnmp,/*010*/y=az(~gl(y));z=inv(zR);z(inv(mxmp(MXM,y,z))))
Z A3(cmpp,/*010*/I o=x-LTN,w=zw-3;U n=zn;A u=aG(n);L v=gl(y),p=iw(z,w,0);*uG=!o?p<v:o==1?p>v:p==v;L m=n-1,j=m;
 S4(o,F4(w,m,uG[j]=zG[j]< zG[j-1];j--,uG[j]=zH[j]< zH[j-1];j--,uG[j]=zI[j]< zI[j-1];j--,uG[j]=zL[j]< zL[j-1];j--),
      F4(w,m,uG[j]=zG[j]> zG[j-1];j--,uG[j]=zH[j]> zH[j-1];j--,uG[j]=zI[j]> zI[j-1];j--,uG[j]=zL[j]> zL[j-1];j--),
      F4(w,m,uG[j]=zG[j]==zG[j-1];j--,uG[j]=zH[j]==zH[j-1];j--,uG[j]=zI[j]==zI[j-1];j--,uG[j]=zL[j]==zL[j-1];j--),)u)
A ucb(A);
A3(arp,/*010*/Q(xtv)Q(xv<11)Q(ytzc)Q(ztZC)
 P(ztC&&ytc&&xv-8<2u,z=ucb(zR);z(arp(x,ucb(y),z)))                                               //< >: chars as unsigned bytes
 ZE(z=gZ(zR);z(arp(x,y,z)))
 ZB(z=cG(zR);z(arp(x,y,z)))
 G(&dexp,___p,___p,___p,___p,modp,mnmp,mxmp,cmpp,cmpp,cmpp)[xv](x,y,z))
// amber 2.3: each-prior over a FLOAT vector, for + - * % & | < > = (not : or !,
// whose element-wise results are not one uniform vector). arp above only takes
// integer/char data, so every float f': fell into p2's boxed loop -- an atom
// allocated and a generic dyad dispatched per element: ~350 ms for =': on 16.6M
// floats against ~10 ms on longs. This is ___p's plan applied to floats: the
// dyad's own vector kernel on (z; seed,-1_z), so the per-pair semantics are
// exactly the atom dyad's (float collation, NaN, -0.0 included). An integer
// seed is converted as the generic path's mixed float/int dyad would.
A3(arpF,/*010*/Q(xtv)Q(ztF)A s=dexp(av,cF(y),z);P(!s,0)v2[xv](z,s))

Z C tZx(A x)_(C t=TX[xt];t?t:tZ(gl_(x)))
C sup(A*p,A*q)_(A x=*p,y=*q;C t=MAX(tZx(x),tZx(y));*p=x=Ny(ct(t,x));*q=y=Nx(ct(t,y));t)
Z A4(dexa,/*1000*/uR;Ny(sup(&x,&u));x=mut(x);U n=yn;I wx=xw-3,wy=yw-3,wu=utt?-1:uw-3;L v=wu<0?gl_(u):0;
  Mu(I(utt,F4(wx,n,xG[iw(y,wy,i)]=v ,xH[iw(y,wy,i)]=v ,xI[iw(y,wy,i)]=v ,xL[iw(y,wy,i)]=v ))
     E(    F4(wx,n,xG[iw(y,wy,i)]=ug,xH[iw(y,wy,i)]=uh,xI[iw(y,wy,i)]=ui,xL[iw(y,wy,i)]=ul)))x)
Z A4(adma,/*1000*/yR;uR;x=cL(x);u=cL(u);x=mut(x);I(!ytL,y=cI(y))U n=yn;
 #define AWR(i,o,v) xL[i]=(L)((W)xL[i] o (W)(v))   //wrapping, in W: an overflow in L is undefined behaviour
 I(utt,L v=gl(u);My(I(zv==1,I(ytL,F(n,AWR(yl,+,v)))E(F(n,AWR(yi,+,v))))E(I(ytL,F(n,AWR(yl,*,v)))E(F(n,AWR(yi,*,v))))))
 E(Mu(           My(I(zv==1,I(ytL,F(n,AWR(yl,+,ul)))E(F(n,AWR(yi,+,ul))))E(I(ytL,F(n,AWR(yl,*,ul)))E(F(n,AWR(yi,*,ul)))))))x)
 #undef AWR
Z A4(mmma,/*1000*/yR;uR;B d=utT;I(!d,u=enl(u))Ny(sup(&x,&u));x=mut(x);I(!ytL,y=cI(y))U n=yn;
 My(Mu(I(zv==6,I(ytL,F4(xw-3,n,xG[yl]=MIN(xG[yl],uG[d*i]),xH[yl]=MIN(xH[yl],uH[d*i]),xI[yl]=MIN(xI[yl],uI[d*i]),xL[yl]=MIN(xL[yl],uL[d*i])))
               E(    F4(xw-3,n,xG[yi]=MIN(xG[yi],uG[d*i]),xH[yi]=MIN(xH[yi],uH[d*i]),xI[yi]=MIN(xI[yi],uI[d*i]),xL[yi]=MIN(xL[yi],uL[d*i]))))
       E(      I(ytL,F4(xw-3,n,xG[yl]=MAX(xG[yl],uG[d*i]),xH[yl]=MAX(xH[yl],uH[d*i]),xI[yl]=MAX(xI[yl],uI[d*i]),xL[yl]=MAX(xL[yl],uL[d*i])))
               E(    F4(xw-3,n,xG[yi]=MAX(xG[yi],uG[d*i]),xH[yi]=MAX(xH[yi],uH[d*i]),xI[yi]=MAX(xI[yi],uI[d*i]),xL[yi]=MAX(xL[yi],uL[d*i]))))))x)
Z A4(suba,/*1000*/u=neg(uR);u(adma(x,y,ADD,u)))
Z B ina(A x/*0*/,U n)_(S4(xw-3,F(xn,P(xg>=n,0)),F(xn,P(xh>=n,0)),F(xn,P(xi>=n,0)),F(xn,P(xl>=(W)n,0)))1)
A4(ara,/*1000*/Q(xtZC)Q(ytZC)Q(ztv)Q(0xcf&1<<zv)Q(utzZ||utcC)
 XE(ara(gZ(x),y,z,u))
 XB(ara(cG(x),y,z,u))
 YE(y=gZ(yR);y(ara(x,y,z,u)))
 YB(y=cG(yR);y(ara(x,y,z,u)))
 P(_tE(u),u=gZ(uR);u(ara(x,y,z,u)))
 P(_tB(u),u=cG(uR);u(ara(x,y,z,u)))
 P(utT&&yn-un,el(x))
 P(!ina(y,xn),ei(x))
 G(&dexa,adma,suba,adma,0,0,mmma,mmma)[zv](x,y,z,u))

// `wsm (x;y) -- FUSED dot product: sum of x*y with no intermediate vector.
//
// simd_dot_f64/simd_dot_i64 have been in src/simd.c since the SIMD backend
// landed, are exported by simd.h and are covered by tests/test_simd.c -- and
// nothing in the engine ever called them.  They were dead code, because the only
// way to spell a dot product was `+/x*y`, which materialises the whole product
// vector and then makes the sum re-read what it just wrote.
//
// amber.k's wsum, wavg and cov are all exactly that shape, and wavg IS vwap --
// the most-used aggregate on a tick desk.  Measured here at 10M float64:
// `+/x*y` costs 4.6 ms, of which the multiply and its 80 MB temporary are
// 3.2 ms and the sum only 0.2 ms.  Reading both inputs once and never writing
// the product is the whole win.
//
// Only the like-typed float and 64-bit-int cases are taken natively; amber.k
// guards the call so anything else keeps the old k expression.
A wsmC(A x){
 if(_t(x)!=tA||_n(x)!=2) return et(x);
 A*a=_A(x);A p=a[0],q=a[1];U n=_n(p);
 if(_n(q)!=n) return et(x);
 if(_t(p)==tF&&_t(q)==tF){F r=par_bdot_f64((CO F*)_V(p),(CO F*)_V(q),n);mr(x);return af(r);}
 // ints: only a 64-bit list can hold 0N, so two narrower ones are k's +/x*y and one is widened; a null on either side gives 0N, so
 // amber.k's wsum knows to take the pairs instead (an int 0N*y wraps, it doesn't stay null) - digest #43
 if(LH(tB,_t(p),tL)&&LH(tB,_t(q),tL)){I(_t(p)!=tL&&_t(q)!=tL,A wp=_R(p),wq=_R(q);mr(x);return K2("{+/x*y}",wp,wq);)   //No null possible: k's own fused +/x*y
  A wp=_t(p)==tL?_R(p):cL(_R(p)),wq=_t(q)==tL?_R(q):cL(_R(q));mr(x);P(!wp||!wq,mr(wp);mr(wq);0)
  CO L*RES wa=_V(wp),*RES wb=_V(wq);W w0=0,w1=0;I wz=0;U i=0;
  for(;i+2<=n;i+=2){L a0=wa[i],a1=wa[i+1],b0=wb[i],b1=wb[i+1];wz|=(a0==NL)|(a1==NL)|(b0==NL)|(b1==NL);w0+=(W)a0*(W)b0;w1+=(W)a1*(W)b1;}
  for(;i<n;i++){L a0=wa[i],b0=wb[i];wz|=(a0==NL)|(b0==NL);w0+=(W)a0*(W)b0;}
  mr(wp);mr(wq);return al(wz?NL:(L)(w0+w1));}
 return et(x);}

// `cvm (x;y) -- cov, scov and cor's sums in one kernel: (sxy;sxx;syy;n), centred, over the pairs with no
// null, in two fused passes and no temporaries. amber.k's cov was avg[x*y]-avg[x]*avg y, which lost digits
// far from 0 and kept the values whose partner was null (digest #43 #46). Floats or ints on either side; a
// narrower int list is widened first (it can't hold a null). Four lanes, as the sums above.
#define CVNF(v) ((v)!=(v))
#define CVNL(v) ((v)==NL)
#define CVM(NM,TX,TY,NX,NY) Z V NM(CO TX*RES cpa,CO TY*RES cpb,U n,F*cvo){F cvs[4]={0},cvt[4]={0};W cvc[4]={0};U i=0,m4=n&~(U)3; \
 for(;i<m4;i+=4)for(I j=0;j<4;j++){TX a_=cpa[i+j];TY b_=cpb[i+j];I k_=!(NX(a_)||NY(b_));cvs[j]+=k_?(F)a_:0.;cvt[j]+=k_?(F)b_:0.;cvc[j]+=k_;} \
 for(;i<n;i++){TX a_=cpa[i];TY b_=cpb[i];I k_=!(NX(a_)||NY(b_));cvs[0]+=k_?(F)a_:0.;cvt[0]+=k_?(F)b_:0.;cvc[0]+=k_;} \
 W cn=cvc[0]+cvc[1]+cvc[2]+cvc[3];F mxa=((cvs[0]+cvs[1])+(cvs[2]+cvs[3]))/(F)cn,mxb=((cvt[0]+cvt[1])+(cvt[2]+cvt[3]))/(F)cn; \
 F sab[4]={0},saa[4]={0},sbq[4]={0};i=0; \
 for(;i<m4;i+=4)for(I j=0;j<4;j++){TX a_=cpa[i+j];TY b_=cpb[i+j];I k_=!(NX(a_)||NY(b_));F da=k_?(F)a_-mxa:0.,db=k_?(F)b_-mxb:0.;sab[j]+=da*db;saa[j]+=da*da;sbq[j]+=db*db;} \
 for(;i<n;i++){TX a_=cpa[i];TY b_=cpb[i];I k_=!(NX(a_)||NY(b_));F da=k_?(F)a_-mxa:0.,db=k_?(F)b_-mxb:0.;sab[0]+=da*db;saa[0]+=da*da;sbq[0]+=db*db;} \
 cvo[0]=(sab[0]+sab[1])+(sab[2]+sab[3]);cvo[1]=(saa[0]+saa[1])+(saa[2]+saa[3]);cvo[2]=(sbq[0]+sbq[1])+(sbq[2]+sbq[3]);cvo[3]=(F)cn;}
CVM(cvmFF,F,F,CVNF,CVNF) CVM(cvmFL,F,L,CVNF,CVNL) CVM(cvmLF,L,F,CVNL,CVNF) CVM(cvmLL,L,L,CVNL,CVNL)
A cvmC(A x){
 if(_t(x)!=tA||_n(x)!=2) return et(x);
 A*a=_A(x);A p=a[0],q=a[1];
 if(_tP(p)||_tP(q)||!(_t(p)==tF||LH(tB,_t(p),tL))||!(_t(q)==tF||LH(tB,_t(q),tL))) return et(x);
 U n=_n(p);if(_n(q)!=n) return el(x);
 p=_t(p)==tF||_t(p)==tL?_R(p):cL(_R(p));q=_t(q)==tF||_t(q)==tL?_R(q):cL(_R(q));mr(x);P(!p||!q,mr(p);mr(q);0)
 A r=an(4,tF);F*o=(F*)_V(r);B fp=_t(p)==tF,fq=_t(q)==tF;
 I(fp&&fq,cvmFF(_V(p),_V(q),n,o))J(fp,cvmFL(_V(p),_V(q),n,o))J(fq,cvmLF(_V(p),_V(q),n,o))E(cvmLL(_V(p),_V(q),n,o))
 mr(p);mr(q);return r;}

// ---- amber 2.1: fused reduction over a dyad -- +/x*y, +/x=y, +/x<y, +/x>y ----
// The compiler (src/b.c fus()) turns `+/ (x DYAD y)` into fredC(dyad;x;y).
// Flat same-typed float or long vectors (and vector/atom pairs for the
// comparisons) run in one pass with no intermediate. Everything else -- and
// any float comparison involving a NaN or a negative zero, whose ordering the
// of1() domain defines differently from IEEE -- is computed EXACTLY as the
// unfused program would: the dyad, then the derived verb +/ applied to it.
Z A fredslow(L d,A x,A y){A t=d==18?cmprC(x,_R(y)):v2[d](x,_R(y));P(!t,0)A dv=_1(aw+1,ADD);A r=_1(dv,t);mr(dv);return r;}
// amber 2.2: `#'=x` (count per group) as one counting pass over a byte, short
// or char vector; keys in first-appearance order and counts squeezed to the
// narrowest width, exactly as `=x` then `#'` produce them. Anything else runs
// the unfused pair. Reached through fredC with code 19 (see b.c fus()).
Z A fcntC(A x){UC tx=_t(x);
 I(!_tP(x)&&(tx==tG||tx==tC),U n=xn;CO UC*p=xV;U c0[256]={0},c1[256]={0},c2[256]={0},c3[256]={0};U i=0;
  for(;i+4<=n;i+=4){c0[p[i]]++;c1[p[i+1]]++;c2[p[i+2]]++;c3[p[i+3]]++;}for(;i<n;i++)c0[p[i]]++;
  U nd=0;F(256,c0[i]+=c1[i]+c2[i]+c3[i];nd+=c0[i]>0)
  UC b[256];U nb=0;UC seen[256]={0};F(n,UC v=p[i];I(!seen[v],seen[v]=1;b[nb++]=v;I(nb==nd,break;)))
  A ky=aV(tx,nb,b);A cn=aL(nb);F(nb,_L(cn)[i]=c0[b[i]])return am(ky,sqzZ(cn));)
 I(!_tP(x)&&tx==tH,U n=xn;CO UH*p=xV;U*c=(U*)calloc(4*65536,SZ(U));P(!c,die("OOM"))U*c1=c+65536,*c2=c+2*65536,*c3=c+3*65536;U i=0;
  for(;i+4<=n;i+=4){c[p[i]]++;c1[p[i+1]]++;c2[p[i+2]]++;c3[p[i+3]]++;}for(;i<n;i++)c[p[i]]++;
  U nd=0;F(65536,c[i]+=c1[i]+c2[i]+c3[i];nd+=c[i]>0)
  UH*b=(UH*)malloc(65536*SZ(UH));P(!b,free(c);die("OOM"))UC*seen=(UC*)calloc(65536,1);P(!seen,free(c);free(b);die("OOM"))U nb=0;
  F(n,UH v=p[i];I(!seen[v],seen[v]=1;b[nb++]=v;I(nb==nd,break;)))
  A ky=aV(tH,nb,b);A cn=aL(nb);F(nb,_L(cn)[i]=c[b[i]])free(c);free(b);free(seen);return am(ky,sqzZ(cn));)
 A g=grp(_R(x));P(!g,0)A dv=_1(aw,LEN);A r=_1(dv,g);mr(dv);return r;}
// amber 2.2: `&x=c` / `&x<c` / `&x>c` on a char or byte vector against an atom
// that fits its width: one compare-and-emit pass, no mask vector. The index
// width is the one `&` on the mask would have chosen. Other shapes run the
// unfused pair (the comparison, then `&`).
Z A whrcmpC(L op,A x,A y){UC tx=_t(x),ty=_t(y);I opc=op==8?0:op==9?1:2;
 I((_tz(x)||tx==tc)&&!_tP(y)&&(ty==tG||ty==tC),return whrcmpC(op==8?9:op==9?8:op,y,x))
 I(!_tP(x)&&(tx==tG||tx==tC)&&(_tz(y)||ty==tc)&&!(tx==tC&&ty==tc),L v=ty==tc?(L)(C)_v(y):gl_(y);   //char with char: unsigned, so the unfused path
  I(tZ(v)<=tG,U n=xn;C t_=tZ((L)n-1);A z=an(n,t_);N k=0;
   I(t_==tG,{G*r=zV;CO G*p=xV;G w=(G)v;F(n,r[k]=(G)i;k+=opc==2?p[i]==w:opc==1?p[i]>w:p[i]<w)})
   J(t_==tH,k=simd_wherecmp_i8_16(xV,(G)v,opc,n,zV))E(k=simd_wherecmp_i8_32(xV,(G)v,opc,n,zV))
   return AN((U)k,z);))
 A t=v2[op](x,_R(y));P(!t,0)return whr(t);}
Z A cmpcmpC(L op,A s,A x,A y){UC ts=_t(s),tx=_t(x),ty=_t(y);I opc=op==8?0:op==9?1:2;
 I((_tz(x)||tx==tc)&&!_tP(y)&&(ty==tG||ty==tC),return cmpcmpC(op==8?9:op==9?8:op,s,y,x))
 I(!_tP(s)&&LH(tG,ts,tS)&&!_tP(x)&&(tx==tG||tx==tC)&&xn==_n(s)&&(_tz(y)||ty==tc)&&!(tx==tC&&ty==tc),L v=ty==tc?(L)(C)_v(y):gl_(y);
  I(tZ(v)<=tG,U n=xn;A z=an(n,ts);N k=0;
   S4(Tw[ts]-3,k=simd_compresscmp_8(_V(s),xV,(G)v,opc,n,zV),k=simd_compresscmp_16(_V(s),xV,(G)v,opc,n,zV),k=simd_compresscmp_32(_V(s),xV,(G)v,opc,n,zV),k=simd_compresscmp_64(_V(s),xV,(G)v,opc,n,zV))
   return AN((U)k,z);))
 A m=v2[op](x,_R(y));P(!m,0)return cmprC(s,m);}
// ---- amber 2.3: the shift idiom (1_x) OP ((-1)_x) --------------------------
// Neighbour-wise comparison and differencing -- `(1_x)<(-1)_x` (descents: the
// sortedness check), `(1_x)=(-1)_x` (runs), `(1_x)-(-1)_x` (deltas) -- dropped
// the vector twice, i.e. copied it twice, before the dyad read both copies: at
// 10M float64, 160 MB written and re-read so that 10 MB of flags could come out.
// The compiler (src/b.c fus()) now emits fredC code 200+op+16*flip (+100 under
// +/) for these shapes and x is read once, in place:
//   flip 0:  r[i] = x[i+1] OP x[i]     (1_x) OP ((-1)_x)
//   flip 1:  r[i] = x[i] OP x[i+1]     ((-1)_x) OP (1_x)
// Result types are exactly the unfused dyad's: comparisons give a byte vector;
// + - * on integers widen past the input width only as far as the results need
// (the unfused kernels detect overflow and redo one width wider, which lands on
// the same width); int64 wraps as simd_add_i64 does; & | keep the input width;
// floats are plain IEEE per element, as the vector kernels are. A float compare
// that meets a NaN or a -0.0 -- where the engine's collation is not IEEE -- and
// every shape not listed here run the unfused program itself (shslow).
Z A shslow(L op,B fl,B s,A x)_(A b=drp(fl?1:-1,_R(x));P(!b,0)A a=drp(fl?-1:1,_R(x));P(!a,mr(b);0)
 A t=v2[op](a,b);mr(a);P(!t||!s,t)A dv=_1(aw+1,ADD);A r=_1(dv,t);mr(dv);r)
#define SHNEG0(v) ({W b_;MC(&b_,&(v),8);b_==0x8000000000000000ull;})
#define SHCMP(T,OPR) {CO T*RES p=xV;I(s,N c=0;I(fl,for(N i=0;i<m;i++)c+=p[i] OPR p[i+1];)E(for(N i=0;i<m;i++)c+=p[i+1] OPR p[i];)return az((L)c);) \
  A z=aG((U)m);G*RES r=zV;I(fl,for(N i=0;i<m;i++)r[i]=p[i] OPR p[i+1];)E(for(N i=0;i<m;i++)r[i]=p[i+1] OPR p[i];)return z;}
#define SHARI(T,OPR,RT) {CO T*RES p=xV;A z=an((U)m,RT);RT##_*RES r=zV;I(fl,for(N i=0;i<m;i++)r[i]=p[i] OPR p[i+1];)E(for(N i=0;i<m;i++)r[i]=p[i+1] OPR p[i];)return z;}
TD F tF_;TD L tL_;
// symbols: interned, so equal ids ARE equal symbols and only a change of id needs
// the string compare that orders symbols (qA's strcmp). A sorted column with a
// few distinct values -- what `sa checks inside xasc -- costs a handful of them.
Z I shsym(U a,U b)_(a==b?0:strcmp(su(a),su(b)))
Z A shiftS(L op,B fl,B s,A x){N n=_n(x),m=n-1;CO U*RES p=(CO U*)xV;N c=0;A z=s?0:aG((U)m);
 for(N i=0;i<m;i++){I d=fl?shsym(p[i],p[i+1]):shsym(p[i+1],p[i]);G r=op==8?d<0:op==9?d>0:d==0;I(s,c+=r)E(zG[i]=r)}
 return s?az((L)c):z;}
Z A shiftC(L code,A x){B s=code>=300;code-=s?300:200;B fl=code>=16;L op=code&15;UC t=_t(x);
 P(!_tP(x)&&t==tS&&op>=8&&_n(x)>=2,shiftS(op,fl,s,x))
 P(_tP(x)||!(t==tG||t==tH||t==tI||t==tL||t==tF||t==tC)||_n(x)<2,shslow(op,fl,s,x))
 N n=_n(x),m=n-1;
 I(op>=8,                                                 // < > =
  I(t==tF,{CO F*RES p=xV;int bad=0;F(n,F v=p[i];bad|=(v!=v)|SHNEG0(v))I(bad,return shslow(op,fl,s,x))})
  S(op*16+t,
   C(8*16+tG,SHCMP(G,<))C(8*16+tH,SHCMP(H,<))C(8*16+tI,SHCMP(I,<))C(8*16+tL,SHCMP(L,<))C(8*16+tF,SHCMP(F,<))C(8*16+tC,SHCMP(UC,<))
   C(9*16+tG,SHCMP(G,>))C(9*16+tH,SHCMP(H,>))C(9*16+tI,SHCMP(I,>))C(9*16+tL,SHCMP(L,>))C(9*16+tF,SHCMP(F,>))C(9*16+tC,SHCMP(UC,>))
   C(10*16+tG,SHCMP(G,==))C(10*16+tH,SHCMP(H,==))C(10*16+tI,SHCMP(I,==))C(10*16+tL,SHCMP(L,==))C(10*16+tF,SHCMP(F,==))C(10*16+tC,SHCMP(G,==)))
  return shslow(op,fl,s,x);)
 P(s||t==tC,shslow(op,fl,s,x))                           // +/ of arithmetic, char arithmetic: unfused
 I(t==tF,S(op,C(1,SHARI(F,+,tF))C(2,SHARI(F,-,tF))C(3,SHARI(F,*,tF))C(4,SHARI(F,/,tF)))return shslow(op,fl,s,x);)
 P(op==4||op>7||op==5||op==0,shslow(op,fl,s,x))          // integer % and anything else: unfused
 // integers: + - * & |.  Exact in 64 bits (wrapping for int64 as the vector
 // kernels do), then stored at the width the unfused dyad would have chosen.
 U w=Tw[t]-3;CO V*p=xV;
 #define SHGET(i) (w==0?(L)((CO G*)p)[i]:w==1?(L)((CO H*)p)[i]:w==2?(L)((CO I*)p)[i]:((CO L*)p)[i])
 #define SHOP(a,b) (op==1?(L)((W)(a)+(W)(b)):op==2?(L)((W)(a)-(W)(b)):op==3?(L)((W)(a)*(W)(b)):op==6?MIN(a,b):MAX(a,b))
 U wr=w;
 I(w<3&&op<=3,L mn=0,mx=0;F(m,L a=SHGET(fl?i:i+1),b=SHGET(fl?i+1:i);L v=SHOP(a,b);I(!i||v<mn,mn=v)I(!i||v>mx,mx=v))
  U wn=MAX(tZ(mn),tZ(mx))-tG;wr=MAX(w,wn);)
 A z=an((U)m,tG+wr);V*q=zV;
 S4(wr,F(m,((G*)q)[i]=(G)SHOP(SHGET(fl?i:i+1),SHGET(fl?i+1:i))),F(m,((H*)q)[i]=(H)SHOP(SHGET(fl?i:i+1),SHGET(fl?i+1:i))),
       F(m,((I*)q)[i]=(I)SHOP(SHGET(fl?i:i+1),SHGET(fl?i+1:i))),F(m,((L*)q)[i]=SHOP(SHGET(fl?i:i+1),SHGET(fl?i+1:i))))
 #undef SHGET
 #undef SHOP
 return z;}
// ---- amber 2.3: masks that are comparisons, fused (F12) --------------------
// +/x@&(c OP k): code 28+op, four arguments (code;x;c;k)
Z A fredcmp(I op,A x,A c,A k){
 A m=v2[8+op](c,_R(k));P(!m,0)A fa[3];fa[0]=az(18);fa[1]=x;fa[2]=m;A r=fredC(fa,3);mr(m);return r;}
// ---- amber 2.3: _x%y on integers -- floor division in one integer pass ------
// `_x%y` is how k spells integer division, and it used to be three passes and
// two temporaries: x widened to doubles, divided, floored back to int64. For an
// integer vector x and an integer atom y it is now one pass (a shift when y is a
// positive power of two), and it is EXACTLY the old answer: when |x|<2^53 and
// 0<|y|<2^53 both convert to doubles exactly, and the rounded quotient can never
// step across an integer (that would need |x|>=2^53), so floor(fl(x)/fl(y)) is
// the true floor quotient. Any element outside that range, or equal to its
// width's minimum (the value integer-to-float conversion treats as null), and
// every other shape, runs the unfused program. The result is int64, as before.
// One width-specialised pass: check the range guard and write the quotient;
// returns 0 (nothing to keep) when the guard fails.
#define FDIV(T,MN) Z B fdiv##T(CO T*RES p,U n,L v,L*RES r){CO L lim=(L)1<<53;I bad=0;          \
  if(v>0&&!(v&(v-1))){U s=(U)CTZ((W)v);                                                  \
    for(U i=0;i<n;i++){L e=(L)p[i];bad|=(e==(L)(MN))|(e>=lim)|(e<=-lim);r[i]=e>>s;}}    \
  else for(U i=0;i<n;i++){L e=(L)p[i];I b=(e==(L)(MN))|(e>=lim)|(e<=-lim);bad|=b;        \
    e=b?0:e;  /* never divide a guarded element: INT64_MIN/-1 traps (SIGFPE) */           \
    L q=e/v;r[i]=q-((e%v!=0)&((e<0)!=(v<0)));}                                           \
  return !bad;}
FDIV(G,-128) FDIV(H,-32768) FDIV(I,-2147483648LL) FDIV(L,NL)
#undef FDIV
Z A fdivC(A x,A y){UC t=_t(x);
 if(!_tP(x)&&(t==tG||t==tH||t==tI||t==tL)&&_tz(y)){
  L v=gl_(y);CO L lim=(L)1<<53;
  if(v&&v!=NL&&v<lim&&v>-lim){U n=xn;A z=aL(n);B ok;
   switch(t){case tG:ok=fdivG(xV,n,v,zL);break;case tH:ok=fdivH(xV,n,v,zL);break;
             case tI:ok=fdivI(xV,n,v,zL);break;default:ok=fdivL(xV,n,v,zL);break;}
   if(ok)return z;
   mr(z);}}
 A u=v2[4](x,_R(y));P(!u,0)return flr(u);}
AA(fredC,/*10..0*/P(n!=3&&n!=4,en(*a))L d=gl(*a);A x=a[1],y=a[2];
 I(n==4,P(d<28||d>30,en0())return fredcmp((I)d-28,x,y,a[3]))
 I(d==40,return fdivC(x,y))
 I(d>=200&&d<400,return shiftC(d,x))
 I(d==19,return fcntC(x))
 I(d>=108&&d<=110,return whrcmpC(d-100,x,y))
 I(d==18,I(!_tP(x)&&!_tP(y)&&ytG&&yn&&yn<=xn,int bad=0;
  I(xtF,F s=simd_masksum_f64(xV,yV,yn,&bad);I(!bad,return af(s)))
  I(xtL,L s=simd_masksum_i64(xV,yV,yn,&bad);I(!bad,return az(s)))))
 I(d==3,I(xtF&&ytF&&xn==yn,return af(par_bdot_f64(xV,yV,xn)))I(xtL&&ytL&&xn==yn,return az(simd_dot_i64(xV,yV,xn))))
 I(d>=8&&d<=10,I op=d==8?0:d==9?1:2,fl=d==8?1:d==9?0:2;int bad=0;
  I(xtF&&ytF&&xn==yn,L c=simd_cntcmpv_f64(xV,yV,xn,op,&bad);I(!bad,return az(c)))
  J(xtF&&ytf,L c=simd_cntcmps_f64(xV,*yF,xn,op,&bad);I(!bad,return az(c)))
  J(xtf&&ytF,L c=simd_cntcmps_f64(yV,*xF,yn,fl,&bad);I(!bad,return az(c)))
  J(xtL&&ytL&&xn==yn,return az(simd_cntcmpv_i64(xV,yV,xn,op)))
  J(xtL&&ytz,return az(simd_cntcmps_i64(xV,gl_(y),xn,op)))
  J(xtz&&ytL,return az(simd_cntcmps_i64(yV,gl_(x),yn,fl))))
 // amber 2.2: byte / short / int / char vectors against an atom that fits
 // their width, or against a vector of the same width, count in one pass.
 I(d>=8&&d<=10,I op=d==8?0:d==9?1:2,fl=d==8?1:d==9?0:2;UC tx=_t(x),ty=_t(y);
  B vx=!_tP(x)&&(tx==tG||tx==tH||tx==tI||tx==tC),vy=!_tP(y)&&(ty==tG||ty==tH||ty==tI||ty==tC);
  U wx=vx?Tw[tx]-3:0,wy=vy?Tw[ty]-3:0;I((tx==tC||tx==tc)&&(ty==tC||ty==tc),vx=vy=0);   //chars with chars compare unsigned: the unfused path
  I(vx&&vy&&wx==wy&&xn==yn,return az(wx==0?simd_cntcmpv_i8(xV,yV,xn,op):wx==1?simd_cntcmpv_i16(xV,yV,xn,op):simd_cntcmpv_i32(xV,yV,xn,op)))
  I(vx&&(_tz(y)||ty==tc),L v=ty==tc?(L)(C)_v(y):gl_(y);I(tZ(v)<=tG+wx,return az(wx==0?simd_cntcmps_i8(xV,(G)v,xn,op):wx==1?simd_cntcmps_i16(xV,(H)v,xn,op):simd_cntcmps_i32(xV,(I)v,xn,op))))
  I(vy&&(_tz(x)||tx==tc),L v=tx==tc?(L)(C)_v(x):gl_(x);I(tZ(v)<=tG+wy,return az(wy==0?simd_cntcmps_i8(yV,(G)v,yn,fl):wy==1?simd_cntcmps_i16(yV,(H)v,yn,fl):simd_cntcmps_i32(yV,(I)v,yn,fl)))))
 fredslow(d,x,y))
// ---- amber 2.1: fused a+s*b / a-s*b with a literal scalar s ---------------
// One pass over a and b; the product is rounded before the add (no FMA
// contraction), so the result is bit-identical to `s*b` then `a+`. The result
// is written in place into whichever float operand is a dying temporary,
// exactly the reuse the unfused path had.
AA(fmaC,/*10..0*/P(n!=4,en(*a))L sb=gl(*a);A x=a[1],sc=a[2],b=a[3];
 I(sb>=108&&sb<=110,return cmpcmpC(sb-100,x,sc,b))
 I(_tF(x)&&_tF(b)&&xn==_n(b)&&(_tf(sc)||_tz(sc)),F sv=_tf(sc)?*_F(sc):(F)gl_(sc);A z=MINE(b)?b:MINE(x)?x:aF(xn);
  simd_fma_f64(xV,sv,_V(b),zV,xn,(int)sb);_at(z)=0;return z==b||z==x?_R(z):z;)
 A t=v2[3](sc,_R(b));P(!t,0)v2[sb?2:1](x,t))
// ---- amber 2.2: +/(a +- s*b)@&m -- the whole chain in one pass -------------
// The compiler (src/b.c fus()) emits this for `+/(a+s*b)@&m` and its `-` and
// operand-order variants. Every piece of it was ALREADY fused -- fmaC does
// a+-s*b, cmprC does x@&m, fredC does +/x@&m -- but chained they still wrote
// and re-read a full-width intermediate for the arithmetic: at 10M float64
// that is 80 MB stored and 80 MB loaded that nothing else ever reads, which is
// most of what separated this expression from the same loop written in C.
// simd_masksum_fma_f64 is bit-identical to the chain (the product is rounded
// before the add, and the accumulation tree is the one simd_masksum_f64 uses),
// so fusing cannot move an answer.
// Anything the kernel does not handle -- a non-float operand, a length
// mismatch, a mask byte above 1 -- is computed by calling fmaC and then fredC,
// i.e. by running EXACTLY the unfused program through the same two entry points
// the compiler would have emitted without this rule.
// The fallback runs the unfused program through the SAME two entry points the
// compiler would have emitted without this rule -- fmaC for the arithmetic,
// then fredC's masked sum -- rather than re-deriving the chain here, so the two
// paths cannot drift apart and the ownership rules are the proven ones.
Z A fmsslow(L sb,A x,A sc,A b,A m){
 A fa[4];fa[0]=az(sb);fa[1]=x;fa[2]=sc;fa[3]=b;
 A u=fmaC(fa,4);if(!u)return 0;
 A fr[3];fr[0]=az(18);fr[1]=u;fr[2]=m;
 A r=fredC(fr,3);mr(u);return r;}
// amber 2.3: six arguments (code;a;s;b;c;k), code = sub + 2*op: the mask c OP k
// is built by the verb, then summed by the five-argument path below, whose sum is
// exact (a kernel that built the mask inside its own loop was slower than this).
Z A fmscmp(L code,A x,A sc,A b,A c,A k){I sub=(I)(code&1),op=(I)(code>>1);
 A m=v2[8+op](c,_R(k));P(!m,0)
 A fa[5];fa[0]=az(sub);fa[1]=x;fa[2]=sc;fa[3]=b;fa[4]=m;A r=fmsC(fa,5);mr(m);return r;}
AA(fmsC,/*10..0*/P(n!=5&&n!=6,en(*a))L sb=gl(*a);A x=a[1],sc=a[2],b=a[3],m=a[4];
 I(n==6,return fmscmp(sb,x,sc,b,m,a[5]))
 I(_tF(x)&&_tF(b)&&xn==_n(b)&&(_tf(sc)||_tz(sc))&&!_tP(m)&&_t(m)==tG&&_n(m)&&_n(m)<=xn,
  F sv=_tf(sc)?*_F(sc):(F)gl_(sc);int bad=0;
  F r=simd_masksum_fma_f64(xV,sv,_V(b),_V(m),_n(m),(int)sb,&bad);
  I(!bad,return af(r)))
 fmsslow(sb,x,sc,b,m))
