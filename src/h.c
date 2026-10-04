#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
A1(rs0,rsz(0,x))
ZN A flt(A x,A y,B b/*01b*/)_(P(xK-1,er(y))Ym(K("{(!y)[i]!(.y)i:&z~/:x@.y}",xR,y,ai(b)))
 x=Ny(x1(yR));x=xN?Ny(cL(x)):x(emp(tG));P(!xtt&&xN-yN,el(x(y)))A z=rs0(yR);F(yN,L n=gl(ii(x,i));B(b&&n-(U)n,z=ed(z))Fj(b?n:!n,PSH(z,ii(y,i))))x(y(z)))
V cyc(V*a,U m,U n){Q(m);W(2*m<=n,MC(a+m,a,m);m*=2)I(n>m,MC(a+m,a,n-m))}
Z V cpyB(W*x,U j,CO W*y,U k,U n) {P(!n)x+=j>>6;y+=k>>6;j&=63;k&=63; // x[j..j+n] = y[k..k+n] (bits; from upstream ngn/k fe213831..78383dd3)
 I(j,W a=*y>>k;I(k&&n>64-k,a|=y[1]<<64-k);*x=(*x&(1ULL<<j)-1)|a<<j;P(n<=64-j)++x;k+=64-j;y+=k>>6;k&=63;n-=64-j) // align x
 I(!k,MC(x,y,n+7>>3))E(W a=*y++>>k,b;F(n>>6,b=*y++;*x++=a|b<<64-k;a=b>>k);I(n&63,b=(n&63)>64-k?*y:0;*x++=a|b<<64-k))}
Z V cycB(W*x,CO W*y,U k,U m,U n){ // cycle n bits from y[0..m] starting at k
 cpyB(x,0  ,y,k,MIN(m-k,n));P(n<=m-k);n-=m-k;
 cpyB(x,m-k,y,0,MIN(k  ,n));P(n<=k  );n-=k;
 W(n>m,cpyB(x,m,x,0,m);n-=m;m*=2)cpyB(x,m,x,0,n);}
A rsz(L n,A x/*1*/)_(
 X(Rt(rsz(n,enl(x)))
   RM(A y=kv(&x),z=az(n);aM(x,Nx(z(r2(RSH,z,y)))))
   Rm(A y=kv(&x);x=Ny(rsz(n,x));y=Nx(rsz(n,y));am(x,y))
   RE(Lij P(n>j-i||n<i-j,rsz(n,gZ(x)))x(0);n>=0?aE(i,i+n):aE(j+n,j))
   R_(P(n==NL,x)P(!xn,rsz(n,enl(fir(x))))
      I r=n<0;n*=1-2*r;P((W)n-(U)n,ez(x))A y=an(n,xt);N w=MAX(0,xw-3),m=xn<<w,k=n%xn<<w,l=n<<w;
      XB(cycB(yV,xV,r?m-k:0,m,l);x(y))                                        //bits: cycled bit by bit (it was 'nyi)
      I(!r,MC(yV,xV,MIN(m,l)))J(l<=m,MC(yV,xV+m-l,l))E(MC(yV,xV+m-k,k);MC(yV+k,xV,m-k))
      cyc(yV,m,l);I(!n&&ytA,yx=mkn(_R(xx)))x(ytA?sqz(mRa(y)):y)))0)
A slc(A x/*0*/,U i,U j)_(Q(xtT&&i<=j&&j<=xN)N n=j-i;XB(A y=an(j-i,tB);cpyB(yV,0,xV,i,j-i);y)XE(I v=*xL;aE(v+i,v+j))A y=an(n,xt);U w=xw-3;MC(yV,xV+((W)i<<w),(W)n<<w);XA(P(!n,yx=mkn(_R(xx));y)sqz(mRa(y)))y)
Z A chp(L n,A x/*1*/)_(P(n<0,ed(x))XmM(en(x))L m=(xn+n-1)/n;A y=aA(m);F(m|!m,ya=slc(x,n*i,MIN((L)xn,n*i+n)))x(0);I(!m,yx=mkn(yx))y)
Z A2(rsh,/*01*/XE(x=gZ(xR);x(rsh(x,y)))YE(rsh(x,gZ(y)))YmM(en(y))Yt(rsh(x,enl(y)))Q(xtZ);N r=xn;P(!r,fir(y))P(r>256,ez(y))x=Ny(cL(xR));L s[r];MC(s,xV,r<<3);x(0);
 I(r==2,P(*s==NL,chp(s[1],y))P(s[1]==NL,A u=az(*s);u(K2("{$[(0<x)&~x!#y;(x;(-x)!#y)#y;((-x)!(#y)*!x)_y]}",u,y))))P(r==1&&*s==NL,y)I(!yn,y=enl(fir(y)))
 L m=1;F(r,L d=s[r-1-i];P(d<0,ed(y))P(__builtin_mul_overflow(m,MAX(1ll,d),&m)||m>>32,ez(y)))y=N(rsz(m,y));F(r-1,L d=s[r-1-i];I(d,y=N(chp(MAX(1ll,d),y)))E(y=N(e1f(rs0,N(chp(1,y))))))rsz(*s,y))
