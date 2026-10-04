#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
Z A2(dec,/*01*/yN?K2("0{z+x*y}/",x,y):K1("0^*:",y))
Z X2(enc,/*01*/Ril(K2("{$[&/~*x:(x|-x)!|$[x>0;(-x)!;-x!]\\y;1_x;@[x;0;-:0<]]}",x,y))REBGHIL(K2("{$[x;(x|-x)!'|(,y),y{$[y<0;-y!;(-y)!]x}\\-1_|x;~^`c`C?@y;`err\"type\";0#(,10)\\y]}",x,y))R_(en(y)))
Z A scC(C c    ,C*p,U n)_(           A x=emp(tA);C*q;W((q=memchr(p,c,n  )),PSH(x,aCm(p,q));n-=q-p+1;p=q+1)I(n||c-10&&xn,PSH(x,aCn(p,n)))x)
Z A sCC(C*s,L m,C*p,U n)_(P(!m,el0())A x=emp(tA);C*q;W((q=memmem(p,n,s,m)),PSH(x,aCm(p,q));n-=q+m-p;p=q+m)I(n||      xn,PSH(x,aCn(p,n)))x)
Z A sc(C c    ,A x)_(XC(x(scC(c,  xV,xn)))et(x))A1(spl,sc(10,x))
Z A sC(C*s,L m,A x)_(XC(x(sCC(s,m,xV,xn)))et(x))
Z L jN(U m,A x/*0*/)_(P(!xtA,-1)L n=(xn-!!xn)*m;F(xn,A y=xa;P(!ytcC,-1)n+=yN)P(n!=(U)n,-2)n)//total length or -1
A jc(C c,    A x)_(XC(jc(c,  flp(flp(x))))L n=jN(1,x);P(n<-1,ez(x))P(n<0,et(x))A y=aC(n);C*p=yV;F(xn,I(i,*p++=c        )A z=xa;I(ztc,*p++=zv)E(MC(p,zV,zn);p+=zn))x(y))
A jC(S s,U m,A x)_(XC(jC(s,m,flp(flp(x))))L n=jN(m,x);P(n<-1,ez(x))P(n<0,et(x))A y=aC(n);C*p=yV;F(xn,I(i,MC(p,s,m);p+=m)A z=xa;I(ztc,*p++=zv)E(MC(p,zV,zn);p+=zn))x(y))
Z A1(re0,x?rs0(enl(x)):0) Z A o1f(A1 f,A x/*f1*/)_(re0(f==whr?x(aI(0)):f(fir(x)))) Z A2(o1,/*01*/ xtu?o1f(v1[xv],y):y(emp(tA)))
                          Z A o2f(A2 f,A x,A y/*f01*/)_(re0(f(x,y)))               Z A3(o2,/*001*/xtv?o2f(v2[xv],y,z):z(emp(tA)))
Z AX(em,/*01..1*/A z=0;U k=0;F(n,A y=a[i];I(ytm,k++;y=yx;I(z,P(TS[yt]-TS[zt],z(ed8(a,n)));z=cat10(z,y))E(z=yR)))z=unq(z);
 Ab8;MC(b,a,n*SZ(A));F(n,A y=b[i];I(ytm,A u=kv(&y);b[i]=au;PSH(u,ie(x,u));b[i]=u(u1(y(fil(ai(yn),N(fnd(y,zR),mrn(n,b);u(y(z(0))))))))))AX e8;am(z,Nz(e8(x,b,n))))
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
Z AX(l8,/*01..1*/Ab8;MC(b,a,n*SZ(A));*b=GAP;x=prj(x,b,n);x(e1(x,*a)))
  A3(e2,/*001*/Xv(e2f(v2[xv],y,z))Yt(r2(x,y,z))Zt(l2(x,y,z))P(ytm||ztm,em(x,A8(yR,z),2))U m=yN;P(m-zN,el(z))P(!m,z(xtv&&xv<11?yR:emp(tA)))A u=0;C t=ztA&&MINE(z);
   F(m,A w=ii(y,i),v=x2(w,t?za:ii(z,i));mr(w);B(!v,I(u,u=u(0))I(t,mrn(m-i-1,zA+i+1)))I(!u,u=!v?0:LH(ti,_t(v),ts)?AN(0,an(m,TT[_t(v)])):aA0(m))PSH(u,v))mr(t?AZ(z):z);u)
