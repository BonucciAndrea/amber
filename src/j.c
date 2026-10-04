#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include"csv.h"   //csv_float: numbers read as a CSV cell reads them
Z CO C je[]="\"\\/\b\f\n\r\t",ej[]="\"\\/bfnrt";Z AM_TLS_IE C*s;   //read/write position, per thread (peach workers)

Z A0 jx;Z C jw()_(W(*s&&*s<=32u,s++)*s)
ZN F jdx(C*b,C*f,C*p,W w,L x,L n,I d)_(F v;I(n>19?!csv_long(b,f-b-d,f,x+n,&v):!csv_fast(w,x,&v),v=csv_float(b,p));v)   //the Eisel-Lemire method (csv_fast), else from the first 19 digits, else csv_float: correctly rounded
Z A0(jd,C*p=s;I m=*p=='-';p+=m;C*b=p;P(!C09(*p),s=p+1;0)W w=0;W(C09(*p),w=10*w+(W)(*p-'0');p++)   //w the digits
 P(p-b<16&&*p-'.'&&(*p|32)-'e',s=p;F v=(F)w;af(m?-v:v))C*f=p;I d=*p=='.';   //an integer of at most 15 digits is exact as it is; f where the fraction starts (if d, a point)
 I(d,f=++p;W(C09(*p),w=10*w+(W)(*p-'0');p++))L x=f-p,n=p-b-d;   //n the digits, x the exponent; past 19 digits w wraps, and csv_long reads the first 19 again
 F v;P(n<16&&(*p|32)-'e'&&csv_cl(w,x,&v),s=p;af(m?-v:v))   //at most 15 digits and no exponent: Clinger's path, before the exponent's checks
 I((*p|32)=='e',p++;I q=*p=='-';p+=*p=='+'||*p=='-';P(!C09(*p),s=p;0)L e=0;W(C09(*p),I(e<1000000000000000ll,e=10*e+*p-'0')p++)x+=q?-e:e)   //an exponent needs a digit
 s=p;I(n>19||!csv_cl(w,x,&v),v=jdx(b,f,p,w,x,n,d));af(m?-v:v))   //exact in one step (Clinger's), else jdx, out of line
Z I ju(S s)_(I v=0;F(4,UC c=s[i],d=c-'0';I(d>9,d=(c|32)-'a';P(d>5,-1)d+=10)v=v<<4|d)v)   //-1 at the first byte that is not a hex digit, so never past the text's end
Z A0(js,UC c;C*p=++s;U n=0;W((c=*p)-'"',P(c<32,s=p;0)p++;I(c=='\\',c=*p++;P(c<32,s=p;0)n++;I(c=='u',L v=ju(p);P(v<0,s=p;0)p+=4;I(v>>10==54&&*p=='\\'&&p[1]=='u'&&ju(p+2)>>10==55,p+=6;n+=7;continue)n+=4-(v>127)-(v>2047))))A x=aC(p-s-n);p=s;C*r=xC;
 W((c=*p++)-'"',I(c=='\\',c=*p++;I(c-'u',U j=si(ej,c);P(j>=L(ej),s=p-1;x(0))c=je[j])E(I v=ju(p);p+=4;I(v>>10==54&&*p=='\\'&&p[1]=='u'&&ju(p+2)>>10==55,I u=0x10000+(v-0xD800<<10)+ju(p+2)-0xDC00;p+=6;*r++=240|u>>18;*r++=128|63&u>>12;*r++=128|63&u>>6;c=128|63&u)   //a surrogate pair is one character, four bytes
  J(v<128,c=v)J(v<2048,*r++=192|v>>6;c=128|63&v)E(*r++=224|v>>12;*r++=128|63&v>>6;c=128|63&v)))*r++=c)s=p;x)
Z A0(ja,s++;A x=emp(tA);P(jw()==']',s++;x)W(1,PSH(x,Nx(jx()));C c=jw();P(c==']',s++;x)Nx(c==',');s++)0)
Z A0(jo,s++;A x=emp(tS),y=emp(tA);C c=jw();P(c=='}',s++;am(x,y))W(c=='"',A z=js();B(!z)PSH(x,cS(z));B(jw()-':')s++;z=jx();B(!z)PSH(y,z);c=jw();P(c=='}',s++;am(x,y))B(c-',')s++;c=jw())x(y(0)))
//true/false/null are matched with memcmp: loading them as an I read the input unaligned.
//true and false read as 1 and 0 (repl.k: `j?"..[true,..]" -> (1;..)); they were the verbs +: and ::.
Z __attribute__((aligned(16))) A jx(){C c=jw();S(c,R3('f','n','t',c=c>>3&3;I v=!memcmp(A("alse","null","true")[c],s+!c,4);s+=(4+!c)*v;!v?0:c==1?_R(cn[tf]):ai(c>>1))
 R('[',ja())R('{',jo())R('"',js())R_(jd()))}   //aligned to 16 bytes: placed at 4 or 12 mod 16, as the linker left it, jx (jd inlined) read JSON numbers 4-6% more slowly
