#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
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
 // One combined directive: `omp simd` must be immediately followed by the loop,
 // so vectorisation and threading cannot be stacked as two separate pragmas.
 // Variadic, because the four-way kernels below reduce over a LIST of partials
 // (reduction(*:a,b,c,d)): a fixed two-parameter macro cannot carry that, the
 // commas inside the clause get read as extra macro arguments.
 #define AMPFOR(...) AMPRAGMA(omp parallel for simd reduction(__VA_ARGS__) schedule(static))
#else
 #define AMPFOR(...)
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
#define AMRED(n,body,...) I((n)>=AMPARN,AMPFOR(__VA_ARGS__) body)E(body)
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
  L addfB(CO V*a,U n)_(CO W*p=a;U r=0;F(n>>6,r+=PC(*p++))n&=63;n?r+PC(*p&~(-1ll<<n)):r)
Z L addfG(CO V*a,U n)_(CO G*RES p=a;L r=0;AMRED(n,for(U i=0;i<n;i++)r+=p[i];,+:r)r)
Z L addfH(CO V*a,U n)_(CO H*RES p=a;L r=0;AMRED(n,for(U i=0;i<n;i++)r+=p[i];,+:r)r)
Z L addfI(CO V*a,U n)_(CO I*RES p=a;L r=0;AMRED(n,for(U i=0;i<n;i++)r+=p[i];,+:r)r)
Z L addfL(CO V*a,U n)_(CO L*RES p=a;L r=0;AMRED(n,for(U i=0;i<n;i++)r+=p[i];,+:r)r)
Z L mulfG(CO V*a,U n)_(MUL4(G))
Z L mulfH(CO V*a,U n)_(MUL4(H))
Z L mulfI(CO V*a,U n)_(MUL4(I))
Z L mulfL(CO V*a,U n)_(MUL4(L))
Z L minfG(CO V*a,U n)_(MNM1(G,(G)((1u  << 7)-1),MIN,min))
Z L minfH(CO V*a,U n)_(MNM1(H,(H)((1u  <<15)-1),MIN,min))
Z L minfI(CO V*a,U n)_(MNM1(I,(I)((1u  <<31)-1),MIN,min))
Z L minfL(CO V*a,U n)_(MNM4(L,(L)((1ull<<63)-1),MIN,min))
Z L maxfG(CO V*a,U n)_(MNM1(G,(G)(1u  << 7),MAX,max))
Z L maxfH(CO V*a,U n)_(MNM1(H,(H)(1u  <<15),MAX,max))
Z L maxfI(CO V*a,U n)_(MNM1(I,(I)(1u  <<31),MAX,max))
Z L maxfL(CO V*a,U n)_(MNM4(L,(L)(1ull<<63),MAX,max))
  L addfZ(L v,A x/*0*/)_(v+    G(&addfG,addfH,addfI,addfL)[xw-3](xV,xn) )
Z L mulfZ(L v,A x/*0*/)_(v*    G(&mulfG,mulfH,mulfI,mulfL)[xw-3](xV,xn) )
  L minfZ(L v,A x/*0*/)_(MIN(v,G(&minfG,minfH,minfI,minfL)[xw-3](xV,xn)))
Z L maxfZ(L v,A x/*0*/)_(MAX(v,G(&maxfG,maxfH,maxfI,maxfL)[xw-3](xV,xn)))
// sumF: four-way partial float sum (see the header note above).
Z F sumF(CO F*RES p,U n)_(F a=0,b=0,c=0,d=0;U m=n&~(U)3;
 for(U i=0;i<m;i+=4){a+=p[i];b+=p[i+1];c+=p[i+2];d+=p[i+3];}
 F r=(a+b)+(c+d);for(U i=m;i<n;i++)r+=p[i];r)