//dicts: keys of different ranks do not merge ('domain), and the union of the keys and looking
//them up can fail ('nyi for table keys): check each
U urnk(A);
// + - * % & | and ,' on two dicts line them up by key: x's keys in order (a repeat kept, as ngn/k and q), then y's
// new ones; a key on both sides gets f, a key on one side passes its value through (+ - * % with their identity,
// so 0-y, 1%y and the types they give). They were looked up in the union of the keys, so a missing key read a null
// shaped like the other dict's first value (| and ,' mangled lists, bytes became ints), an empty dict was 'length
// and a repeated key of x went (digest #23-#26). The same keys: just f on the two value lists
Z A dkey(I c,A2 f,A x,A y/*00f01*/)_(P(!_n(xx)&&!_n(yx),y(_R(x)))P(c<6&&mtc_(xx,yx),A v=f(xy,_R(yy));y(v?am(_R(xx),v):0))
 A v=K("{[c;kx;vx;ky;vy]yo:$[#kx;&^kx?ky;!#ky];iy:$[#ky;ky?kx;(#kx)#0N];iy:$[#kx;@[iy;&~(kx?kx)=!#kx;:;0N];iy];b:&~^iy;xo:&^iy;f:(+;-;*;%;&;|;,)c;u:c>1;r:$[c=6;f'[vx b;vy iy b];f[vx b;vy iy b]];(kx,ky yo)!($[c<4;f[vx xo;u];vx xo],r,$[c<4;f[u;vy yo];vy yo])@<xo,b,(#kx)+!#yo}",
       az(c),_R(xx),_R(xy),_R(yx),_R(yy));y(v))
A e2f(A2 f,A x,A y/*f01*/)_(U k=xtt<<1|ytt;P(k==3,f(x,y))
 P(xtm||ytm,P(xtm>ytm,A z=N(e2f(f,xy,y));am(_R(xx),z))P(xtm<ytm,A z=Ny(e2f(f,x,_R(yy)));y(am(_R(yx),z)))
  P(_n(xx)&&_n(yx)&&urnk(xx)-urnk(yx),ed(y))
  I c=f==add?0:f==sub?1:f==mul?2:f==dvd?3:f==mnm||f==mnu?4:f==mxm||f==mxu?5:f==cat?6:-1;P(c>=0&&!_tM(xx)&&!_tM(yx),dkey(c,f,x,y))
  A z=cat(xx,_R(yx));P(!z,y(0))P(_tM(xx)||_tM(yx),mr(z);y(en0()))   /*keyed tables: not yet (issue #19); keys that do not join were 'domain above*/z=unq(z);P(!z,y(0))B o=(f==add||f==sub||f==mul||f==dvd||f==mnm||f==mnu)&&(_n(xx)||_t(xx)-tA)&&(_n(yx)||_t(yx)-tA)&&!mtc_(xx,yx);A mx=o?fnd(xx,zR):0,my=o?fnd(yx,zR):0;x=x1(zR);P(!x,I(mx,mr(mx))I(my,mr(my))z(y(0)))y=y(y1(zR));P(!y,I(mx,mr(mx))I(my,mr(my))z(x(0)))   //mx my: 0N where a key is missing on that side (none is when the keys match)
  I(o,                                                         //a missing key takes the verb's identity (only a missing one: a null value stays)
   A u=f==mnm||f==mnu?(xtF||ytF?af(WF):az(WL)):ai(f==mul||f==dvd);x=K("{$[|/^z;@[x;&^z;:;y];x]}",x,_R(u),mx);P(!x,mr(u);mr(my);z(y(0)))y=K("{$[|/^z;@[x;&^z;:;y];x]}",y,u,my);P(!y,z(x(0))))   //the identity atom at the missing positions only
  am(z,Nz(x(e2f(f,x,y)))))
 P(!k&&xN-yN,el(y))U n=k<2?xN:yN;P(!n,x=fir(xR);x(o2f(f,x,fir(y))))A z=emp(tA);F(n,A v=ii(x,i);A u=f(v,ii(y,i));mr(v);B(!u,z=z(0))PSH(z,u))y(z))