X2(hsh,/*01*/Ril(rsz(gl_(x),y))RU(flt(x,y,1))RT(P(ytm,A k=xR,i=fnd(yx,xR);P(!i,mr(k);y(0))P(_tA(yx)&&_n(yx)&&_tt(i),mr(k);mr(i);en(y))A u=i1(yy,i);P(!u,mr(k);y(0))y(aV(yt,2,A(k,u))))P(ytM&&xtS,A k=xR,u=i1(y,xR);P(!u,mr(k);y(0))y(aV(yt,2,A(k,u))))XZ(rsh(x,y))et(y))R_(et(y)))   //dict: i1's lookup, inline; x one rank below list keys is one key, which find gives as an atom: 'nyi (not for (), as in `a`b#()!()); k: x's second reference, taken with the first (one read of the thread-local refcount flag)
A drp(L n,A x/*1*/)_(X(Rm(A y=kv(&x);am(Ny(drp(n,x)),Nx(drp(n,y))))RM(A y=kv(&x);aM(x,Nx(e2f(und,az(n),y))))Rt(er(x))RE(Lij x(0);W d=n<0?0-(W)n:(W)n;d=MIN(d,(W)(j-i));n<0?aE(i,j-(L)d):aE(i+(L)d,j))   //drop at most the count: i+n and j+n could overflow
 R_(P(n==NL,rs0(x))L m=xn;n=MAX(-m,MIN(m,n));P(-n<(W)m&&MINE(x),I(xtA,mrn(-n,xA+m+n);return sqz(AN(m+n,x)))AN(m+n,x))x(slc(x,MAX(0ll,n),m+MIN(0ll,n)))))0)   //in place, a general list is squeezed too, as the copy path does (digest #28)
Z A rmv(A x/*1*/,L i)_(XB(rmv(cG(x),i))X(RT_E(P(i>=(W)xn,x)A y=an(xn-1,xt);U w=xw-3;MC(yV,xV,i<<w);MC(yV+(i<<w),xV+(i+1<<w),xn-i-1<<w);I(xtA,I(!yn,yx=mkn(_R(xx)))y=sqz(mRa(y)))x(y))
 RM(A y=kv(&x);y=Nx(y(l2f(und,y,az(i))));aM(x,y))RE(rmv(gZ(x),i))R_(et(x)))0)
Z A2(cut,/*01*/Q(xtZ)Q(ytMT)K2("{y$[|/0<':x,#y;`err\"domain\";x+!'1_-':x,#y]}",x,y))
A2(und,/*01*/
 Xz(drp(gl_(x),y))
 XU(flt(x,y,0))
 Xm(A z=N(fnd(xx,yR));Zz(y(0);y=Nz(und(xx,zR));z=Ny(und(xy,z));am(y,z))ZZ(z(0);K2("_/",x,y))z(en(y)))
 P(xtZ&&ytMT,cut(x,y))
 P(xtMT&&ytz,rmv(xR,gl(y)))
 Ym(K2("{((!y)^x)#y}",x,y))
 YM(flp(N(und(x,flp(y)))))
 et(y))
X1(enl,R5(ti,tl,tf,tc,ts,x(aV(TT[xt],1,TP(xt)?&x:xV)))Rm(A y=kv(&x);aM(x,e1f(enl,y)))R_(aA1(x)))
A2(cat10,
 XE(cat10(gZ(x),y))
 YE(y=gZ(yR);y(cat10(x,y)))
 P(xtB&&ytB,U m=xn,n=yn,d=m&63;x=aa(m+n,x);P(!d,MC(xV+(m>>3),yV,n+63>>6<<3);x)
  L*a=xL+(m>>6),*b=yL,v=*a&~(~0ull<<d);F(n+63>>6,v|=(W)*b<<d;*a++=v;v=(W)*b++>>64-d;)*a=v;x)
 P(xtT&&ytT,P(!yn,x)P(!xn,x(yR))P(xt-yt,P(xtZ&&ytZ,yR;N(sup(&x,&y));cat11(x,y))cat11(blw(x),blw(yR)))P(xtB||ytB,en(x))
  U m=xn,n=yn,w=xw-3;x=aa(m+n,x);
  MC(xV+((W)m<<w),yV,(W)n<<w);I(ytA,mRa(y))x)
 P(xtm&&ytm,a4(x,yx,av,yy))
 Xmt(cat10(enl(x),y))
 Ymt(psh(x,yR))
 P(xtM||ytM,P(!yN,x)P(!xN,x(yR))P(xtT||ytT,x=N(blw(x));y=Nx(blw(yR));cat11(x,y))P(!xtM||!ytM,et(x))P(!mtc_(xx,yx),ed(x))A z=e2f(cat,xy,_R(yy));x(z?aM(_R(xx),z):0))Q(0);0)
