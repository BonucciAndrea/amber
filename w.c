#include"a.h" // ngn/k, (c) 2019-2024 ngn, GNU AGPLv3 - https://codeberg.org/ngn/k/raw/branch/master/LICENSE
Z A2(dec,/*01*/yN?K2("0{z+x*y}/",x,y):K1("0^*:",y))
Z X2(enc,/*01*/Ril(K2("{$[&/~*x:(x|-x)!|$[x>0;(-x)!;-x!]\\y;1_x;@[x;0;-:0<]]}",x,y))REBGHIL(K2("{(x|-x)!'|(,y),y{$[y<0;-y!;(-y)!]x}\\-1_|x}",x,y))R_(en(y)))
Z A scC(C c    ,C*p,U n)_(           A x=emp(tA);C*q;W((q=memchr(p,c,n  )),PSH(x,aCm(p,q));n-=q-p+1;p=q+1)I(n||c-10&&xn,PSH(x,aCn(p,n)))x)
Z A sCC(C*s,L m,C*p,U n)_(P(!m,el0())A x=emp(tA);C*q;W((q=memmem(p,n,s,m)),PSH(x,aCm(p,q));n-=q+m-p;p=q+m)I(n||      xn,PSH(x,aCn(p,n)))x)
Z A sc(C c    ,A x)_(XC(x(scC(c,  xV,xn)))et(x))A1(spl,sc(10,x))
Z A sC(C*s,L m,A x)_(XC(x(sCC(s,m,xV,xn)))et(x))
Z L jN(U m,A x/*0*/)_(P(!xtA,-1)L n=(xn-!!xn)*m;F(xn,A y=xa;P(!ytcC,-1)n+=yN)n)//total length or -1
A jc(C c,    A x)_(XC(jc(c,  flp(flp(x))))L n=jN(1,x);P(n<0,et(x))A y=aC(n);C*p=yV;F(xn,I(i,*p++=c        )A z=xa;I(ztc,*p++=zv)E(MC(p,zV,zn);p+=zn))x(y))
A jC(S s,U m,A x)_(XC(jC(s,m,flp(flp(x))))L n=jN(m,x);P(n<0,et(x))A y=aC(n);C*p=yV;F(xn,I(i,MC(p,s,m);p+=m)A z=xa;I(ztc,*p++=zv)E(MC(p,zV,zn);p+=zn))x(y))
Z A1(re0,x?rs0(enl(x)):0) Z A o1f(A1 f,A x/*f1*/)_(re0(f==whr?x(aI(0)):f(fir(x)))) Z A2(o1,/*01*/ xtu?o1f(v1[xv],y):y(emp(tA)))
                          Z A o2f(A2 f,A x,A y/*f01*/)_(re0(f(x,y)))               Z A3(o2,/*001*/xtv?o2f(v2[xv],y,z):z(emp(tA)))
Z AX(em,/*01..1*/A z=0;U k=0;F(n,A y=a[i];I(ytm,k++;y=yx;I(z,P(TS[yt]-TS[zt],ed8(a,n));z=cat10(z,y))E(z=yR)))z=unq(z);
 Ab8;MC(b,a,64);F(n,A y=b[i];I(ytm,A u=kv(&y);PSH(u,ie(x,u));b[i]=u(u1(y(fil(ai(yn),fnd(y,zR)))))))AX e8;am(z,Nz(e8(x,b,n))))
A e1f(A1 f,A x){X(Rt(f(x))Rm(A y=kv(&x);am(x,Nx(e1f(f,y))))RA(U n=xn;P(!n,o1f(f,x))x=mut(x);F(n,P(!(xa=f(xa)),xa=au;x(0)))sqz(x))
 RE(Lij x(0);L n=j-i,i0=i;P(!n,o1f(f,x))A y=aA(n);F(n,P(!(ya=f(ai(i0+i))),mrn(i,yA);0))sqz(y))
 RGHIL(U n=xn;P(!n,o1f(f,x))A y=aA(n);
  S4(xt-tG,F(n,P(!(ya=f(ai(xg))),mrn(i,yA);x(0))),F(n,P(!(ya=f(ai(xh))),mrn(i,yA);x(0))),F(n,P(!(ya=f(ai(xi))),mrn(i,yA);x(0))),F(n,P(!(ya=f(az(xl))),mrn(i,yA);x(0))))sqz(x(y)))
 R_(U n=xN;P(!n,o1f(f,x))A y=aA0(n);F(n,A z=f(ii(x,i));B(!z,y=y(0))PSH(y,z))x(y)))}
Z A2(e1,/*01*/X(Ru(e1f(v1[xv],y)))Y(Rt(x1(y))Rm(A z=kv(&y);am(y,Ny(e1(x,z))))RA(U n=yn;P(!n,o1(x,y))y=mut(y);F(n,;P(!(ya=x1(ya)),ya=au;y(0)))sqz(y))
                                    R_(U n=yN;P(!n,o1(x,y))A u=aA0(n);F(n,A z=x1(ii(y,i));B(!z,u=u(0))PSH(u,z))y(u)))0)