AX(e8,/*01..1*/P(n==1,e1(x,*a))P(n==2,A y=*a;y(e2(x,y,a[1])))Ab8;C t[8];L m=-1;F(n,A y=b[i]=a[i];Ym(em(x,a,n))t[i]=ytP?0:ytt?1:ytA?2+!MINE(y):4;I(t[i]>1,L l=yN;P(m>=0&&m-l,el8(a,n))m=l))
 P(m<0,x8(a,n))F(n,I(t[i]==1,_r(a[i])+=m))A u=0;I(!m,u=x==LEN?emp(tG):n==2&&xtv&&xv<11?_R(a[!_N(a[1])]):emp(tA))//t[i] 0:pkdatm,1:refatm,2:tA(r=1),3:tA,4:other
 Fj(m,F(n,A y=a[i];I(t[i]==2,b[i]=yA[j])I(t[i]>2,b[i]=ii(y,j)))A z=x8(b,n);B(!z,I(u,u=u(0))F(n,A y=a[i];I(t[i]==1,yr-=m-j-1)I(t[i]==2,mrn(m-j-1,yA+j+1))))I(!j,u=LH(ti,zt,ts)?AN(0,an(m,TT[zt])):emp(tA))PSH(u,z))
 F(n,mr(t[i]-2||!m?a[i]:AZ(a[i])))u)   //an empty list gave none of its items out: released whole (AZ, the container only, leaked an empty list's prototype item, digest #8)

Z A2(cs,/*01*/A z,v=yR,u=enl(yR);W(1,z=yR;y=x1(y);P(!y,mr(v);z(u(0)))B m=mtc_(y,z)||mtc_(y,v);z(0);B(m)PSH(u,yR))mr(v);y(u))
Z A2(cf,/*01*/A z=yR,u;W(1,zR;u=x1(z);B(!u)P(mtc_(u,y)||mtc_(u,z),y(u(z)))z=z(u))y(z(u)))
Z A ns(A x,L m,A y/*0m1*/)_(A z=aA0(m+1);F(m,PSH(z,yR);y=x1(y);B(!y))y?PSH(z,y):z(y))
Z A nf(A x,L m,A y/*0m1*/)_(F(m,y=N(x1(y)))y)
Z A3(ws,/*001*/A u=enl(zR);W(1,A w=y1(zR);B(!w,u=u(0))B(!tru(w))z=x1(z);P(!z,u(0))PSH(u,zR))z(u))
Z A3(wf,/*001*/W(1,A w=y1(zR);B(!w,z=z(0))B(!tru(w))z=x1(z);P(!z,0))z)
Z A nS(A x,L m,CO A*a,U n/*0m1n*/)_(P(n==1,ns(x,m,*a))P(m<0,mrn(n,a);ed0())P(m<n,mrn(n-m-1,a+m+1);sqz(aV(tA,m+1,a)))
 P((W)m-(U)m,ez8(a,n))A z=aA(m+n),*b=zA;zn=n;MC(zA,a,n<<3);F(m+1-n,mRn(n,b);b[n]=Nz(x8(b,n));zn++;b++)sqz(z))
Z A wS(A x,A y,CO A*a,U n/*001n*/)_(P(n==1,ws(x,y,*a))Ab8;MC(b,a,n<<3);mRn(n,b);A z=sqz(aV(tA,n,b));
 W(1,A u=y1(ii(z,zn-1));B(!u,z=z(0))B(!tru(u))mRn(n-1,b+1);u=x8(b,n);B(!u,*b=au;z=z(0))memmove(b,b+1,n-1<<3);b[n-1]=u;PSH(z,uR))mrn(n,b);z)
