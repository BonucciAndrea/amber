#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#define abn V*RES a,CO V*RES b,U n
#define M(x,y,z) Z V c##x##y(abn){x*r=AL(a);CO y*p=AL(b);F(n+((1<<z)-1)>>z,Fj(1<<z,*r++=*p++))}
 M(H,G,4)M(G,H,4)M(H,I,3)M(I,H,3)M(G,I,3)M(I,G,3)M(H,L,2)M(L,I,2)M(I,L,2)M(G,L,2)//cHG cGH cHI cIH cGI cIG cHL cLI cIL cGL
#undef M
Z V cFL(abn){F*r=AL(a);CO L*p=AL(b);F(n+3>>2,Fj(4,*r++=*p==NL?NF:*p;p++))}
Z V cLF(abn){L*r=AL(a);CO F*p=AL(b);L o=0;F(n,F v=p[i];L b=__builtin_fabs(v)<0x1p63;o|=(b|v!=v)^1;L c=(L)(b?v:0);r[i]=b?c:NL)I(o,F(n,F v=p[i];B b=__builtin_fabs(v)<0x1p63;L c=(L)(b?v:0);r[i]=b?c:v>0?WL:v<0?NL+1:NL))}//as F2C; the second loop only for a value out of range
Z V cGB(abn){G*r=AL(a);CO G*p=AL(b);F(n,*r++=p[i>>3]>>(i&7)&1)}
Z V cBG(abn){G*r=AL(a);CO G*p=AL(b);MS(r,0,n+63>>6<<3);F(n,r[i>>3]|=(*p++&1)<<(i&7))}
Z V cLA(abn){L*r=AL(a);CO A*p=AL(b);F(n,*r++=gl_(*p++))}
// amber 2.7: a cast of 1M items or more runs on the thread pool, chunks of whole 32-item blocks (the kernels
// write in blocks of up to 16), each item made as on one thread. Bit vectors (B) stay on one thread.
#include"parallel.h"
TD V(*CK)(V*RES,CO V*RES,U);TD struct{CK f;C*o;CO C*p;U n,so,si;int nt;}CJ;
Z V cjw(V*c_,int t){CJ*c=c_;U nb=(c->n+31)/32;U s=(U)((W)nb*t/c->nt)*32,e=(U)((W)nb*(t+1)/c->nt)*32;if(e>c->n)e=c->n;if(e>s)c->f(c->o+(N)s*c->so,c->p+(N)s*c->si,e-s);}
Z V pcast(CK f,V*o,CO V*p,U n,U so,U si){int nt=n<(1u<<20)?1:par_thread_count(n);if(nt>(int)(n>>18))nt=(int)(n>>18);if(nt<2){f(o,p,n);return;}CJ c={f,(C*)o,(CO C*)p,n,so,si,nt};par_run(nt,cjw,&c);}
Z A2(cT,UC t=xv,u=yt,i=t-tB,j=u-tB;Q(i<8);P(j>6,et(y))
 Z CO TY(&cBG)a[][8]={{0,cBG,0,0,0,0,cBG},{cGB,0,cGH,cGI,cGL,0,0},{0,cHG,0,cHI,cHL,0,cHG},{0,cIG,cIH,0,cIL,0,cIG},{0,0,0,cLI,0,cLF,0},{0,0,0,0,cFL,0,0},{},{}};
 TY(&cBG)f=a[i][j];P(f,A z=an(yn,t);My(I(t==tB||u==tB,f(zV,yV,zn))E(pcast(f,zV,yV,zn,1u<<(Tw[t]-3),1u<<(Tw[u]-3))));z)
 P(t==tS,u==tC?(y=str0(y),y(sym(yV))):et(y))
 P((1<<t|1<<u)==(1<<tG|1<<tC),AT(t,mut(y)))
 cT(t,N(cT(u==tB?tG:G(tG,tI,tI,tL,tI,tL,tG,tC)[i],y))))
NI A2(ct,UC t=xv,u=yt;Q(xti||x==t)Q(tB<=t&&t<=tS)P(t==TT[u],y)
 Y(RmMA(r2f(ct,x,y))RE(cT(x,gZ(y)))Rf(ct(x,al(F2C(gf(y)))))Rilc(L v=gl(y);S(t,R4(tB,tG,tH,tI,ai(v))RC(ac(v))RL(al(v))RF(af(v==NL?NF:v))RS(u==tc?as((UC)yv):et0()))0)R_(cT(x,y)))et(y))