A l2f(A2 f,A x,A y/*f01*/){X(Rt(f(x,y))Rm(A z=N(l2f(f,xy,y));       am(_R(xx),z) )R_(U n=xN;P(!n,x=fir(xR);x(o2f(f,x,y)))A u=aA0(n);F(n,A w=ii(x,i),v=f(w,yR);mr(w); B(!v,u=u(0))PSH(u,v))y(u)))}
A r2f(A2 f,A x,A y/*f01*/){Y(Rt(f(x,y))Rm(A z=Ny(r2f(f,x,_R(yy)));y(am(_R(yx),z)))R_(U n=yN;P(!n,        o2f(f,x,fir(y)))A u=aA0(n);F(n,A v=f(x,ii(y,i));            B(!v,u=u(0))PSH(u,v))y(u)))}
Z A3(l2,/*001*/Xv(l2f(v2[xv],y,z))Yt(x2(y,z))Ym(x=prj(x,A8((A)GAP,z),2);x(e1(x,yR)))  U n=yN;P(!n,y=fir(yR); y(o2(x,y,z)))A u=aA0(n);F(n,A w=ii(y,i),v=x2(w,zR);mr(w);B(!v,u=u(0))PSH(u,v))z(u))
  A3(r2,/*001*/Xv(r2f(v2[xv],y,z))Zt(x2(y,z))Zm(x=prj(x,A8(yR,GAP),2);x(e1(x,z)))     U n=zN;P(!n,         o2(x,y,fir(z)))A u=aA0(n);F(n,A v=x2(y,ii(z,i));           B(!v,u=u(0))PSH(u,v))z(u))
Z AX(l8,/*01..1*/Ab8;MC(b,a,64);*b=GAP;x=prj(x,b,n);x(e1(x,*a)))
  A3(e2,/*001*/Xv(e2f(v2[xv],y,z))Yt(r2(x,y,z))Zt(l2(x,y,z))P(ytm||ztm,em(x,A8(yR,z),2))U m=yN;P(m-zN,el(z))P(!m,z(xtv&&xv<11?yR:emp(tA)))A u=0;C t=ztA&&MINE(z);
   F(m,A w=ii(y,i),v=x2(w,t?za:ii(z,i));mr(w);B(!v,I(u,u=u(0))I(t,mrn(m-i-1,zA+i+1)))I(!u,u=!v?0:LH(ti,_t(v),ts)?AN(0,an(m,TT[_t(v)])):aA0(m))PSH(u,v))mr(t?AZ(z):z);u)
A e2f(A2 f,A x,A y/*f01*/)_(U k=xtt<<1|ytt;P(k==3,f(x,y))
 P(xtm||ytm,P(xtm>ytm,A z=N(e2f(f,xy,y));am(_R(xx),z))P(xtm<ytm,A z=Ny(e2f(f,x,_R(yy)));y(am(_R(yx),z)))
  A z=unq(cat(xx,_R(yx)));x=x1(zR);y=y(y1(zR));I(f==add||f==sub||f==mul||f==dvd,A u=ai(f==mul||f==dvd);x=fil(u,x);y=fil(u,y))am(z,Nz(x(e2f(f,x,y)))))
 P(!k&&xN-yN,el(y))U n=k<2?xN:yN;P(!n,x=fir(xR);x(o2f(f,x,fir(y))))A z=emp(tA);F(n,A v=ii(x,i);A u=f(v,ii(y,i));mr(v);B(!u,z=z(0))PSH(z,u))y(z))
AX(e8,/*01..1*/P(n==1,e1(x,*a))P(n==2,A y=*a;y(e2(x,y,a[1])))Ab8;C t[8];L m=-1;F(n,A y=b[i]=a[i];Ym(em(x,a,n))t[i]=ytP?0:ytt?1:ytA?2+!MINE(y):4;I(t[i]>1,L l=yN;P(m>=0&&m-l,el8(a,n))m=l))
 P(m<0,x8(a,n))F(n,I(t[i]==1,_r(a[i])+=m))A u=0;I(!m,u=x==LEN?emp(tG):n==2&&xtv&&xv<11?_R(a[!_N(a[1])]):emp(tA))//t[i] 0:pkdatm,1:refatm,2:tA(r=1),3:tA,4:other
 Fj(m,F(n,A y=a[i];I(t[i]==2,b[i]=yA[j])I(t[i]>2,b[i]=ii(y,j)))A z=x8(b,n);B(!z,I(u,u=u(0))F(n,A y=a[i];I(t[i]==1,yr-=m-j-1)I(t[i]==2,mrn(m-j-1,yA+j+1))))I(!j,u=LH(ti,zt,ts)?AN(0,an(m,TT[zt])):emp(tA))PSH(u,z))
 F(n,mr(t[i]-2?a[i]:AZ(a[i])))u)

