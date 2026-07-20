#include"a.h" // ngn/k, (c) 2019-2024 ngn, GNU AGPLv3 - https://codeberg.org/ngn/k/raw/branch/master/LICENSE
Z S s0,s;Z U k;Z A pb(A,C);                                                                         //parser state (s:current pointer, s0:start of source, k:implicit arg counter)
U si(S s,C v)_(strchrnul(s,v)-(C*)s)                                                                //find char (string index)
B id0(UC c)_(CAz(c)|(c|1)==0xd1)                                                                    //is identifier start char?
Z B id1(C c)_(id0(c)|C09(c))                                                                        //is identifier char?
Z B num(S s)_(C09(s[*s=='-']))                                                                      //is number start?
Z S pw(S s)_(W(*s==32,s++)s)                                                                        //skip whitespace
Z A1(p1,x&&xn==1?fir(x):x)                                                                          //singleton list to atom
S pID(S s)_(W(id1(*s),s+=0xe555>>((UC)*s>>4&-2)&3)s)                                                //parse identifier
W pu(S*p)_(S s=*p;W v=0;C c=*s;W(C09(c),v=10*v+c-'0';c=*++s)*p-s?*p=s,v:NL)                         //parse unsigned long
L pl(S*p)_(B m=**p=='-';*p+=m;(1-2*m)*pu(p))                                                        //parse long
Z L plN(S*p)_(L v=pl(p);!v&&**p=='N'?(*p)++,NL:v)                                                   //parse long (with support for nulls)
Z L pfu(S*p)_(L v=pu(p);S s=*p;C c=*s;P(c=='w',(*p)++;WFL)P(c=='n',(*p)++;v^NFL)I e=0;              //parse float unsigned
 I(c=='.',c=*++s;W(C09(c),I(v<(1ull<<63)/10,v=10*v+c-'0';e--)c=*++s))
 I(c=='e',s++;e+=pl(&s);P(e<-308,0)P(e>308,WFL))
 Z F t[309];I(!*t,*t=1;F(308,t[i+1]=10*t[i]))
 *p=s;*(L*)A(e<0?v/t[-e]:v*t[e]))
L pf(S*p)_(B m=**p=='-';(*p)+=m;L v=(L)m<<63|pfu(p);(*p)+=**p=='f';v)                               //parse float
Z A pV(C t,TY(pl)*f)_(L a[1<<9];U n=0;                                                              //parse ints or floats
 W(1,L v=f(&s);P(n>=L(a),ez0())a[n++]=v;S p=pw(s);B(p==s||!num(p))s=p)aV(t,n,a))
Z A0(pZ,S p=s;W(*p-'0'<2u,p++)                                                                      //parse ints
 P(*p=='B',S t=s;s=p+1;cB(aV(tG,p-t,t)))//todo
 P(*p=='b',S t=s;s=p+1;cG(cB(aV(tG,p-t,t))))
 sqzZ(N(pV(tL,plN))))
Z A0(pF,pV(tF,pf))                                                                                  //parse floats
Z A0(pC,C a[1<<9];U n=0;C c=*++s;                                                                   //parse "string"
 W(c&&c-'"'&&n<L(a),I(c=='\\',c=*++s;U i=fG("tnr0",4,c);I(i<4,c="\t\n\r"[i]))a[n++]=c;c=*++s)
 P(!c,ep0())P(n>=L(a),ez0())s++;aV(tC,n,a))
Z A0(p0x,S p=s;W(CA9(*p),p++)A x=N(unhC(s,p-s));s=p;x)                                              //parse 0x string
Z A0(ps,S p=s;C c=*s;I(id0(c),s=pID(s))J(c>>7,W(*++s<-64)s+=*s==':')aCm(p,s))                       //parse symbol
Z A pS(C c)_(I a[256];U n=0;                                                                        //parse symbols
 W(1,P(n>=L(a),ez0())A y=*++s-'"'?ps():N(pC());y=str0(y);a[n++]=us(yC);y(0);S p=pw(s);B(*p-c)s=p)
 aV(tS,n,a))
Z A0(pP,I a[8];U n=0;                                                                               //parse dot-separated path of identifiers
 W(1,P(n>=L(a),ez0())A y=str0(ps());a[n++]=us(yV);y(0);B(*s-'.'||!id0(s[1]))++s)
 aV(tS,n,a))
