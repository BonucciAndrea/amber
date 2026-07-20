#include"a.h" // ngn/k, (c) 2019-2024 ngn, GNU AGPLv3 - https://codeberg.org/ngn/k/raw/branch/master/LICENSE
#define abn V*RES a,CO V*RES b,U n
#define M(x,y,z) Z V c##x##y(abn){x*r=AL(a);CO y*p=AL(b);F(n+((1<<z)-1)>>z,Fj(1<<z,*r++=*p++))}
 M(H,G,4)M(G,H,4)M(H,I,3)M(I,H,3)M(G,I,3)M(I,G,3)M(H,L,2)M(L,I,2)M(I,L,2)M(G,L,2)//cHG cGH cHI cIH cGI cIG cHL cLI cIL cGL
#undef M
Z V cFL(abn){F*r=AL(a);CO L*p=AL(b);F(n+3>>2,Fj(4,*r++=*p==NL?NF:*p;p++))}
Z V cLF(abn){L*r=AL(a);CO F*p=AL(b);F(n+3>>2,Fj(4,*r++=*p!=*p?NL:*p;p++))}
Z V cGB(abn){G*r=AL(a);CO G*p=AL(b);F(n,*r++=p[i>>3]>>(i&7)&1)}
Z V cBG(abn){G*r=AL(a);CO G*p=AL(b);MS(r,0,n+63>>6<<3);F(n,r[i>>3]|=(*p++&1)<<(i&7))}
Z V cLA(abn){L*r=AL(a);CO A*p=AL(b);F(n,*r++=gl_(*p++))}
Z A2(cT,UC t=xv,u=yt,i=t-tB,j=u-tB;Q(i<8);P(j>6,et(y))
 Z CO TY(&cBG)a[][8]={{0,cBG,0,0,0,0,cBG},{cGB,0,cGH,cGI,cGL,0,0},{0,cHG,0,cHI,cHL,0,cHG},{0,cIG,cIH,0,cIL,0,cIG},{0,0,0,cLI,0,cLF,0},{0,0,0,0,cFL,0,0},{},{}};
 TY(&cBG)f=a[i][j];P(f,A z=an(yn,t);My(f(zV,yV,zn));z)
 P(t==tS,u==tC?(y=str0(y),y(sym(yV))):et(y))
 P((1<<t|1<<u)==(1<<tG|1<<tC),AT(t,mut(y)))
 cT(t,N(cT(u==tB?tG:G(tG,tI,tI,tL,tI,tL,tG,tC)[i],y))))
NI A2(ct,UC t=xv,u=yt;Q(xti||x==t)Q(tB<=t&&t<=tS)P(t==TT[u],y)
 Y(RmMA(r2f(ct,x,y))RE(cT(x,gZ(y)))Rf(ct(x,al(gf(y))))Rilc(L v=gl(y);S(t,R4(tB,tG,tH,tI,ai(v))RC(ac(v))RL(al(v))RF(af(v))RS(u==tc?as(yv):et0()))0)R_(cT(x,y)))et(y))
A1(cB,ct(tB,x))A1(cG,ct(tG,x))A1(cH,ct(tH,x))A1(cI,ct(tI,x))A1(cL,ct(tL,x))A1(cF,ct(tF,x))A1(cC,ct(tC,x))A1(cS,ct(tS,x))
Z X1(csti,RmMA(e1f(csti,x))RF(sqzZ(cL(x)))Rf(az(gf(x)))RC(cG(x))Rc(ai(xv))Ruvw(ai(xv))RilEGHIL(x)R_(et(x)))
Z X1(prsI,RmMA(e1f(prsI,x))Rc(prsI(enl(x)))RC(x=str0(x);S s=xV;P(!*s,x(_R(cn[tl])))L v=pl(&s);x(*s?_R(cn[tl]):az(v)      ))R_(et(x)))
Z X1(prsF,RmMA(e1f(prsF,x))Rc(prsF(enl(x)))RC(x=str0(x);S s=xV;P(!*s,x(_R(cn[tf])))L v=pf(&s);x(*s?_R(cn[tf]):aV(tf,1,&v)))R_(et(x)))
Z Y2(pad,RmMA(e2f(pad,x,y))RC(K2("{y@(!x)+(x<0)*#y}",x,y))Rc(dlr(x,enl(y)))R_(et(y)))
X2(dlr,Rs(I v=xv;P(v-(C)v,ed(y))G(&csti,cF,cC,cS,prsI,prsF,ed)[si("ifcsIF",v|'s'*!v)](y))Ril(pad(x,y))R_(et(y)))
X1(sqzZ,R_(x)/*RG(F(xn,P(xg&-2,x))cB(x))*/
 RH(F(xn,P(xh-(G)xh,x))cG(x))
 RI(F(xn,P(xi-(H)xi,x))sqzZ(cH(x)))
 RL(F(xn,P(xl-(I)xl,x))sqzZ(cI(x))))
Z A sqzA(A x,C t)_(U n=xn,w=Tw[t];Q(w-3<4u)P(w==6,A y=an(n,t);cLA(yV,xV,n);x(y))A y=an(n,w==5?t:tI);cIL(yV,xV,n);x(0);ct(t,y))
A1(sqz,P(!xtA,x)U n=xn;A y=xx;C t=yt;
 Y(Ril(B l=0;F(n,A y=xa;I(ytl,l=1)E(P(!yti,x)))
       P(l,sqzA(x,tL))
       A z=an(n,tI);cIL(zV,xV,n);x=x(z);
       I a=0;F(n,I v=xi;a|=v^v>>31)t=tZ(a);P(t==tI,x)
       A y=an(n,t);G(&cGI,cHI)[t-tG](yV,xV,n);x(y))
   R3(tf,tc,ts,F(n,P(_t(xa)-t,x))sqzA(x,TT[t]))
   Rm(F(n,P(_t(xa)-tm||!mtc_(yx,_x(xa)),x))y=aM(_R(yx),e1f(rs0,_R(yy)));F(n,PSH(y,_R(xa)))x(y))
   R_(x))0)
X1(blw,RA(x)Rt(aA1(x))Rm(et(x))R_(U n=xN;A y=aA(n);F(n|!n,ya=ii(x,i))x(0);I(!n,yx=mkn(yx))y))
A1(gZ,Lij x(0);P(i<0,x=az(i);x(add(x,gZ(aE(0,j-i)))))C t=MAX(tZ(i),tZ(j-1));x=an(j-i,t);tilV(xV,i,j-i,t-tG);x)
