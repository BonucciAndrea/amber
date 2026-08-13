#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
// ---- vectorisation hints ---------------------------------------------------
// Same probe-don't-assume policy as src/3.c: build.sh only adds -fopenmp when
// the compiler actually accepts it, so where it is absent these expand to
// nothing and the plain loops still compile and produce identical results. They
// are hints; they never change semantics, only whether GCC/Clang is allowed to
// issue an AVX2 / NEON body for a loop it could not otherwise prove safe to
// reassociate or to run without aliasing checks.
#ifdef _OPENMP
 #define VPRAGMA(x) _Pragma(#x)
 #define VSIMD      VPRAGMA(omp simd)
 #define VSIMDR(...) VPRAGMA(omp simd reduction(__VA_ARGS__))
#else
 #define VSIMD
 #define VSIMDR(...)
#endif
X1(flp,Rt(enl(enl(x)))R_(enl(x))RM(A y=kv(&x);am(x,y))RB(flp(cG(x)))
 Rm(A y=kv(&x);Y(RA(I(yn>1,L n=cfm(yA,yn);P(n<0,x(el(y)))F(yn,A z=ya;I(ztt,y=mut(y);ya=rsz(n,z))))aM(x,y))RT_A(aM(x,e1f(enl,y)))R_(x(en(y))))0)
 RA(U m=xn;L n=cfm(xA,m|!m);P(n==-1,enl(x))P(n<0,el(x))C t=_t(xx);I(t<tM&&t-tE,F(m,A y=xa;B(yt-t,t=0)))E(t=0)A y=aA(n);
  P(!t,F(n|!n,A z=aA(m);Fj(m,zA[j]=ii(xA[j],i))I(!zn,zx=mkn(zx))ya=sqz(z))x(0);I(!yn,yx=mkn(yx))y)
  U w=Tw[t]-3;Fj(n|!n,A z=yA[j]=an(m,t);S4(w,F(m,zg=_G(xa)[j]),F(m,zh=_H(xa)[j]),F(m,zi=_I(xa)[j]),F(m|!m,zl=_L(xa)[j])I(TR(t),I(!m,zx=mkn(_R(zx)))yA[j]=sqz(mRa(z)))))
  x(0);I(!n,yx=mkn(yx))y))
// amber: this fills a packed multi-lane counter (w selects 1/2/4/8-byte lanes),
// so the accumulator is *meant* to carry across lane boundaries and wrap at
// 64 bits. Done in L that is signed overflow -- undefined behaviour, and UBSan
// flags it on examples/practice.k. W (unsigned) gives the same bits with
// defined semantics; the stored value is cast back to L unchanged.
// amber 1.9.5: `a` is restrict-qualified and the induction on `q` is turned into
// a closed form (base + i*d) so the store loop carries no dependency at all.
// The old `q+=d` chain forced one lane per iteration; with the stride folded in
// the compiler can issue a wide vector store per group. Identical bit pattern:
// the k-th element was always base + k*d, the accumulator just computed it
// serially. Arithmetic stays in W (unsigned), so the intentional wrap at 64 bits
// this packed multi-lane counter relies on is still defined behaviour.
V tilV(V*p,L v,L n,U w){L*RES a=p;W k=(W)G(0x101010101010101ll,0x1000100010001ll,1ll<<32|1,1)[w],d=k<<(3-w),q=(W)v;
 q*=k;q+=(W)G(0x706050403020100ll,0x3000200010000ll,1ll<<32,0)[w];
 // Explicit loop, not the F() macro: F() declares its bound and its counter in
 // one init clause, which is not an OpenMP canonical loop form, so `omp simd`
 // will not attach to it (src/3.c spells its reduction loops out for the same
 // reason).
 L m=(n-1>>3-w)+4&-4;
 VSIMD for(L i=0;i<m;i++)a[i]=(L)(q+(W)i*d);}
