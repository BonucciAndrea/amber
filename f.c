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

Z A1(fN,A y=_R(cn[tl]);x(xtt?y:rsz(xN,y)))

Z L t[256];
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
   B srt=!_tP(x)&&xt!=tF&&xt!=tS&&_at(x)==1;TY(fGL)*f=(srt?G(&bGL,bHL,bIL,bLL):G(&fGL,fHL,fIL,fLL))[xw-3];V*a=xV;U m=xn;
   Yt(az(f(a,m,gl(y))))
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