Z A nF(A x,L m,CO A*a,U n/*0m1n*/)_(P(n==1,nf(x,m,*a))las(N(nS(x,m,a,n))))
Z A wF(A x,A y,CO A*a,U n/*001n*/)_(P(n==1,wf(x,y,*a))las(N(wS(x,y,a,n))))
Z A3(ls2,/*010*/Y(Ril(ns(x,gl(y),zR))RU(y(ws(x,y,zR)))R_(et(y)))0)
Z A3(lf2,/*010*/Y(Ril(nf(x,gl(y),zR))RU(y(wf(x,y,zR)))R_(et(y)))0)
Z AX(ls8,/*01..1*/A y=*a;P(n==2,A z=a[1];z(ls2(x,y,z)))Y(Ril(nS(x,gl(y),a+1,n-1))RU(y(wS(x,y,a+1,n-1))))et8(a,n))
Z AX(lf8,/*01..1*/A y=*a;P(n==2,A z=a[1];z(lf2(x,y,z)))Y(Ril(nF(x,gl(y),a+1,n-1))RU(y(wF(x,y,a+1,n-1))))et8(a,n))
X1(raz,RA(U n=0;F(xn,n+=_N(xa))A y=xx;y=ytT&&!ytA?AN(0,an(n,ytE?tG:yt)):aA0(n);F(xn,y=Nx(cat10(y,xa)))x(y))Rm(raz(val(x)))R_(x))
A ucb(A),cub(A);
#define MMC (xtv&&xv-6<2u)                   //& |: of chars only, unsigned, a char back, as the verb does with two chars (issue #17)
#define CA (xtv&&xv<11&&xv&&xv-5&&xv-8>1u)  //+ - * % & | =: f/ f\ read chars as ints, as the verb does with a number
#define CI(y) (CA&&y##tC)
#define CF(y) (CI(y)&&(yN<2||xv==4||xv==10))  //unseeded: arf already folds 2 or more chars as ints for + - *, so only % = and short lists convert first (& | of chars go by MMC)
#define LGC(c) (xtv&&xv-8<2u&&(c))          //< > with chars and a seed go item by item through the verb (char with char unsigned, with a number signed: issue #17), so y f/z is {x f y}/[y;z] and the last of y f\z; unseeded (and the seed of an empty fold), the chars are read as unsigned ints first
Z A2(f1,/*01*/Yt(y)P(MMC&&ytC,P(!yN,y(ac(xv==6?-1:0)))CO UC*q_=(CO UC*)yV;UC m_=*q_;I(xv==6,F(yn,m_=q_[i]<m_?q_[i]:m_))E(F(yn,m_=q_[i]>m_?q_[i]:m_))y(ac((C)m_)))   /*& | of chars: unsigned, a char back, in one pass*/P(CF(y)||LGC(ytC),f1(x,xv-8<2u?ucb(y):cG(y)))P(xtv&&xv<11&&ytZFC&&!LGC(ytC),y(arf(x,0,y)))P(x==CAT,raz(y))P(!yN,y(ie(x,y)))A z=ii(y,0);F(yN-1,z=z(x2(z,ii(y,i+1)));B(!z))y(z))
Z A3(f2,/*010*/P(MMC&&(ytc||ytC)&&ztC,A w=ucb(zR);A r=f2(x,ucb(y),w);mr(w);cub(r))P(CA&&ztC&&!(MMC&&(ytm||ytA)),A w=cG(zR);A r=f2(x,y,w);mr(w);r)   //the items too, as the seed
 P(!MMC&&(CI(y)||CA&&ytc),f2(x,ytc?ai((C)yv):cG(y),z))P(LGC(ytc||ytC)&&!zN,f2(x,ucb(y),z))Zt(y(x2(y,zR)))P(xtv&&xv<11&&ytzfc&&ztZFC&&!LGC(ytc||ztC)&&!(MMC&&ytc),arf(x,y,z))P(x==CAT,raz(N(cat10(enl(y),z))))P(xto||xtp,F(zN,y=N(x8(A8(y,ii(z,i)),2)))y)F(zN,y=y(x2(y,ii(z,i)));B(!y))y)
L cfm(CO A*a/*0*/,I n)_(L m=-1;F(n,A x=a[i];I(!xtt,U v=xN;P(m>=0&&m-v,-2)m=v))m)
AX(f8,/*01..1*/P(n==1,f1(x,*a))P(n==2,A y=*a,z=a[1];z(f2(x,y,z)))n--;A y=*a++,z=*a;L m=cfm(a,n);P(m==-1,y?x8(a-1,n+1):z)P(m<0,I(y,y(0))el8(a,n))P(!m&&!y,x=ie(x,z);mrn(n,a);x)
 L i=!y;I(i,y=ii(z,0))Ab8;W(i<m,*b=y;Fj(n,b[j+1]=ii(a[j],i))y=x8(b,n+1);B(!y)i++)mrn(n-1,a+1);z(y))