Z A0(pp,P(*s-'[',au)A x=N(pS(';'));P(*s-']'||!xn,ep(x))P(xN>8,ez(x))s++;x)                          //parse parameter list
Z A pt(C*v)_(C c=*s;                                                                                //parse term
 P(c=='`',qte(p1(N(pS('`')))))
 P(c=='"',p1(pC()))
 P(c=='[',s++;pb(GAP,']'))
 P(c=='(',s++;P(*s==')',s++;emp(tA))A x=N(pb(MKL,')'));xn-2?x:las(x))
 P(c=='{',C k0=k;k=1;S s1=s0,t=s0=s++;A y=N(pp()),z=pb(GAP,'}');P(!z,s0=s1;y(0))I(y==au,y=aS(k);F(3,yi='x'+i))A x=N(cpl(aCn(t,s-t),z,y));s0=s1;k=k0;x)
 P(id0(c),S p=s;A x=pP();I(s-p==1&&c-'y'<2u,k=MAX(k,c-'w'))AO(p-s0,x))
 P(C09(c)&&s[1]==':',B u=s[2]==':';s+=2+u;U i=20+c-'0';P(i>25,ep0())*v=1;Lt(tv-u)|i)
 P(c=='0'&&s[1]=='x',s+=2;p1(p0x()))
 P(num(s)&&(c-'-'||s==s0||(!id1(s[-1])&&!strchr(")]}\"",s[-1]))),
  B d=0;S p=s;c=*p;W(1,p=pw(p);B(!num(p))p+=*p=='-';c=*p;B(!CA9(c))W(CA9(c)||c=='.'||c==':',d|=!!strchr(".nwef",c);c=*++p))p1(d?pF():pZ()))
 P(c>>7,S p=s;A x=pP();*v=1;AO(p-s0,x))
 U i=si("'/\\",c);P(i<3,c=*++s;B h=c==':';s+=h;*v=1;aw+i+3*h)i=si(vc,c);P(i>19,GAP)
 B u=*++s==':';s+=u;*v=1;Lt(tv-u)|i)
Z X1(pm,                                                                                            //monadify
 Rv(x^au^av)
 RA(I(xx==aw,x=mut(x);xA[xn-1]=pm(xA[xn-1]))x)
 Rs(S s=su(xv);U n;P(*s>>7&&s[(n=SL(s))-1]-':',C b[8];MC(b,s,n);b[n]=':';b[n+1]=0;sym(b))x)
 RS(xn==1?enl(pm(fir(x))):x)
 R_(x))
Z A pT(C*v)_(A x=N(pt(v));                                                                          //parse term and the adverbs or square brackets after it (v:verb?)
 W(1,C c=*s;U i=si("'/\\[",c);P(i>3,x)s++;
  I(i>2,x=AO(s-1-s0,N(pb(x,']')));I(xn==2,I(xy==GAP,xy=au)E(xx=pm(xx)))*v=0)
  E(I c=*s==':';s+=c;x=aA2(aw+i+3*c,x);*v=1))x)
Z A pe(A x,C*v)_(s=pw(s);C c=*s;                                                                    //parse expression
 I(c=='/'&&(s==s0||s[-1]==32||s[-1]==10),
  I(s[1]==10,C*e=strstr(s+1,"\n\\\n");s=e?e+2:s+SL(s))
  E(W((c=*++s)&&c-10)))
 P(s>s0&&*s=='\\'&&s[-1]==32,s++;A y=pe(0,v);P(!y,x?x(0):0);*v=0;y=aA2(OUT,y);I(x,y=aA2(pm(x),y))y)
 UH o=s-s0;C b=0;A y=pT(&b);P(!y,x?x(0):0)P(y==GAP,x?x:y)
 P(!b,A z=pe(y,v);P(!x,z)Nx(z);*v?aA3(aw,x,z):AO(o,aA2(pm(x),z)))
 A z=pe(0,v);P(!z,y(x?x(0):0))P(z==GAP,*v=1;P(!x,y)Yu(ep(x))AO(o,aA3(y,x,z)))
 *v&=y!=av;I(!x,y=pm(y))*v?aA3(aw,x?AO(o,aA3(y,x,GAP)):y,z):AO(o,x?aA3(y,x,z):aA2(pm(y),z)))
Z A pb(A x,C c)_(x=x?aA1(x):emp(tA);                                                                //parse body (sequence of ;-separated expressions)
 W(1,C v=0;A y=Nx(pe(0,&v));PSH(x,c-']'&&y==GAP?au:y);P(y==GAP&&c==')',ep(x))B(*s-';'&&*s-10)B(c==10&&*s==10)s++)
 P(c==10&&!*s,x)P(*s-c,ep(x))s++;x)
A pk(S*p,C c)_(s0=s=*p;A x=pb(GAP,c);*p=s;P(x,xn==2?las(x):x)eQ(s0,SL(s0),s-s0);0)                  //parse either a group of lines (c='\n') or till '\0' (c='\0')