Z A2(cs,/*01*/A z,v=yR,u=enl(yR);W(1,z=yR;y=x1(y);P(!y,mr(v);z(u(0)))B m=mtc_(y,z)||mtc_(y,v);z(0);B(m)PSH(u,yR))mr(v);y(u))
Z A2(cf,/*01*/A z=yR,u;W(1,zR;u=x1(z);B(!u)P(mtc_(u,y)||mtc_(u,z),y(u(z)))z=z(u))y(z(u)))
Z A ns(A x,L m,A y/*0m1*/)_(A z=aA0(m+1);F(m,PSH(z,yR);y=x1(y);B(!y))y?PSH(z,y):z(y))
Z A nf(A x,L m,A y/*0m1*/)_(F(m,y=N(x1(y)))y)
Z A3(ws,/*001*/A u=enl(zR);W(1,A w=y1(zR);B(!w,u=u(0))B(!tru(w))z=x1(z);P(!z,u(0))PSH(u,zR))z(u))
Z A3(wf,/*001*/A u=0;W(1,A w=y1(zR);B(!w,z=z(0))B(!tru(w))z=x1(z);P(!z,0))z)
Z A nS(A x,L m,CO A*a,U n/*0m1n*/)_(P(n==1,ns(x,m,*a))P(m<0,mrn(n,a);ed0())P(m<n,mrn(n-m-1,a+m+1);sqz(aV(tA,m+1,a)))
 A z=aA(m+n),*b=zA;zn=n;MC(zA,a,64);F(m+1-n,mRn(n,b);b[n]=Nz(x8(b,n));zn++;b++)sqz(z))
Z A wS(A x,A y,CO A*a,U n/*001n*/)_(P(n==1,ws(x,y,*a))Ab8;MC(b,a,64);mRn(n,b);A z=sqz(aV(tA,n,b));
 W(1,A u=y1(ii(z,zn-1));B(!u,z=z(0))B(!tru(u))mRn(n-1,b+1);u=x8(b,n);B(!u,*b=au;z=z(0))memmove(b,b+1,n-1<<3);b[n-1]=u;PSH(z,uR))mrn(n,b);z)
Z A nF(A x,L m,CO A*a,U n/*0m1n*/)_(P(n==1,nf(x,m,*a))las(N(nS(x,m,a,n))))
Z A wF(A x,A y,CO A*a,U n/*001n*/)_(P(n==1,wf(x,y,*a))las(N(wS(x,y,a,n))))
Z A3(ls2,/*010*/Y(Ril(ns(x,gl(y),zR))RU(y(ws(x,y,zR)))R_(et(y)))0)
Z A3(lf2,/*010*/Y(Ril(nf(x,gl(y),zR))RU(y(wf(x,y,zR)))R_(et(y)))0)
Z AX(ls8,/*01..1*/A y=*a;P(n==2,A z=a[1];z(ls2(x,y,z)))Y(Ril(nS(x,gl(y),a+1,n-1))RU(y(wS(x,y,a+1,n-1))))et8(a,n))
Z AX(lf8,/*01..1*/A y=*a;P(n==2,A z=a[1];z(lf2(x,y,z)))Y(Ril(nF(x,gl(y),a+1,n-1))RU(y(wF(x,y,a+1,n-1))))et8(a,n))
X1(raz,RA(U n=0;F(xn,n+=_N(xa))A y=xx;y=ytT&&!ytA?AN(0,an(n,ytE?tG:yt)):aA0(n);F(xn,y=Nx(cat10(y,xa)))x(y))Rm(raz(val(x)))R_(x))
Z A2(f1,/*01*/Yt(y)P(xtv&&xv<11&&ytZFC,y(arf(x,0,y)))P(x==CAT,raz(y))P(!yN,y(ie(x,y)))A z=ii(y,0);F(yN-1,z=z(x2(z,ii(y,i+1)));B(!z))y(z))
Z A3(f2,/*010*/Zt(y(x2(y,zR)))P(xtv&&xv<11&&ytzfc&&ztZFC,arf(x,y,z))P(x==CAT,raz(N(cat10(enl(y),z))))P(xto||xtp,F(zN,y=N(x8(A8(y,ii(z,i)),2)))y)F(zN,y=y(x2(y,ii(z,i)));B(!y))y)
L cfm(CO A*a/*0*/,I n)_(L m=-1;F(n,A x=a[i];I(!xtt,U v=xN;P(m>=0&&m-v,-2)m=v))m)
AX(f8,/*01..1*/P(n==1,f1(x,*a))P(n==2,A y=*a,z=a[1];z(f2(x,y,z)))n--;A y=*a++,z=*a;L m=cfm(a,n);P(m==-1,y?x8(a-1,n+1):z)P(m<0,I(y,y(0))el8(a,n))P(!m&&!y,x=ie(x,z);mrn(n,a);x)
 L i=!y;I(i,y=ii(z,0))Ab8;W(i<m,*b=y;Fj(n,b[j+1]=ii(a[j],i))y=x8(b,n+1);B(!y)i++)mrn(n-1,a+1);z(y))