Z A3(admf,/*010*/B i=xv==3;U n=zn;P((y&&ytf)||ztF,F v=y?gf(cF(y)):i;z=cF(zR);CO F*RES q=zV;Mz(I(i,F(n,v*=q[i]))E(v+=sumF(q,n)))af(v))L v=y?gl(y):i;az((i?mulfZ:addfZ)(v,z)))
Z A3(subf,/*010*/y=y?neg(y):zn?mul(ai(-2),ii(z,0)):ai(0);neg(admf(ADD,y,z)))
Z A3(mmmf,/*010*/B i=xv==7;P((y&&ytf)||ztF,y=of1(y?cF(y):aV(tf,1,A((L)((W)i<<63)|WFL)));z=of1(cF(zR));of0(N(z(mmmf(x,y,z)))))L v=y?gl(y):i?-WL:WL;az(zn?(i?maxfZ:minfZ)(v,z):v))
A3(arf,/*010*/Q(xtv)Q(xv<11)Q(!y||ytzfc)Q(ztZFC)
 ZE(P(ztE&&x==ADD&&!y,L i=*zL,j=zL[1];az((j-i)*(j+i-1)/2))z=gZ(zR);z(arf(x,y,z)))
 ZB(z=cG(zR);z(arf(x,y,z)))
 G(&dexf,admf,subf,admf,___f,___f,mmmf,mmmf,___f,___f,___f)[xv](x,y,z))

Z A3(___s,/*010*/U i=!y;A u;I(i,y=ii(z,0);u=enl(yR))E(yR;u=emp(tG))U n=zn;W(i<n,y=y(x2(y,ii(z,i++)));P(!y,u(0))PSH(u,yR))y(u))
Z A3(dexs,/*010*/I(y,y(0))zR)
Z A3(adms,/*010*/L w=y?gl(y):x==MUL;U n=zn;I b=1;L v=w;C t=tG+zw-3;A u=an(n,t);
 I(x==ADD,F4(zw-3,n,ug=v+=zg;B(v-(G)v,b=0),uh=v+=zh;B(v-(H)v,b=0),ui=v+=zi;B(v-(I)v,b=0),ul=v+=zl))
 E(       F4(zw-3,n,ug=v*=zg;B(v-(G)v,b=0),uh=v*=zh;B(v-(H)v,b=0),ui=v*=zi;B(v-(I)v,b=0),ul=v*=zl))P(b,u)z=ct(t+1,u(zR));z(adms(x,az(w),z)))
Z A3(subs,/*010*/y=neg(y?y:mul(ai(2),ii(z,0)));neg(adms(ADD,y,z)))
Z A3(mxms,/*010*/P((!y||ytz)&&ztZ,L v=y?gl(y):-WL,l=(L)(~0ull<<((1<<zw)-1)),h=~l;U n=zn;I(v<=l||h<=v,P(v>=0,rsz(n,az(v)))v=v<0?l:h)
                                  A u=an(n,zt);F4(zw-3,n,ug=v=MAX(v,zg),uh=v=MAX(v,zh),ui=v=MAX(v,zi),ul=v=MAX(v,zl))u)___s(x,y,z))
Z A3(mnms,/*010*/P((!y||ytz)&&ztZ,z=inv(zR);z(inv(mxms(MXM,y?az(~gl(y)):0,z))))___s(x,y,z))
Z A3(eqls,/*010*/U n=zn,i=!y;L v=gl(y?y:io(z,0)),a=v;A u=aG(n);S4(zw-3,W(i<n,ug=v=v==zg;i++),W(i<n,ug=v=v==zh;i++),W(i<n,ug=v=v==zi;i++),W(i<n,ug=v=v==zl;i++))y||!n?u:a4(u,ai(0),av,az(a)))
A3(ars,/*010*/Q(xtv)Q(xv<11)Q(!y||ytzc)Q(ztZC)
 ZE(z=gZ(zR);z(ars(x,y,z)))
 ZB(z=cG(zR);z(ars(x,y,z)))
 G(&dexs,adms,subs,adms,___s,___s,mnms,mxms,___s,___s,eqls)[xv](x,y,z))

