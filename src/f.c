#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
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
   My(F(n,L v=RD(wy,b,i);W j=((W)v*GOLD)>>sh;L k=NL;
     W(1,B(hv[j]<0,)B(hk[j]==v,k=hv[j])j=(j+1)&msk)r[i]=k))
   free(hk);free(hv);)
 z)

Z A1(fN,A y=_R(cn[tl]);x(xtt?y:rsz(xN,y)))

Z AM_TLS L t[256];// per-call char/byte find scratch; thread-local for peach workers
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