A1(cB,ct(tB,x))A1(cG,ct(tG,x))A1(cH,ct(tH,x))A1(cI,ct(tI,x))A1(cL,ct(tL,x))A1(cF,ct(tF,x))A1(cC,ct(tC,x))A1(cS,ct(tS,x))
Z X1(csti,RmMA(e1f(csti,x))RF(sqzZ(cL(x)))Rf(az(F2C(gf(x))))RC(cG(x))Rc(ai(xv))Ruvw(ai(xv))R(tdt,ai((I)x))R(ttm,ai((I)x))R(tnp,L v_=*(L*)_V(x);x(al(v_)))RilEGHIL(x)R_(et(x)))
Z B pov(S p,S e)_(B m=*p=='-';p+=m;W(*p=='0'&&C09(p[1]),p++)I n=e-p;n>19||n==19&&strncmp(p,m?"9223372036854775808":"9223372036854775807",19)>0)//digits p..e beyond int64?
Z X1(prsI,RmMA(e1f(prsI,x))Rc(prsI(enl(x)))RC(x=str0(x);S s=xV;P(!*s,x(_R(cn[tl])))L v=pl(&s);x(*s||pov(xV,s)?_R(cn[tl]):az(v)))R_(et(x)))
Z X1(prsF,RmMA(e1f(prsF,x))Rc(prsF(enl(x)))RC(x=str0(x);S s=xV;P(!*s,x(_R(cn[tf])))L v=pf(&s);x(*s?_R(cn[tf]):aV(tf,1,&v)))R_(et(x)))
A ptT(A),ptD(A),ptP(A);   //"T"$ "D"$ "P"$: the text as the literal reader reads it (p.c)
Z X1(cSb,RmMA(e1f(cSb,x))RC(U i=0,n=xn;W(i<n&&CBK(xC[i]),i++)W(n>i&&CBK(xC[n-1]),n--)P(!i&&n==xn,cS(x))x(cS(aCn(xC+i,n-i))))Rc(cS(CBK(xv)?emp(tC):x))R_(cS(x)))//`$: blanks trimmed, as q
Z Y2(pad,RmMA(e2f(pad,x,y))RC(K2("{y@(!x)+(x<0)*#y}",x,y))Rc(dlr(x,enl(y)))R_(et(y)))
X2(dlr,Rs(I v=xv;P(v-(C)v,ed(y))G(&csti,cF,cC,cSb,prsI,prsF,ed)[si("ifcsIF",v|'s'*!v)](y))Ril(pad(x,y))
 Rc(C ch=xv;P(ch=='D'||ch=='d',ptD(y))P(ch=='T'||ch=='t',ptT(y))P(ch=='P'||ch=='p',ptP(y))et(y))R_(et(y)))
X1(sqzZ,R_(x)/*RG(F(xn,P(xg&-2,x))cB(x))*/
 RH(F(xn,P(xh-(G)xh,x))cG(x))
 RI(F(xn,P(xi!=(H)xi,x))sqzZ(cH(x)))
 RL(F(xn,P(xl!=(I)xl,x))sqzZ(cI(x))))
Z A sqzA(A x,C t)_(U n=xn,w=Tw[t];Q(w-3<4u)P(w==6,A y=an(n,t);cLA(yV,xV,n);x(y))A y=an(n,w==5?t:tI);cIL(yV,xV,n);x(0);ct(t,y))
A1(sqz,P(!xtA,x)U n=xn;A y=xx;C t=yt;
 Y(Ril(B l=0;F(n,A y=xa;I(ytl,l=1)E(P(!yti,x)))
       P(l,sqzA(x,tL))
       A z=an(n,tI);cIL(zV,xV,n);x=x(z);
       I a=0;F(n,I v=xi;a|=v^v>>31)t=tZ(a);P(t==tI,x)
       A y=an(n,t);G(&cGI,cHI)[t-tG](yV,xV,n);x(y))
   R3(tf,tc,ts,F(n,P(_t(xa)-t,x))sqzA(x,TT[t]))
   Rm(P(!n,x)F(n,P(_t(xa)-tm||!mtc_(yx,_x(xa)),x))y=aM(_R(yx),blw(e1f(rs0,_R(yy))));F(n,PSH(y,_R(xa)))x(y))
   R_(x))0)
X1(blw,RA(x)Rt(aA1(x))Rm(et(x))R_(U n=xN;A y=aA(n);F(n|!n,ya=ii(x,i))x(0);I(!n,yx=mkn(yx))y))
A1(gZ,Lij x(0);P(i<0,x=az(i);x(add(x,gZ(aE(0,j-i)))))C t=MAX(tZ(i),tZ(j-1));x=an(j-i,t);tilV(xV,i,j-i,t-tG);x)