X1(til,RA(K1("{x@'!#'x}",x))Ril(L n=gl(x);I(n==NL,n=0)aE(MIN(0,n),MAX(0,n)))REBGHIL(K1("{(*a)#'&'x#'1_a:|*\\|x,1}",x))RmM(x(_R(xx)))Ro(val(x))RS(gns(_v(jS(x))))Rs(gns(xv))R_(et(x)))
X1(whr,Ril(whr(enl(x)))RA(K1("{$[`A~@x;(,&#'*'x),,'/x@\\:!0|/#'x:o'x;,&x]}",x))Rm(A y=kv(&x);x(x1(Nx(whr(y)))))RE(whr(gZ(x)))R_(et(x))
 RB(U m=xn,n=addfB(xV,m);A y=aI(n);I*r=yV;Mx(F(m+7>>3,C v=xg;W(v,U j=CTZ(v);v&=~(1<<j);*r++=i<<3|j)))Q(r-yI==n);y)
 RGHIL(L m=xn,n=addfZ(0,x);P(n<0||minfZ(0,x)<0,ed(x))C t=tZ(m-!!m);P(t>tI,ez(x))A y=an(n,t);I w=xw-3;
  Mx(S4(t-tG,{G*r=yV;S4(w,F(m,Fj(xg,*r++=i)),F(m,Fj(xh,*r++=i)),F(m,Fj(xi,*r++=i)),F(m,Fj(xl,*r++=i)))},
             {H*r=yV;S4(w,F(m,Fj(xg,*r++=i)),F(m,Fj(xh,*r++=i)),F(m,Fj(xi,*r++=i)),F(m,Fj(xl,*r++=i)))},
             {I*r=yV;S4(w,F(m,Fj(xg,*r++=i)),F(m,Fj(xh,*r++=i)),F(m,Fj(xi,*r++=i)),F(m,Fj(xl,*r++=i)))},))y))
X1(rev,Rm(A y=kv(&x);am(rev(x),rev(y)))RM(A y=kv(&x);aM(x,e1f(rev,y)))Rt(x)RE(rev(gZ(x)))RB(cB(rev(cG(x))))
 R_(P(xn<2,x)x=mut(x);U n=xn;I w=xw-3;S4(w,F(n>>1,SW(xg,xG[n-1-i])),F(n>>1,SW(xh,xH[n-1-i])),F(n>>1,SW(xi,xI[n-1-i])),F(n>>1,SW(xl,xL[n-1-i])))x))
A1(typ,x(as(TS[xt])))
A1(len,x(az(xN)))
U _N(A x/*0*/){X(RE(Lij j-i)RT_E(xn)Rm(_N(xy))RM(_N(_x(xy)))R_(1))}
Y2(fil,RmMA(e2f(fil,x,y))Rt(YU(y-au?y:xR)fir(fil(x,enl(y))))R_(K2("{@[y;&^y;:;x]}",x,y)))
X2(crt,Rt(fil(x,y))R_(en(y))
 RT(I v=rnk(y);P(!v,crt(x,enl(y)))
  P(v>0&&rnk(x)==v,I(xtE&&ytE,Lij L k=*yL,l=yL[1];P(k<=i,y(0);aE(MAX(i,l),MAX(j,l)))P(j<=l,y(0);aE(i,MIN(j,k))))K2("{x@&^y?x}",x,y))
  K2("{x@&~(!0),x~\\:y}/",x,y)))