Z A3(s2,/*010*/P(MMC&&(ytc||ytC)&&ztC&&zn,A w=ucb(zR);A r=s2(x,ucb(y),w);mr(w);cub(r))P(CA&&ztC&&zn&&!(MMC&&(ytm||ytA)),A w=cG(zR);A r=s2(x,y,w);mr(w);r)P(!MMC&&(CI(y)||CA&&ytc)&&zN,s2(x,ytc?ai((C)yv):cG(y),z))   //chars as ints, as the fold (an empty scan is the items as they are)
 Zt(y(x2(y,zR)))Zm(A u=N(s2(x,y,zy));am(_R(zx),u))P(!zN,y(zR))P(xtv&&xv<11&&ytzfc&&ztZFC&&!LGC(ytc||ztC),ars(x,y,z))A u=aA0(zN);F(zN,y=y(x2(y,ii(z,i)));P(!y,u(0))PSH(u,yR))y(u))
Z A2(s1,/*01*/Yt(y)P(MMC&&ytC,P(!yN,y)cub(s1(x,ucb(y))))P(CI(y)||LGC(ytC),s1(x,xv-8<2u?ucb(y):cG(y)))P(!yN,y)Ym(A z=kv(&y);am(y,Ny(s1(x,z))))P(x==CAT,y(s2(x,emp(tA),y)))P(xtv&&xv<11&&ytZFC&&!LGC(ytC),y(ars(x,0,y)))
 A z=ii(y,0),u=enl(zR);F(yN-1,z=z(x2(z,ii(y,i+1)));P(!z,y(u(0)))PSH(u,zR))z(y(u)))
Z AX(s8,/*01..1*/A y=*a;P(n==1,s1(x,y))P(n==2,A z=a[1];z(s2(x,y,z)))L m=cfm(a+1,n-1);P(m==-2,el8(a,n))I(m<0,m=1)a++;n--;
 A z=aA0(m);Ab8;F(m,*b=y;Fj(n,b[j+1]=ii(a[j],i))y=x8(b,n+1);P(!y,mrn(n,a);z(0))PSH(z,yR))mrn(n,a);y(z))
Z A3(p2,/*010*/Zt(er(y))Zm(y=N(p2(x,y,zy));am(_R(zx),y))P(!zN,y(zR))P(MMC&&ytc&&ztC,A w=ucb(zR);A r=p2(x,ucb(y),w);mr(w);cub(r))P(xtv&&xv-6<4u&&ztC&&zn>1&&(ytz||ytf),K("{[f;y;z]@[f':[*z;z];0;:;*f':[y;1#z]]}",_R(x),y,_R(z)))   //& | < > of chars with a number seed: the chars among themselves, the first item through the verb
 P(xtv&&xv<11&&ytzc&&ztZC&&!(xv-6<4u&&ztC&&!ytc),arp(x,y,z))
 P(xtv&&(LH(1,xv,4)||LH(6,xv,10))&&(ytf||ytz)&&ztF,arpF(x,y,z))A u=aA0(zN);F(zN,A v=ii(z,i),r=x2(v,y);y=v;B(!r,u=u(0))PSH(u,r))y(u))
// amber 2.3: unseeded |': and &': seed with the first element, so element 0 is x[0] itself
// (an identity seed of -0w/-0W is not below NaN/0N, and `|':0n 1.0` began -0w)
Z A2(p1,/*01*/y(p2(x,(x==MXM||x==MNM)&&!_tP(y)&&_t(y)<tM&&_N(y)?fir(yR):ie(x,y),y)))
Z A stn(A x,L n,A y/*0n0*/)_(P(n<0||n-(I)n,ed0())P(!ytT,et0())YE(y=gZ(yR);y(stn(x,n,y)))L m=MAX(0ll,yn-n+1);A z=aA0(m);P(!m,mr(zx);zx=mkn(rsz(n,fir(yR)));z)F(m,PSH(z,Nz(x1(slc(y,i,i+n)))))z)
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

