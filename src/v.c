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