B tru(A x/*1*/)_(B v=xtU?x!=au:xtt?!!gl_(x):!!xN;x(0);v)
X1(imx,RGHILC(imn(inv(x)))RF(imx(of1(x)))RE(Lij x(0);az(j-i?j-i-1:NL))R_(fir(N(dsc(x)))))
 // amber 1.9.5: argmin as two vectorisable passes instead of one branchy scan.
 // The old body was `if(p[i]<v){v=p[i];j=i;}` -- a loop-carried dependency on
 // BOTH the running minimum and the running index, plus a data-dependent branch,
 // which pins it to roughly one element per iteration and blocks the vectoriser
 // outright (it cannot reassociate a reduction that also carries an index).
 // Splitting it into (1) a pure `min` reduction, which `omp simd reduction(min:)`
 // turns into one wide pminu/pmins per group, and (2) a scan for the FIRST
 // element equal to that minimum, which exits at the answer, gives the identical
 // result -- "first index of the smallest value" is exactly what both phrasings
 // compute -- while the expensive phase now runs at vector width.
 // The base pointer is pulled out of the loop and restrict-qualified, so no
 // object header is re-read and no runtime aliasing check is emitted.
 // Written with explicit `for`s rather than the F() macro: F() declares its
 // bound and its counter in a single init clause, which is not an OpenMP
 // canonical loop form, so `omp simd` would silently fail to attach (src/3.c
 // spells its reduction loops out for exactly the same reason).
 //
 // Each width is a real FUNCTION rather than a block pasted into S4()'s argument
 // list. That is not a style choice: VSIMDR expands to _Pragma, and a _Pragma
 // that materialises inside another macro's argument list gets repositioned by
 // the preprocessor to a point where the reduction variable is not yet in scope.
 // GCC then rejects it outright ("'v' undeclared"), and it does so on some GCC
 // builds but not others -- the same defect that broke src/i.c's reducers on a
 // WSL toolchain while compiling clean here. Inside a plain function body the
 // pragma sits at a statement position, which is the only well-defined place
 // for it. Do not inline these back into S4().
#define VIMN_FN(T,NM) Z L NM(CO T*RES p,N n){ \
  I(!n,return NL)                        /* empty vector -> null index, and    */ \
  T v=*p;                                /* never dereference p[0]             */ \
  VSIMDR(min:v)                                                                   \
  for(N i=0;i<n;i++)v=p[i]<v?p[i]:v;     /* pass 1: pure min, runs at vector width */ \
  for(N i=0;i<n;i++)if(p[i]==v)return(L)i;/* pass 2: FIRST index holding it     */ \
  return 0;}
VIMN_FN(G,vimnG) VIMN_FN(H,vimnH) VIMN_FN(I,vimnI) VIMN_FN(L,vimnL)
#undef VIMN_FN
X1(imn,RF(imn(of1(x)))RE(Lij x(0);az(NL*(i==j)))R_(fir(N(asc(x))))
 RGHILC(N n=xn;L j;S4(xw-3,j=vimnG(xV,n),j=vimnH(xV,n),j=vimnI(xV,n),j=vimnL(xV,n))x(az(j))))

// ============================================================================
// BATCH 2 -- (1) single-pass O(n) moving-window aggregates
//            (2) cache-friendly LSD radix grade for 8/16/32/64-bit integer,
//                IEEE-754 double and timestamp vectors
//
// Both kernels take ALL of their transient workspace from the HFT scratch arena
// (arena.h) -- no malloc, no free, no per-element object churn -- and both
// bracket that workspace in arena_mark()/arena_release() so a kernel invoked a
// thousand times inside ONE K expression still peaks at a single generation of
// scratch instead of a thousand (arena_reset() only runs at the end of an
// evaluation cycle; see evs() in src/m.c).
//
// PREPROCESSOR NOTE (non-negotiable, see the VIMN_FN comment above): every
// loop here lives in a plain function body. Nothing in this file puts a
// _Pragma -- or a macro that expands to one -- inside the argument list of a
// variadic macro such as a.h's F(...) / C(...) / D(...) / S4(...). GCC's
// argument prescan relocates such a pragma to a point where the loop's
// variables are not yet in scope and rejects the translation unit outright, on
// some toolchains but not others. Plain `for` at statement position only.
// ============================================================================
#include"arena.h"