Z A3(s2,/*010*/Zt(y(x2(y,zR)))Zm(A u=N(s2(x,y,zy));am(_R(zx),u))P(!zN,y(zR))P(xtv&&xv<11&&ytzc&&ztZC,ars(x,y,z))A u=aA0(zN);F(zN,y=y(x2(y,ii(z,i)));P(!y,u(0))PSH(u,yR))y(u))
Z A2(s1,/*01*/Yt(y)P(!yN,y)Ym(A z=kv(&y);am(y,Ny(s1(x,z))))P(x==CAT,y(s2(x,emp(tA),y)))P(xtv&&xv<11&&ytZC,y(ars(x,0,y)))
 A z=ii(y,0),u=enl(zR);F(yN-1,z=z(x2(z,ii(y,i+1)));P(!z,y(u(0)))PSH(u,zR))z(y(u)))
Z AX(s8,/*01..1*/A y=*a;P(n==1,s1(x,y))P(n==2,A z=a[1];z(s2(x,y,z)))L m=cfm(a+1,n-1);P(m==-2,el8(a,n))I(m<0,m=1)a++;n--;
 A z=aA0(m);Ab8;F(m,*b=y;Fj(n,b[j+1]=ii(a[j],i))y=x8(b,n+1);P(!y,mrn(n,a);z(0))PSH(z,yR))mrn(n,a);y(z))
Z A3(p2,/*010*/Zt(er(y))Zm(y=N(p2(x,y,zy));am(_R(zx),y))P(!zN,y(zR))P(xtv&&xv<11&&ytzc&&ztZC,arp(x,y,z))A u=aA0(zN);F(zN,A v=ii(z,i),r=x2(v,y);y=v;B(!r,u=u(0))PSH(u,r))y(u))
Z A2(p1,/*01*/y(p2(x,ie(x,y),y)))
Z A stn(A x,L n,A y/*0n0*/)_(P(n<0||n-(I)n,ed0())P(!ytT,et0())YE(y=gZ(yR);y(stn(x,n,y)))L m=MAX(0,yn-n+1);A z=aA0(m);P(!m,mr(zx);zx=mkn(rsz(n,fir(yR)));z)F(m,PSH(z,Nz(x1(slc(y,i,i+n)))))z)
Z A3(ste,/*010*/Yz(stn(x,gl(y),z))et(y))
Z A win(L n,A x)_(x(stn(au,n,x)))
Z AX(cas,/*01..1*/Q(xtZ)K2("{(++y)[x]@'!#x}",x,aV(tA,n,a)))
A w1(U i,A x,A y/*01*/){S(i,
 R(0,X(Rt(e1(x,y))R_(bin(x,y)))0)
 R(1,X(RfF(dec(x,y))RilEBGHIL(dec(x,y))Rc(jc(xv,y))RC(jC(xV,xn,y))R_((xK<2?cf:f1)(x,y)))0)
 R(2,X(RfF(en(y))   RilEBGHIL(enc(x,y))Rc(sc(xv,y))RC(sC(xV,xn,y))R_((xK<2?cs:s1)(x,y)))0)
 R(3,X(Ril(win(gl_(x),y))R_(p1(x,y)))0)
 R(4,er(y))
 R_(e1(x,y)))}
A w2(U i,A x,A y,A z/*010*/){S(i,
 R(0,X(REBGHIL(cas(x,A8(y,zR),2))R_(y(e2(x,y,zR))))0)
 R(1,(xK<2?lf2:f2)(x,y,z))
 R(2,(xK<2?ls2:s2)(x,y,z))
 R(3,(xK==1?ste:p2)(x,y,z))
 R(4,y(r2(x,y,zR)))
 R_(y(l2(x,y,zR))))}
A w8(U i,A x,CO A*a,U n/*0,1..1*/){A y=*a;P(n==1,w1(i,x,y))P(n==2,A z=a[1];z(w2(i,x,y,z)))S(i,
 R(0,X(REBGHIL(cas(x,a,n))R_(e8(x,a,n)))0)
 R(1,xK<2||n-xK==1?lf8(x,a,n):f8(x,a,n))
 R(2,xK<2||n-xK==1?ls8(x,a,n):s8(x,a,n))
 R(3,er8(a,n))
 R(4,er8(a,n))
 R_(l8(x,a,n)))}