// ---- `ejx: ej's join kernel, kept here, last in the link ----------------------
// It belongs with find in f.c, but 7 KB of new code there moves every function linked
// after it, and that alone made the timestamp, date and generic grade benchmarks 5-9%
// slower with their code unchanged. w.c is linked last (build.sh links o/*.o in name
// order), so code at its end moves no function but two LTO copies of e2f and the cold
// noupd. Copies of f.c's definitions; keep them in step (AMNF is find's float equality).
V free(V*);
I posix_memalign(V**,N,N);
Z V*amal(N b)_(V*p=0;P(posix_memalign(&p,64,b),(V*)0)p)
#define AMNZ(w) ((W)(w)==0x8000000000000000ull?(L)0:(w))
#define AMNF(w) (((W)(w)<<1)>0xffe0000000000000ull?(L)0x7ff8000000000000ll:AMNZ(w))
#define RD(w,p,i) ((w)==0?(L)((CO G*)(p))[i]:(w)==1?(L)((CO H*)(p))[i]:(w)==2?(L)((CO I*)(p))[i]:((CO L*)(p))[i])
#define LUTDOM  ((W)1<<16)
#define GOLD    0x9E3779B97F4A7C15ull
// `ejx (yk;xk): the rows of an equi-join (amber.k's ej)
// ej wants, for each row of x in order, every row of y with the same key, in y's order.
// In K it found y's keys in themselves to learn whether any repeats, then x's keys in
// y's: two hash builds over y where ij (2.4.1's ej) makes one. Here one build over
// y's keys, backwards so a key's slot ends at
// its first row, notices a repeat as an occupied slot. With none, one probe per row of x
// writes the pair at once: ij's find, null test, where and index in one pass. With one,
// a second pass numbers the keys and lists y's rows key by key (a counting sort, so each
// key's rows stay in y's order), and each row of x copies its key's run.
// Keys compare as find does: integers of any width by value, symbols by id, floats by
// value with -0.0 equal to 0.0 and every NaN one value (AMNF). The table is a direct
// LUT when y's integer keys span at most 64K or 4 slots per key, else
// a hash sized to y. Anything else (chars, lists of rows, mismatched types) gives ()
// and amber.k takes its K path. Returns (rows of x;rows of y) as int vectors.
#define EJK(w,p,i) (fl?AMNF(((CO L*)(p))[i]):RD(w,p,i))
#define EJB(SL) for(U j=m;j--;){L v=EJK(wa,pa,j);I*h=SL;dup|=*h>=0;*h=(I)j;}
#define EJC(SL) F(m,L v=EJK(wa,pa,i);I*h=SL;I g=*h;I(g<0,g=*h=(I)ng++;cn[g]=0)cn[g]++;gy[i]=g)
#define EJH(v) W s=((W)(v)*GOLD)>>sh;W(hd[s]>=0&&hk[s]!=(v),s=(s+1)&msk)
#define EJLL(v) ({W d=(W)(v)-(W)lo;d<rg?hd[d]:-1;})
#define EJHL(v) ({EJH(v)hd[s];})
#define EJP(T,LK,ST) {CO T*RES pp=(CO T*)pb;F(n,L v=(L)pp[i];I k=LK;ST)}
#define EJQ(ST) I(fl,{CO L*RES pp=(CO L*)pb;F(n,L v=AMNF(pp[i]);I k=EJHL(v);ST)}) \
 J(lut,S4(wb,EJP(G,EJLL(v),ST),EJP(H,EJLL(v),ST),EJP(I,EJLL(v),ST),EJP(L,EJLL(v),ST))) \
 E(S4(wb,EJP(G,EJHL(v),ST),EJP(H,EJHL(v),ST),EJP(I,EJHL(v),ST),EJP(L,EJHL(v),ST)))