// ---- 1. moving-window aggregates -------------------------------------------
// `mw (code; w; x)  ->  the window aggregate as a vector, or () to tell the
// caller (std.k) to fall back to the portable K definition.
//
// kdb/K window semantics: a GROWING window over the first w-1 points, then a
// fixed w-wide window. The old K definitions were
//     msum: sums 0.0+x  then a shifted subtraction   -- O(n) but three full
//           materialised vectors and two passes
//     mmin/mmax: {&/x@(0|1+j-w)+!(1+j)&w}'!#x        -- O(n*w), and it builds
//           one K list PER ELEMENT (n index vectors + n slices)
//
// Here msum/mavg/mvar/mdev are one pass with a running difference
//     new_sum = old_sum + incoming - outgoing
// and mmin/mmax are one pass over a monotonic index deque, which is O(n)
// *independent of w* -- each index is pushed once and popped at most once.
//
// NULLS. 0n (any NaN) is treated as ABSENT rather than poisoning the rest of
// the vector: it is neither added to the running sum nor counted, so
//     msum -> the sum of the non-null members of the window (0 if all null)
//     mavg -> sum / count-of-non-nulls, or 0n when the window is all null
//     mmin/mmax -> the extreme of the non-null members, 0n when there are none
// which is what kdb does and what the prefix-sum version could not do (one 0n
// anywhere made every later element 0n). Integer nulls (0Ni/0N) are recognised
// on the sum path and mapped to 0n. On null-free input every result is
// identical to the K definitions it replaces.
enum{MWSUM,MWAVG,MWVAR,MWDEV,MWMIN,MWMAX};
#define MWNI ((I)(-2147483647-1))          // 0Ni
// Numerical hygiene for the running difference. Adding and later subtracting
// the same double is not exactly reversible, so over millions of elements the
// running sum drifts away from the true window sum -- and the drift is
// UNBOUNDED, because nothing ever re-anchors it. Every MW_RESYNC elements the
// accumulators are recomputed from the raw window, which caps the error at the
// drift of one resync interval. Cost is O(n*w/MW_RESYNC), i.e. under 0.5% of
// the pass for a typical w, and it is skipped entirely once w is wide enough
// that the resync would dominate.
#define MW_RESYNC 4096u
#define MWVARQ ({F m_=c?s/(F)c:NF,q_=c?ss/(F)c-m_*m_:NF;q_<0?0.0:q_;})
// `ss` is only touched by the variance/deviation instantiations; NEEDSS is a
// literal 0/1 so the multiply-accumulate vanishes from the sum/avg bodies.
#define MWRUN(NM,NEEDSS,EMIT)                                                  \
Z V NM(CO F*RES p,F*RES r,N n,N w){                                            \
  F s=0,ss=0;N c=0,per=(w<=(N)MW_RESYNC)?(N)MW_RESYNC:0;                       \
  for(N i=0;i<n;i++){                                                          \
    F v=p[i];                                                                  \
    if(v==v){s+=v;if(NEEDSS)ss+=v*v;c++;}                                      \
    if(i>=w){F o=p[i-w];if(o==o){s-=o;if(NEEDSS)ss-=o*o;c--;}}                 \
    if(per&&i>=w&&!(i%per)){                                                   \
      F a_=0,q_=0;N k_=0;                                                      \
      for(N j=i+1-w;j<=i;j++){F u=p[j];if(u==u){a_+=u;if(NEEDSS)q_+=u*u;k_++;}}\
      s=a_;ss=q_;c=k_;}                                                        \
    r[i]=(EMIT);}}
MWRUN(mwsum,0,s)
MWRUN(mwavg,0,c?s/(F)c:NF)
MWRUN(mwvar,1,MWVARQ)
MWRUN(mwdev,1,SQ(MWVARQ))

// Monotonic deque. `dq` holds indices whose values are strictly increasing
// (min) / decreasing (max); the front is therefore the extreme of the live
// window, and an index leaves the deque either because a newer element beats it
// or because it fell out of the window. Amortised O(1) per element, so the
// whole pass is O(n) no matter how wide w is.
#define MWDQ1(NM,T,ISNUL,CMP)                                                  \
Z V NM(CO T*RES p,T*RES r,N n,N w,U*RES dq){                                    \
  N h=0,t=0;                                                                   \
  for(N i=0;i<n;i++){                                                          \
    T v=p[i];                                                                  \
    if(!(ISNUL)){                                                              \
      while(t>h&&!(p[dq[t-1]] CMP v))t--;                                      \
      dq[t++]=(U)i;}                                                           \
    while(t>h&&(N)dq[h]+w<=i)h++;                                              \
    r[i]=t>h?p[dq[h]]:v;}}   /* t==h only when the whole window was null */
#define MWDQ(T,SFX,ISNUL) MWDQ1(mwmin##SFX,T,ISNUL,<) MWDQ1(mwmax##SFX,T,ISNUL,>)
MWDQ(G,G,0) MWDQ(H,H,0) MWDQ(I,I,0) MWDQ(L,L,0) MWDQ(F,F,v!=v)