X1(js0,RC(U n=xn;x=aa(n+4,x);MS(xC+n,0,4);s=xV;A y=jx();C c=jw();I(y&&c,y=y(0))I(!y,ep0();eS(x,s-xC))x(y))Rc(js0(enl(x)))R_(et(x)))

Z V JX(A);Z U nX(A);Z AM_TLS_IE I jf;   //jf: a value's text failed (a formatter's error); per thread, as s is
Z AM_TLS_IE UC t[256];/*escape table, filled on first use: per thread, so no worker reads it half filled*/ZN V it(){MS(t,5,32);t[127]=5;F(L(je)-1,t[je[i]]=1)}  //L(je) counts the literal's NUL, which made NUL a one-letter escape
Z U nC(S p,U n)_(U m=2+n;F(n,m+=t[(UC)p[i]])m)
//a control character is \u00 and its two hex digits: hexC of ONE byte (it was 2, which read the
//next byte, past the end for the last character, and wrote one digit past the end of the output)
Z V JC(S p,U n){*s++='"';UC c;F(n,S(t[c=*p++],C(0,*s++=c)C(1,*s++='\\';*s++=ej[si(je,c)])D(MC(s,"\\u00",4);s+=4;hexC(p-1,1,s);s+=2)))*s++='"';}
//JI: v is integral and inside L, so (L)v is defined. v==(L)v alone converted NaN, infinities
// and floats beyond 2^63 to L (undefined): the optimiser read it as "integral" and the cast
// saturated, so `j@1e20 and `j@0w wrote 9223372036854775807, and `j@-0w wrote 0N and two
// unwritten bytes. NaN and infinities are written as null (JSON has neither; JSON.stringify
// does the same); other floats beyond L as floats (1e20).
#define JI(v) (-0x1p63<(v)&&(v)<0x1p63&&(v)==(L)(v))
Z U nl(L v)_(P(v==NL,4)U n=v<0?(v=-v),2:1;W m=10;W(m<=v&&n<20,n++;m*=10)n)
Z V Jl(L v){s=v!=NL?sl(s,v):MC(s,"null",4)+4;}  //v-NL overflowed for v>=0
//(from upstream ngn/k fdf2ec5f, "json character count") the size of a list, item by item: a dict's
//keys and values are sized as lists (nx sized a char vector as one string, where each char is
//written as a string of its own), and nl counts up to 20 characters (a negative 19-digit int).
Z U nxe(A x/*0*/)_(U n=xN,m=1+n+!n;F(n,m+=nX(ii(x,i)))m)
//a dict's keys: a number is written as a string, as q ({1:3} is no JSON), so two quotes more each; a key that is
//a list other than a string is 'type, as q (digest #60)
Z B jkq(UC u)_(u==ti||u==tl||u==tf)
Z U nkq(A k/*0*/)_(UC u=_t(k);P(_tP(k)||u==tS||u==tC,0)P(LH(tB,u,tF),2*_n(k))P(u-tA,0)U m=0;
 F(_n(k),A e=ii(k,i);UC v=_t(e);I(jkq(v),m+=2)J(!_tP(e)&&LH(tA,v,tS)&&v!=tC,jf=1;et0())mr(e))m)
Z V JK(A k/*1*/);
Z U nx(A x/*0*/){X(Ri(nl(xv))Rl(nl(*xL))Rc(C c=xv;nC(&c,1))Rf(F v=*xF;P(JI(v),nl((L)v))P(v-v!=0,4)C b[32];sf(b,*(L*)&v)-b)Rm(nxe(xx)+nxe(xy)-1-!_N(xx)+nkq(xx))Ru(4)RC(nC(xC,xn))
 RMT_C(nxe(x))R_(A y=str(xR);P(!y,jf=1;0)nX(y)))}
Z A Jx(A x/*0*/){X(Rm(*s++='{';F(xN,*s=',';s+=!!i;JK(ii(xx,i));*s++=':';JX(ii(xy,i)))*s++='}';x)Rf(F v=*xF;s=JI(v)?sl(s,(L)v):v-v!=0?MC(s,"null",4)+4:sf(s,*(L*)&v);x)Ri(s=sl(s,xv);x)
 Rl(Jl(*xL);x)Rc(C c=xv;JC(&c,1);x)RC(JC(xC,xn);x)Ru(P(!xv,s=MC(s,"null",4)+4;x)U n=4+!xv;MC(s,xv?"true":"false",n);s+=n;x)R_(I(xtMT,*s++='[';F(xN,I(i,*s++=',')JX(ii(x,i)))*s++=']')E(A y=str(xR);I(y,JX(y))E(jf=1))x))}
Z U nX(A x/*1*/)_(U n=nx(x);x(0);n)
Z V JX(A x/*1*/){Jx(x);x(0);}
Z V JK(A k/*1*/){B q=jkq(_t(k));I(q,*s++='"')JX(k);I(q,*s++='"')}
A1(js1,I(!*t,it())jf=0;U n=nx(x);P(jf,x(0))A y=aC(n);s=yC;JX(x);P(jf,y(0))Q(s==yC+yn);y)
