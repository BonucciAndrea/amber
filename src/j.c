#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
Z CO C je[]="\"\\/\b\f\n\r\t",ej[]="\"\\/bfnrt";Z C*s;

Z A0 jx;Z C jw()_(W(*s&&*s<=32u,s++)*s)
Z A0(jd,I m=*s=='-';s+=m;C c=*s++;P(!C09(c),0)F v=c-'0';W(C09(*s),v*=10;v+=*s++-'0')I(*s=='.',s++;F t=1;W(C09(*s),t/=10;v+=t*(*s++-'0')))
 I((*s|32)=='e',s++;s+=*s=='+';L e=pl((V*)&s);F t=10;I(e<0,e=-e;t=.1)P(e>308,0)F(e,v*=t))af(m?-v:v))
Z I ju(S s)_(I v=0;F(4,UC c=*s++,d=c-'0';v<<=4;v|=d<10?d:(d=(c|32)-'a')<6?d+10:-1)v)
Z A0(js,UC c;C*p=++s;U n=0;W((c=*p)-'"',P(c<32,s=p;0)p++;I(c=='\\',c=*p++;P(c<32,s=p;0)n++;I(c=='u',L v=ju(p);P(v<0,s=p;0)p+=4;n+=4-(v>127)-(v>2047))))A x=aC(p-s-n);p=s;C*r=xC;
 W((c=*p++)-'"',I(c=='\\',c=*p++;I(c-'u',U j=si(ej,c);P(j>=L(ej),s=p-1;x(0))c=je[j])E(I v=ju(p);p+=4;I(v<128,c=v)J(v<2048,*r++=192|v>>6;c=128|63&v)E(*r++=224|v>>12;*r++=128|63&v>>6;c=128|63&v)))*r++=c)s=p;x)
Z A0(ja,s++;A x=emp(tA);P(jw()==']',s++;x)W(1,PSH(x,Nx(jx()));C c=jw();P(c==']',s++;x)Nx(c==',');s++)0)
Z A0(jo,s++;A x=emp(tS),y=emp(tA);C c=jw();P(c=='}',s++;am(x,y))W(c=='"',A z=js();B(!z)PSH(x,cS(z));B(jw()-':')s++;z=jx();B(!z)PSH(y,z);c=jw();P(c=='}',s++;am(x,y))B(c-',')s++;c=jw())x(y(0)))
Z A jx(){C c=jw();S(c,R3('f','n','t',c=c>>3&3;I v=*(I*)(V*)A("alse","null","true")[c]==*(I*)(V*)(s+!c);s+=(4+!c)*v;!v?0:c==1?_R(cn[tf]):au+!!c)
 R('[',ja())R('{',jo())R('"',js())R_(jd()))}
X1(js0,RC(U n=xn;x=aa(n+4,x);MS(xC+n,0,4);s=xV;A y=jx();C c=jw();I(y&&c,y=y(0))I(!y,ep0();eS(x,s-xC))x(y))Rc(js0(enl(x)))R_(et(x)))

Z V JX(A);Z U nX(A);Z UC t[256];ZN V it(){MS(t,5,32);t[127]=5;F(L(je),t[je[i]]=1)}
Z U nC(S p,U n)_(U m=2+n;F(n,m+=t[(UC)p[i]])m)
Z V JC(S p,U n){*s++='"';UC c;F(n,S(t[c=*p++],C(0,*s++=c)C(1,*s++='\\';*s++=ej[si(je,c)])D(MC(s,"\\u00",4);s+=4;hexC(p-1,2,s);s+=2)))*s++='"';}
Z U nl(L v)_(P(v==NL,4)U n=v<0?(v=-v),2:1;W m=10;W(m<=v&&n<19,n++;m*=10)n)
Z V Jl(L v){s=v-NL?sl(s,v):MC(s,"null",4)+4;}
Z U nx(A x/*0*/){X(Ri(nl(xv))Rl(nl(*xL))Rc(C c=xv;nC(&c,1))Rf(F v=*xF;P(v==(L)v,nl(v))P(v!=v,4)C b[32];sf(b,*(L*)&v)-b)Rm(nx(xx)+nx(xy)-1-!_N(xx))Ru(4+!xv)RC(nC(xC,xn))
 RMT_C(U n=xN,m=1+n+!n;F(n,m+=nX(ii(x,i)))m)R_(nX(str(xR))))}
Z A Jx(A x/*0*/){X(Rm(*s++='{';F(xN,*s=',';s+=!!i;JX(ii(xx,i));*s++=':';JX(ii(xy,i)))*s++='}';x)Rf(F v=*xF;s=v==(L)v?sl(s,v):v!=v?MC(s,"null",4)+4:sf(s,*(L*)&v);x)Ri(s=sl(s,xv);x)
 Rl(Jl(*xL);x)Rc(C c=xv;JC(&c,1);x)RC(JC(xC,xn);x)Ru(U n=4+!xv;MC(s,xv?"true":"false",n);s+=n;x)R_(I(xtMT,*s++='[';F(xN,I(i,*s++=',')JX(ii(x,i)))*s++=']')E(JX(str(xR)))x))}
Z U nX(A x/*1*/)_(U n=nx(x);x(0);n)
Z V JX(A x/*1*/){Jx(x);x(0);}
A1(js1,I(!*t,it())A y=aC(nx(x));s=yC;JX(x);Q(s==yC+yn);y)