// Widen any supported numeric vector to double, mapping the integer nulls onto
// 0n so the sum kernels see one uniform "absent" marker.
Z V mwld(A c,F*RES d,N n){CO V*q=_V(c);switch(_t(c)){
  case tG:{CO G*RES p=q;for(N i=0;i<n;i++)d[i]=(F)p[i];}break;
  case tH:{CO H*RES p=q;for(N i=0;i<n;i++)d[i]=(F)p[i];}break;
  case tI:{CO I*RES p=q;for(N i=0;i<n;i++)d[i]=p[i]==MWNI?NF:(F)p[i];}break;
  case tL:{CO L*RES p=q;for(N i=0;i<n;i++)d[i]=p[i]==NL?NF:(F)p[i];}break;
  default:{CO F*RES p=q;for(N i=0;i<n;i++)d[i]=p[i];}break;}}

A mwC(A x){
 P(_t(x)-tA||_n(x)-3,x(emp(tA)))
 A*e=(A*)_V(x);A cd=e[0],wa=e[1],c=e[2];
 P(!_tz(cd)||!_tz(wa),x(emp(tA)))
 L code=gl_(cd),w=gl_(wa);
 P(code<0||code>MWMAX||w<1||w==NL,x(emp(tA)))
 UC t=_t(c);N n=_n(c);
 P(_tP(c),x(emp(tA)))                           // atom: use the K path
 P(!(t==tG||t==tH||t==tI||t==tL||t==tF),x(emp(tA)))
 P(code<MWMIN&&t==tG,x(emp(tA)))                // char-ish byte vectors: K path
 // An empty vector is answered here rather than punted: the K definitions
 // cannot do it at all -- `(-w)_s` on a shorter-than-w vector is a 'length
 // error -- so falling back would turn "no rows" into an exception.
 P(!n,x(an(0,code<=MWDEV?tF:t)))
 ArenaMark mk=arena_mark();A y=0;
 if(code<=MWDEV){
   F*RES d=(F*)arena_alloc(n*SZ(F));
   P(!d,arena_release(mk);x(emp(tA)))
   mwld(c,d,n);
   y=an((U)n,tF);F*RES r=(F*)_V(y);
   switch((U)code){
     case MWSUM: mwsum(d,r,n,(N)w);break;
     case MWAVG: mwavg(d,r,n,(N)w);break;
     case MWVAR: mwvar(d,r,n,(N)w);break;
     default:    mwdev(d,r,n,(N)w);break;}
 }else{
   U*RES dq=(U*)arena_alloc(n*SZ(U));
   P(!dq,arena_release(mk);x(emp(tA)))
   y=an((U)n,t);V*r=_V(y);CO V*p=_V(c);int mx=code==MWMAX;
   switch(t){
     case tG: mx?mwmaxG(p,r,n,(N)w,dq):mwminG(p,r,n,(N)w,dq);break;
     case tH: mx?mwmaxH(p,r,n,(N)w,dq):mwminH(p,r,n,(N)w,dq);break;
     case tI: mx?mwmaxI(p,r,n,(N)w,dq):mwminI(p,r,n,(N)w,dq);break;
     case tL: mx?mwmaxL(p,r,n,(N)w,dq):mwminL(p,r,n,(N)w,dq);break;
     default: mx?mwmaxF(p,r,n,(N)w,dq):mwminF(p,r,n,(N)w,dq);break;}
 }
 arena_release(mk);
 return x(y);}