#define EJF free(hd);free(hk);free(gy);free(cn);free(st);free(ys);mr(a);mr(b);
A1(ejxC,P(_t(x)-tA||_n(x)-2,et(x))A a=_A(x)[0],b=_A(x)[1];
 P(_tP(a)||_tP(b),x(emp(tA)))
 UC ta=_t(a),tb=_t(b);B fl=ta==tF;
 P(!(LH(tE,ta,tL)&&LH(tE,tb,tL)||ta==tb&&(fl||ta==tS)),x(emp(tA)))
 P(_n(a)>>31||_n(b)>>31,x(emp(tA)))
 a=ta==tE?gZ(_R(a)):ta==tB?cG(_R(a)):_R(a);b=tb==tE?gZ(_R(b)):tb==tB?cG(_R(b)):_R(b);
 U m=_n(a),n=_n(b),wa=_w(a)-3,wb=_w(b)-3;CO V*pa=_V(a);CO V*pb=_V(b);
 P(!m||!n,mr(a);mr(b);x(aV(tA,2,A(aI(0),aI(0)))))
 L lo=0,hi=-1;W rg=0;
 I(!fl,lo=hi=RD(wa,pa,0);F(m,L v=RD(wa,pa,i);I(v<lo,lo=v)I(v>hi,hi=v))rg=(W)hi-(W)lo+1)
 B lut=rg&&(rg<=LUTDOM||rg<=4*(W)m);W cap=16;U lg=4;I(lut,cap=rg)E(W(cap<2*(W)m,cap<<=1;lg++))
 // a 32-bit target (wasm32): sizes past size_t go back to the K path, as ajc does
 P(cap>(W)((N)-1)/SZ(L)||(W)m+4>(W)((N)-1)/SZ(I)||(W)n>(W)((N)-1)/SZ(I),mr(a);mr(b);x(emp(tA)))
 U sh=64-lg;W msk=cap-1;
 I*hd=amal((N)cap*SZ(I)),*gy=0,*cn=0,*st=0,*ys=0;L*hk=lut?0:amal((N)cap*SZ(L));U ng=0;
 P(!hd||!lut&&!hk,EJF x(emp(tA)))
 MS(hd,0xff,(N)cap*SZ(I));B dup=0;
 I(lut,EJB(hd+((W)v-(W)lo)))E(EJB(({EJH(v)hk[s]=v;hd+s;})))
 I(dup,gy=amal((N)m*SZ(I));cn=amal((N)m*SZ(I));st=amal((N)m*SZ(I));ys=amal((N)(m+4)*SZ(I));P(!gy||!cn||!st||!ys,EJF x(emp(tA)))
   MS(hd,0xff,(N)cap*SZ(I));I(lut,EJC(hd+((W)v-(W)lo)))E(EJC(({EJH(v)hk[s]=v;hd+s;})))   // a repeat: number the keys, count each one's rows
   U t=0;F(ng,st[i]=t;t+=cn[i])F(m,ys[st[gy[i]]++]=(I)i)F(ng,st[i]-=cn[i])MS(ys+m,0,4*SZ(I)))   // and list y's rows key by key, each key's in y's order
 A rx,ry;
 I(!dup,rx=aI(n);ry=aI(n);I*RES p=_V(rx),*RES q=_V(ry);U h=0;
   EJQ(p[h]=(I)i;q[h]=k;h+=k>=0)
   AN(h,rx);AN(h,ry);)
 E(I*kk=amal((N)n*SZ(I));W t=0;P(!kk,EJF x(emp(tA)))
   EJQ(kk[i]=k;t+=k<0?0:(W)cn[k])
   P(t>>31,free(kk);EJF ez(x))
   rx=aI((U)t+4);ry=aI((U)t+4);I*RES p=_V(rx),*RES q=_V(ry);W o=0;
   // four slots written whatever the count, so the usual count (0 to 4) costs no branch: a row's
   // extra slots are overwritten by the next row's, or lie past the end, cut off below
   F(n,I k=kk[i];U c=k<0?0:(U)cn[k];CO I*RES r=ys+(k<0?0:st[k]);
     p[o]=p[o+1]=p[o+2]=p[o+3]=(I)i;q[o]=r[0];q[o+1]=r[1];q[o+2]=r[2];q[o+3]=r[3];
     for(U u=4;u<c;u++){p[o+u]=(I)i;q[o+u]=r[u];}o+=c)
   AN((U)t,rx);AN((U)t,ry);free(kk);)
 EJF
 x(aV(tA,2,A(rx,ry))))
#undef EJK
#undef EJB
#undef EJC
#undef EJH
#undef EJLL
#undef EJHL
#undef EJP
#undef EJQ
#undef EJF