A2(cat11,y(cat10(x,y)))
A2(cat,/*01*/cat11(xR,y))
A2(psh,/*11*/Q(xtMT);U n=xN;P(!n,enl(x(y)))
 P(xtE,psh(gZ(x),y))   //a range has no room to push into: its items do (sup below keeps it a range, so it looped)
 P(xtG&&yti&&yv==(G)yv||xtC&&ytc,apc(x,yv))
 P(xtH&&yti&&yv==(H)yv ,x=aa(n+1,x);xH[n]=yv;x)
 P(xtI&&yti||xtS&&yts  ,x=aa(n+1,x);xI[n]=yv;x)
 P(xtL&&yti            ,x=aa(n+1,x);xL[n]=yv;x)
 P(xtL&&ytl||xtF&&ytf  ,x=aa(n+1,x);xL[n]=gl(y);x)
 P(xtB&&yti&&yv==(1&yv),x=aa(n+1,x);xG[n>>3]=(UC)(xG[n>>3]&~(1<<(n&7))|yv<<(n&7));x)   //the bit may hold a stale 1 (take and drop leave the bits after the last item)
 P(xtZ&&ytz,N(sup(&x,&y));psh(x,y))
 XM(P(!ytm||!mtc_(xx,yx),psh(Ny(blw(x)),y))x=mut(x);A z=xy=mut(xy);F(zn|!zn,PSH(za,ii(yy,i)))I(!zn,zx=mkn(zx))y(x))
 P(!xtA&&(!ytt||xt-TT[yt]||yt>=tdt),psh(Ny(blw(x)),y))   //a temporal atom keeps its type: there is no temporal vector (TT maps it to its int width)
 L v=xtA?(L)y:gl(y);
 x=aa(n+1,x);U w=xw-3;MC(xV+((W)n<<w),&v,1<<w);x)
A apc(A x/*1*/,C c    )_(Q(xtC||xtG);U n=xn;x=aa(n+1,x);xC[n]=c;x)
A cts(A x/*1*/,S s,U m)_(Q(xtC);     U n=xn;x=aa(n+m,x);MC(xV+n,s,m);x)
Z A insL(A x,L i,L j,A y/*1ij0*/)_(
 P(i>=(W)(j+1)||j>=(W)(xN+1),ei(x))
 A z=an(xn-j+i+yn,xt);U w=xw-3;MC(zV,xV,(W)i<<w);MC(zV+(i<<w),yV,(W)yn<<w);MC(zV+(i+yn<<w),xV+((W)j<<w),(W)(xn-j)<<w);
 I(xtR,I(!zn,zx=emp(tC))I(MINE(x),mrn(j-i,xA+i);AZ(x))E(mRn(i,xA);mRn(xn-j,xA+j))I(MINE(y),AZ(y))E(mRa(y))z=sqz(z))
 x(z))
A3(ins3,/*100*/
 Xmt(et(x))
 Zmt(z=enl(zR);z(ins3(x,y,z)))
 XM(P(!ztM,et(x))P(!mtc_(xx,zx),ed(x))y=prj(QUE,A8((A)GAP,yR,GAP),3);A u=Nx(y(e2(y,xy,_R(zy))));x(aM(_R(xx),u)))
 P(xtZ&&ztZ&&xt-zt,zR;N(sup(&x,&z));z(ins3(x,y,z)))
 P(xt-zt,z=blw(zR);z(ins3(blw(x),y,z)))
 Y(Ril(L i=gl_(y);insL(x,i,i,z))REGHIL(P(yn-2,el(x))insL(x,gl_(ii(y,0)),gl_(ii(y,1)),z))R_(et(x)))0)
AA(ins,/*10..0*/P(n==3,ins3(*a,a[1],a[2]))en(*a))