// ---- 2. LSD radix grade ----------------------------------------------------
// The engine. Keys and their companion row indices travel TOGETHER through the
// ping-pong buffers, so every pass is a sequential read plus a bucketed write.
// The previous kernel (ascZ in src/o.c) re-read the value array through the
// permutation on every pass -- v[w*a[i]+j] -- which is a full random gather per
// byte, i.e. eight scattered passes over the whole vector for a 64-bit column.
//
// Two further wins over a textbook LSD radix:
//   * ALL nb histograms are built in ONE sequential pass, not one pass each.
//   * A byte column that is constant across the vector contributes nothing to
//     the order, so its permute pass is skipped outright. Real data is full of
//     these: a long vector of small non-negative values leaves five of its
//     eight byte columns at zero, so it costs three passes, not eight.
// Radix passes only ever permute the keys, so the histograms stay valid for the
// whole run and the constant-column test can be answered from element 0 -- which
// also means the whole pass PLAN is known before the first permute, so the final
// pass can stop carrying the keys altogether (nothing reads them afterwards) and
// writes only the index array.
//
// Returns whichever index buffer holds the result (ia or ib -- the caller must
// use the returned pointer, not the one it passed in).
#define AMRDX(SC,NM,KT)                                                        \
SC I* NM(KT*RES ka,I*RES ia,KT*RES kb,I*RES ib,N n,U nb){                       \
  N cnt[8][256];U d,ord[8],np=0;                                               \
  MS(cnt,0,SZ cnt);                                                            \
  for(N i=0;i<n;i++){KT v=ka[i];for(d=0;d<nb;d++)cnt[d][(v>>(8*d))&255]++;}     \
  for(d=0;d<nb;d++)if(cnt[d][(ka[0]>>(8*d))&255]-n)ord[np++]=d;                \
  for(U q=0;q<np;q++){                                                         \
    d=ord[q];                                                                  \
    {N s=0;for(U b=0;b<256;b++){N t=cnt[d][b];cnt[d][b]=s;s+=t;}}               \
    if(q+1==np)                        /* final pass: the keys die with it */  \
      for(N i=0;i<n;i++){N j=cnt[d][(ka[i]>>(8*d))&255]++;ib[j]=ia[i];}        \
    else                                                                       \
      for(N i=0;i<n;i++){KT v=ka[i];N j=cnt[d][(v>>(8*d))&255]++;              \
                         kb[j]=v;ib[j]=ia[i];}                                 \
    {KT*tk=ka;ka=kb;kb=tk;}{I*t=ia;ia=ib;ib=t;}}                               \
  return ia;}
AMRDX(Z,amrdx4,U)
AMRDX( ,amrdx8,W)   // external: src/a.c's multi-column grade drives it too

// Range normalisation. Two independent ways to cut radix passes, decided in ONE
// sequential scan of the keys:
//   (a) a byte column that never varies contributes nothing to the order. The
//       scan ORs together k[i]^k[0], so a zero byte in that accumulator is a
//       constant column -- amrdx skips those itself, for free.
//   (b) translating the keys so the smallest is 0 can shrink the SIGNIFICANT
//       WIDTH below what (a) alone achieves, but only for data that is
//       clustered somewhere other than at zero. Nanosecond timestamps inside
//       one session are the canonical case: they share no leading bytes with
//       each other in a useful way, yet span barely 2^47 -- six passes rather
//       than eight. A dense id column based at 1,000,000: one pass, not three.
// (b) costs a full read+write of the key array, so it is only taken when it
// strictly beats (a). Subtracting a value every key is >= cannot wrap, so the
// translation is order-preserving on the unsigned line, which is all radix
// needs. Returns the significant width, or 0 when every key is identical.
#define AMNORM(NM,KT)                                                          \
Z U NM(KT*RES k,N n,U nb){                                                     \
  KT mn=k[0],mx=k[0],dif=0,f=k[0];                                             \
  for(N i=1;i<n;i++){KT v=k[i];dif|=v^f;if(v<mn)mn=v;if(v>mx)mx=v;}            \
  if(!dif)return 0;                                                            \
  {U base=0;for(U b=0;b<nb;b++)if((dif>>(8*b))&255)base++;                     \
   KT sp=mx-mn;U w=0;while(sp){w++;sp>>=8;}                                    \
   if(w>=base)return nb;                     /* skipping alone is as good */   \
   if(mn)for(N i=0;i<n;i++)k[i]-=mn;                                           \
   return w;}}
AMNORM(amnorm4,U)
AMNORM(amnorm8,W)
U amnorm(W*RES k,N n,U nb){return amnorm8(k,n,nb);}