Z A3(dexp,/*010*/zn?cat11(y?y:_R(cn[zt]),drp(-1,zR)):y(zR))
Z A3(___p,/*010*/v2[xv](z,dexp(av,y,z)))
Z A3(modp,/*010*/e2f(mod,z,dexp(av,y,z)))
Z A3(mxmp,/*010*/U w=zw-3;L v=gl(y),l=-1ll<<(8<<w)-1,h=~l;v=MAX(v,l);N n=zn;P(v>=h,rsz(n,az(v)))y=an(zn,tG+zw-3);S4(w,zG[-1]=v,zH[-1]=v,zI[-1]=v,zL[-1]=v)N j=n-1;
 F4(w,n,yG[j]=MAX(zG[j],zG[j-1]);j--,yH[j]=MAX(zH[j],zH[j-1]);j--,yI[j]=MAX(zI[j],zI[j-1]);j--,yL[j]=MAX(zL[j],zL[j-1]);j--)zn=n;y)
Z A3(mnmp,/*010*/y=az(~gl(y));z=inv(zR);z(inv(mxmp(MXM,y,z))))
Z A3(cmpp,/*010*/I o=x-LTN,w=zw-3;U n=zn;A u=aG(n);L v=gl(y),p=iw(z,w,0);*uG=!o?p<v:o==1?p>v:p==v;L m=n-1,j=m;
 S4(o,F4(w,m,uG[j]=zG[j]< zG[j-1];j--,uG[j]=zH[j]< zH[j-1];j--,uG[j]=zI[j]< zI[j-1];j--,uG[j]=zL[j]< zL[j-1];j--),
      F4(w,m,uG[j]=zG[j]> zG[j-1];j--,uG[j]=zH[j]> zH[j-1];j--,uG[j]=zI[j]> zI[j-1];j--,uG[j]=zL[j]> zL[j-1];j--),
      F4(w,m,uG[j]=zG[j]==zG[j-1];j--,uG[j]=zH[j]==zH[j-1];j--,uG[j]=zI[j]==zI[j-1];j--,uG[j]=zL[j]==zL[j-1];j--),)u)
A3(arp,/*010*/Q(xtv)Q(xv<11)Q(ytzc)Q(ztZC)
 ZE(z=gZ(zR);z(arp(x,y,z)))
 ZB(z=cG(zR);z(arp(x,y,z)))
 G(&dexp,___p,___p,___p,___p,modp,mnmp,mxmp,cmpp,cmpp,cmpp)[xv](x,y,z))

Z C tZx(A x)_(C t=TX[xt];t?t:tZ(gl_(x)))
C sup(A*p,A*q)_(A x=*p,y=*q;C t=MAX(tZx(x),tZx(y));*p=x=Ny(ct(t,x));*q=y=Nx(ct(t,y));t)
Z A4(dexa,/*1000*/uR;Ny(sup(&x,&u));x=mut(x);U n=yn;I wx=xw-3,wy=yw-3,wu=utt?-1:uw-3;L v=wu<0?gl_(u):0;
  Mu(I(utt,F4(wx,n,xG[iw(y,wy,i)]=v ,xH[iw(y,wy,i)]=v ,xI[iw(y,wy,i)]=v ,xL[iw(y,wy,i)]=v ))
     E(    F4(wx,n,xG[iw(y,wy,i)]=ug,xH[iw(y,wy,i)]=uh,xI[iw(y,wy,i)]=ui,xL[iw(y,wy,i)]=ul)))x)
Z A4(adma,/*1000*/yR;uR;x=cL(x);u=cL(u);x=mut(x);I(!ytL,y=cI(y))U n=yn;
 I(utt,L v=gl(u);My(I(zv==1,I(ytL,F(n,xL[yl]+=v ))E(F(n,xL[yi]+=v )))E(I(ytL,F(n,xL[yl]*=v ))E(F(n,xL[yi]*=v )))))
 E(Mu(           My(I(zv==1,I(ytL,F(n,xL[yl]+=ul))E(F(n,xL[yi]+=ul)))E(I(ytL,F(n,xL[yl]*=ul))E(F(n,xL[yi]*=ul))))))x)
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