// Order-preserving unsigned keys. Radix sorts bytes as unsigned magnitudes, so
// every signed / IEEE-754 domain has to be folded onto the unsigned line first.
//   integers:  flip the sign bit  (0x80..)  -- two's complement then orders as
//              plain unsigned, which is why the old path needed the extra
//              `x-&/x` pass (a whole materialised vector, and a subtraction
//              that can overflow on a wide range) and this one does not.
//   doubles:   this is amkF below.
// (AMKG/AMKH/AMKI/AMKL live in a.h -- src/a.c's multi-column grade uses them
// too, and one definition of the collation is the only safe number.)
// IEEE-754 doubles: the sign-magnitude layout means the raw bit pattern is
// monotone for positives and REVERSED for negatives, so the standard total-order
// fold is "if the sign bit is set flip every bit, otherwise flip just the sign
// bit". o1() in src/o.c is that same fold written additively (it lands the
// result in the signed-L domain, which asc() then radix-sorted); reproducing it
// here bit-for-bit, and only as a scalar, means the collation of `<` on a float
// vector -- including exactly where 0n and 0w land -- is UNCHANGED, while the
// transformed copy of the whole vector that of1() used to materialise is gone.
// Done in W: the addition is a deliberate wrap on the unsigned line, which is
// defined behaviour, where the L form is signed overflow and UBSan-visible.
W amkF(F f){W b;MC(&b,&f,SZ(F));
 W u=b^((W)((L)b>>63)>>1);
 u+=(W)((-1ull>>12)-1);
 return u^0x8000000000000000ull;}

// Extract keys and the identity permutation in one sequential pass, and notice
// en route whether the column is ALREADY non-decreasing -- the common case for
// a time column, a `s-attributed column, or a table coming out of ajord(). An
// already-ordered vector then costs exactly one pass and zero permutes.
#define RDXK(KT,T,EXPR)                                                        \
 {CO T*RES p=_V(x);KT*RES k=(KT*)kA;KT pv=0;                                    \
  for(N i=0;i<n;i++){KT v=(KT)(EXPR);k[i]=v;iA[i]=(I)i;if(i&&v<pv)srt=0;pv=v;}}

// Ascending grade of a flat numeric vector. Borrows x; returns a tI index
// vector, or 0 when the type is not one this kernel handles or the arena could
// not supply scratch -- in both cases the caller falls back.
A rdxg(A x){
 UC t=_t(x);N n=_n(x);U nb;
 switch(t){
  case tG: case tC: nb=1;break;
  case tH: nb=2;break;
  case tI: nb=4;break;
  case tL: case tF: nb=8;break;
  default: return 0;}
 A y=aI((U)n);I*RES o=(I*)_V(y);
 if(n<2){if(n)o[0]=0;return y;}
 ArenaMark mk=arena_mark();
 N kw=nb<=4?4u:8u;
 V*kA=arena_alloc(n*kw),*kB=arena_alloc(n*kw);
 I*RES iA=(I*)arena_alloc(n*SZ(I));I*iB=(I*)arena_alloc(n*SZ(I));
 if(!kA||!kB||!iA||!iB){arena_release(mk);mr(y);return 0;}
 B srt=1;
 switch(t){
  case tG: case tC: RDXK(U,G,AMKG(p[i])) break;
  case tH: RDXK(U,H,AMKH(p[i])) break;
  case tI: RDXK(U,I,AMKI(p[i])) break;
  case tL: RDXK(W,L,AMKL(p[i])) break;
  default: RDXK(W,F,amkF(p[i])) break;}
 if(srt){for(N i=0;i<n;i++)o[i]=(I)i;arena_release(mk);return y;}
 nb=nb<=4?amnorm4((U*)kA,n,nb):amnorm8((W*)kA,n,nb);
 if(!nb){for(N i=0;i<n;i++)o[i]=(I)i;arena_release(mk);return y;}  // all equal
 I*r=nb<=4?amrdx4((U*)kA,iA,(U*)kB,iB,n,nb)
          :amrdx8((W*)kA,iA,(W*)kB,iB,n,nb);
 MC(o,r,n*SZ(I));
 arena_release(mk);
 return y;}
