#include"a.h" // Amber parser - GNU AGPLv3 - see LICENSE and NOTICE
#include"csv.h"   //csv_float: numbers read as a CSV cell reads them
Z S s0,s,ppe;Z U k;Z A pb(A,C);Z A pe(A,C*);Z A ps();                                                   //Parser state (s:current pointer, s0:start of source, k:implicit arg counter, ppe: just past the last [params] read, cleared for each parse)
U si(S s,C v)_(strchrnul(s,v)-(C*)s)                                                                //find char (string index)
B id0(UC c)_(CAz(c)|(c|1)==0xd1)                                                                    //is identifier start char?
Z B id1(C c)_(id0(c)|C09(c))                                                                        //is identifier char?
Z B num(S s)_(C09(s[*s=='-']))                                                                      //is number start?
Z S pw(S s)_(W((*s==32)|(*s==9)|(*s==13),s++)s)                                                     //skip whitespace: space, tab, CR (so CRLF/tabbed .k files parse; \n stays the statement separator)
Z A1(p1,x&&xn==1?fir(x):x)                                                                          //singleton list to atom
S pID(S s)_(W(id1(*s),s+=*s<0&&(UC)s[1]>>6==2?2:1)s)                                                //parse identifier
W pu(S*p)_(S s=*p;W v=0;C c=*s;W(C09(c),v=10*v+c-'0';c=*++s)*p-s?*p=s,v:NL)                         //parse unsigned long
L pl(S*p)_(B m=**p=='-';*p+=m;(L)((W)(1-2*m)*pu(p)))                                                        //parse long
Z B ovf;                                                                                            //set by plN: an integer literal of 2^63 or more in magnitude
Z L plN(S*p)_(S t=*p+(**p=='-');L v=pl(p);W(*t=='0'&&C09(t[1]),t++)I n=*p-t;I(n>19||n==19&&strncmp(t,"9223372036854775807",19)>0,ovf=1)
 !v&&**p=='N'?(*p)++,NL:v)                                                                          //parse long (with support for nulls)
// amber: pu() signals "no digits consumed" by returning NL, and pl() then
// propagates that as a full-range long. Feeding it straight into the int
// exponent accumulator (`e+=pl(&s)`) is signed overflow -- undefined
// behaviour reachable from a malformed literal such as `1.5e` or `1.5each`,
// found by tests/fuzz.py under UBSan. pf now reads the exponent itself, saturating,
// and no longer calls pl.
//
// amber 2.2: an exponent outside the power table's range used to return EARLY,
// before `*p=s`, so `1e-309` left `e-309` in the input ('value) and every
// subnormal double was unwritable as a literal -- a round-trip hole, since std.k's
// text ser/deser is `k followed by eval. The cursor now always moves past the
// literal.
//
// pf scans the literal (digits, `.` and digits, `e` and an exponent) and builds its
// significand and exponent as it goes; csv_cl (src/csv.h) converts them exactly, inline,
// where Clinger's fast path takes them (one IEEE multiply or divide, or none), and pfs
// otherwise, out of line: the Eisel-Lemire method (csv_fast). Of a literal of more than
// 19 digits the significand keeps the first 19, and csv_span (src/csv.c) decides from
// them where it can; csv_float otherwise, which is exact too. So a literal, `F$ and
// a CSV cell with the same digits give the same double, and a float printed and read
// back is the same float. (pf was pf and pfu; it is one function now, which calls
// nothing on its common paths, so it needs no stack frame: pfs takes the rest.)
Z W pfn(S s,S t)_(W u=0;I e=0;W(s<t,I(!e&&(u<922337203685477580ull||u==922337203685477580ull&&*s<'8'),u=10*u+(W)(*s-'0'))E(e=1)s++)u)   //0n's payload: the integer digits, as they were read before
Z NI L pfs(S b,S s,W w,L x,W g,L n)_(F v;I(n<20?!csv_fast(w,x,&v):!csv_span(w,x,&v),v=csv_float(b,s));L r;MC(&r,&v,8);r|g)   //the rest of pf, out of line so that pf needs no stack frame: at most 19 digits that Clinger's path cannot take (the Eisel-Lemire method), more than 19 (w the first 19, or 0 when csv_float reads them all), or no fast path built
L pf(S*p)_(W g=(W)(**p=='-')<<63;S b=*p+!!g,s=b;W w=0;W(C09(*s),w=10*w+(W)(*s-'0');s++)L x=0,i=s-b,n=i;   //parse float: g the sign, w the first 19 digits, n the count of all, x the exponent; i the integer digits (past 19 w wraps)
 C c=*s;P(c=='w',*p=s+1+(s[1]=='f');g|WFL)P(c=='n',*p=s+1+(s[1]=='f');g|(pfn(b,s)^NFL))P(c=='N'&&!(n>19?pfn(b,s):w),*p=s+1+(s[1]=='f');NFL)   //the null 0N has no sign: -0N is 0n, as in ngn/k
 I(c=='.',S f=++s;W(C09(*s)&&s-f<19-i,w=10*w+(W)(*s-'0');s++)x=f-s;W(C09(*s),s++)n+=s-f;c=*s)   //fraction digits past the 19th only count
 I(c=='e',B q=*++s=='-';s+=q;L d=0;W(C09(*s),I(d<1000000000000000ll,d=10*d+*s-'0')s++)x+=q?-d:d)   //an exponent with no digits counts as 0; it saturates far past any offset the digits can make up for
 *p=s+(*s=='f');F v;P(n<20&&csv_cl(w,x,&v),L r;MC(&r,&v,8);r|g)pfs(b,s,i>19?0:w,x,g,n))   //exact in one step, else pfs; the bits through memcpy (a pointer cast can lose them under -fno-signed-zeros)
Z A pV(C t,TY(pl)*f)_(L a[1<<9];U n=0;A x=0;                                                  //parse ints or floats (in chunks of 512)
 W(1,S q=s;W(*q-'0'<2u,q++)                                                                     //a boolean token (01b) in the strand: its bits
   I(q>s&&*q=='b'&&!CA9(q[1])&&q[1]-'.',F(q-s,I(n==L(a),A c_=aV(t,n,a);x=x?cat11(x,c_):c_;n=0)F b=s[i]-'0';a[n++]=t==tF?*(L*)&b:s[i]-'0')s=q+1)
   E(L v=f(&s);I(n==L(a),A c_=aV(t,n,a);x=x?cat11(x,c_):c_;n=0)a[n++]=v)
   S p=pw(s);B(p==s||!num(p))s=p)A c_=aV(t,n,a);x?cat11(x,c_):c_)
Z A0(pZ,S p=s;W(*p-'0'<2u,p++)                                                                      //parse ints
 P(*p=='B',S t=s;s=p+1;cB(aV(tG,p-t,t)))//todo
 P(*p=='b',S t=s;s=p+1;cG(cB(aV(tG,p-t,t))))
 A x=pV(tL,plN);B o=ovf;ovf=0;P(!x,0)P(o,x(ez0()))sqzZ(x))   //the flag is cleared whatever pV returns, so a failed literal cannot fail the next one
Z A0(pF,S p=s;W(*p-'0'<2u,p++)I((*p=='b'||*p=='B')&&C09(p[1]),B d=0;C c=*++p;W(CA9(c)||c=='.'||c==':',d|=!!strchr(".nwef",c);c=*++p)P(!d,pZ()))pV(tF,pf))   //parse floats; a
 //bool token run into an int (01b1 2.5) is a noun of its own that the strand after it indexes, as before an int strand
 //(pZ) and in ngn/k, which reads each token of a strand by itself: run into a float (01b2.5, 01b0w) it stays 'value
Z A0(pC,C a[1<<9];U n=0;C c=*++s;A x=0;                                                      //parse "string" (in chunks of 512)
 W(c&&c-'"',I(n==L(a),A c_=aV(tC,n,a);x=x?cat11(x,c_):c_;n=0)I(c=='\\',c=*++s;B(!c)U i=fG("tnr0",4,c);I(i<4,c="\t\n\r"[i]))a[n++]=c;c=*++s)
 P(!c,x?x(ep0()):ep0())s++;A c_=aV(tC,n,a);x?cat11(x,c_):c_)
Z A0(p0x,S p=s;W(CA9(*p),p++)A x=N(unhC(s,p-s));s=p;x)                                              //parse 0x string
// amber 2.7: `:path/to/file is one symbol, as in q: after `: come letters, digits and . / _ - :
Z B pfh(C d)_(CA9(d)||d=='.'||d=='/'||d=='_'||d=='-'||d==':')
Z A0(ps,S p=s;C c=*s;I(id0(c),s=pID(s))J(c>>7,W(*++s<-64)s+=*s==':')J(c==':',W(pfh(*++s)))aCm(p,s))  //parse symbol
// amber 2.2: `w` says whether whitespace may PRECEDE an item.
// A parameter list may: `{[a; b]x}` and `{[ a;b]x}` are ordinary spellings and
// used to be a bare syntax error pointing at the whole lambda, which is a
// miserable thing to hand someone pasting an example.
// A backtick symbol VECTOR may NOT: `a `b is two separate symbols, and
// skipping the space there would silently fuse them into one two-item vector.
// Trailing space before the closing bracket is handled by pp() below, because
// the loop breaks without consuming it.
Z A pSw(C c,B w)_(I a[256];U n=0;A x=0;                                                     //parse symbols (in chunks of 256)
 W(1,I(n==L(a),A c_=aV(tS,n,a);x=x?cat11(x,c_):c_;n=0)s++;I(w,s=pw(s))A y=*s-'"'?ps():N(pC(),I(x,mr(x)));y=str0(y);a[n++]=us(yC);y(0);S p=pw(s);B(*p-c)s=p)
 A c_=aV(tS,n,a);x?cat11(x,c_):c_)
Z A pS(C c)_(pSw(c,0))
Z A0(pP,I a[8];U n=0;                                                                               //parse dot-separated path of identifiers
 W(1,P(n>=L(a),ez0())A y=str0(ps());a[n++]=us(yV);y(0);B(*s-'.'||!id0(s[1]))++s)
 aV(tS,n,a))
Z A0(pp,P(*s-'[',au)A x=N(pSw(';',1));s=pw(s);P(*s-']'||!xn,ep(x))P(xN>8,ez(x))s++;ppe=s;x)                          //Parse parameter list
Z S pws(S s)_(W(*s==32||*s==10,s++)s)                                                               //skip spaces and newlines
Z A amkl(CO A*e,U n)_(A x=aA1(MKL);F(n,PSH(x,e[i]))x)                                                //make-list node (e0;e1;..)
Z A amcg(C end,U*np)_(I nm[256];A ex[256];U n=0;s=pws(s);                                            //parse `name:expr;..` group up to end -> (names ! (e0;e1;..))
 W(*s-end,P(!id0(*s),ep0())A y=str0(N(ps()));nm[n]=us(yC);y(0);s=pws(s);P(*s-':',ep0())s++;
  C v=0;ex[n++]=N(pe(0,&v));s=pws(s);I(*s==';',s++)s=pws(s))
 s++;*np=n;aA3(EXC,qte(aV(tS,n,nm)),amkl(ex,n)))
Z A0(amtbl,s++;U nk,nv;A kd=N(amcg(']',&nk)),vd=N(amcg(')',&nv));                                    //table literal ([keys]cols) ; s at '['
 A vt=aA2(FLP,vd);P(!nk,vt)aA3(EXC,aA2(FLP,kd),vt))                                                  //unkeyed:+names!cols  keyed:keytable!valtable
// amber: civil date -> days since 2000.01.01 (Howard Hinnant, epoch-shifted; matches temporal.k ymd2d); "D"$ and "P"$ use it too (below)
Z L ymd2days(L y,L m,L d){L wy=y-(m<=2);L era=(wy>=0?wy:wy-399)/400;L yoe=wy-era*400;L mp=m+(m>2?-3:9);L doy=(153*mp+2)/5+d-1;return era*146097+365*yoe+yoe/4-yoe/100+doy-730425;}
// amber: temporal-literal scanner.  Fires only on unambiguous patterns:
//   HH:MM[:SS[.mmm]]              -> time atom (ms of day)
//   YYYY.MM.DD                    -> date atom (days; needs TWO dots, so floats X.Y are untouched)
//   YYYY.MM.DDD HH:MM:SS.fffffffff-> timestamp atom (ns).  Returns 0 (s unchanged) on no match.
// Issue #18: the fields are checked, as q does: a month of 1 to 12, a day that month has, minutes and
// seconds below 60 (hours are not limited: 99:00:00.000 is a time, and a timestamp's 24:00 is the next
// day). A literal that fails is 'parse; before, 2026.02.29 read as 2026.03.01. A time is 32 bits of milliseconds,
// so one past 596:31:23.647 is 'limit (issue #62; it wrapped, 1000:00:00.000 read as -193:02:47.296), and a date is 32 bits
// of days, so one past 5881610.07.11 is 'limit, as "D"$ (it wrapped: 100000000.01.01 read as -5832990.07.06), and a timestamp
// 64 bits of nanoseconds, so one past 2292.04.10D23:47:16.854775807 is 'limit, as "P"$ (it wrapped: 2300.01.01D00:00:00.000 read
// as 1715.06.13D00:25:26.290448384). tdd holds a field at 1e15 or more, as "T"$, "D"$ and "P"$ do (tdg), so a minute, second, month,
// day or a timestamp's hour of 20 digits does not wrap (10:18446744073709551617:00.000 read as 10:01:00.000). tbad: 1 'parse, 2 'limit.
Z C tbad;Z W tdd(W v,C c){return v<(W)1e15?10*v+(W)(c-'0'):v;}Z B dok(W y,W m,W d){if(m<1||m>12||d<1)return 0;W n=m==2?28+(y%4==0&&(y%100!=0||y%400==0)):30+((m+(m>7))&1);return d<=n;}
Z C tng;Z A pTmp(){S p=s;if(!C09(*p))return 0;W a=0;S q=p;while(C09(*q)){a=10*a+(W)(*q-'0');q++;}
 if(*q==':'){S e=q++;W mi=0;while(C09(*q)){mi=tdd(mi,*q);q++;}W sc=0,ms=0;
  if(*q==':'){q++;while(C09(*q)){sc=tdd(sc,*q);q++;}if(*q=='.'){q++;I nd=0;while(C09(*q)&&nd<3){ms=10*ms+(W)(*q-'0');q++;nd++;}while(nd<3){ms*=10;nd++;}while(C09(*q))q++;}}
  if(mi>59||sc>59){tbad=1;return 0;}S z=p;W(z<e-1&&*z=='0',z++)if(e-z>19||a>596||3600000*a+60000*mi+1000*sc+ms>2147483647)I(!tng||e-z>19||3600000*a+60000*mi+1000*sc+ms-2147483648u,tbad=2;return 0)s=q;return atm((I)(3600000*a+60000*mi+1000*sc+ms));}
 if(*q=='.'){S q2=q+1;if(!C09(*q2))return 0;W mo=0;while(C09(*q2)){mo=tdd(mo,*q2);q2++;}if(*q2!='.')return 0;q2++;if(!C09(*q2))return 0;W dy=0;while(C09(*q2)){dy=tdd(dy,*q2);q2++;}
  W y=0;S r=p;W(r<q,y=tdd(y,*r);r++)if(!dok(y,mo,dy)){tbad=1;return 0;}S z=p;W(z<q-1&&*z=='0',z++)if(q-z>19||a>9999999){tbad=2;return 0;}L days=ymd2days((L)a,(L)mo,(L)dy);   //y: the year held at 1e15 as "D"$ holds it (a wraps past 64 bits)
  if(*q2=='D'){q2++;W hh=0;while(C09(*q2)){hh=tdd(hh,*q2);q2++;}if(*q2!=':')return 0;q2++;W mi=0;while(C09(*q2)){mi=tdd(mi,*q2);q2++;}W sc=0,ns=0;
   if(*q2==':'){q2++;while(C09(*q2)){sc=tdd(sc,*q2);q2++;}if(*q2=='.'){q2++;I nd=0;while(C09(*q2)&&nd<9){ns=10*ns+(W)(*q2-'0');q2++;nd++;}while(nd<9){ns*=10;nd++;}while(C09(*q2))q2++;}}
   if(mi>59||sc>59){tbad=1;return 0;}__int128 t=(__int128)days*86400000000000ll+(__int128)hh*3600000000000ll+60000000000ll*(L)mi+1000000000ll*(L)sc+(L)ns;
   if(t-(L)t){tbad=2;return 0;}s=q2;return antp((L)t);}
  if(days-(I)days){tbad=2;return 0;}s=q2;return adt((I)days);}
 return 0;}
//A strand of temporal literals of one kind (2026.01.01 2026.01.02) is a list of them, as q; it was 'type, the second
//applied to the first. Another kind, or anything else after the space, ends it (digest #65)
//A - before a time literal is part of it, as q and as -10 for an int (#83 Q8): -10:00:00.000 was the verb on a time,
//'type, so a negative time's text did not read back. Dates and timestamps keep the verb, as q. tng lets
//pTmp take one more millisecond, so -596:31:23.648 is a time, as "T"$ reads it
Z A pTng()_(S o=s,q=s+1;W(C09(*q),q++)P(*q-':'||q==s+1,0)s++;tng=1;A b=pTmp();tng=0;P(!b,s=o;0)atm(0u-(U)(I)b))
Z A pTms(A a){A z=0;UC k=_t(a);for(;*s==' ';){S o=s;while(*s==' ')s++;A b=pTmp();I(!b&&!tbad&&*s=='-'&&k==ttm,b=pTng())if(!b){s=o;tbad=0;break;}if(_t(b)!=k){mr(b);s=o;break;}if(!z){z=emp(tA);PSH(z,MKL);PSH(z,a);}PSH(z,b);}return z?z:a;}   //As (d1;d2) parses: MKL and the items
// amber 2.0.0: identifiers usable INFIX like a verb -- `x in y`, `t lj kt`,
// `1 within 2 3`, `"/" sv parts` -- as well as the bracket form in[x;y].  ngn/k
// already treats every unicode-named identifier (pt's `c>>7` branch) as an infix
// verb; this extends that to a curated set of Amber's two-argument library dyads.
// The `*s!=':'` guard at the call site keeps the name an ordinary lvalue while it
// is being DEFINED (`in:{...}` in amber.k) or amended.
Z B infixkw(S p,U n)_(static CO C*const kw[]={"in","within","like","lj","ij","uj","aj","aj0","wj","wj1","pj","ej","cross","inter","union","except","ss","sv","vs","xasc","xdesc"};F(L(kw),P(SL(kw[i])==n&&!memcmp(kw[i],p,n),1))0)
// amber 2.2: the parameter names of the lambda currently being parsed.
//
// The infix decision above is made from the NAME, and until now from nothing
// else -- so a name that is a PARAMETER of the enclosing lambda was still read
// as a verb:
//     f:{[r;ss] ... 1_ss ... }
// parsed `ss` as the string-search verb, which makes `1_ss` a projection rather
// than a drop. The function then silently returned its input. 21 names are
// affected and none of them errors.
//
// Only the INNERMOST frame is consulted, and that is exactly right rather than
// a simplification: k has no closures, so a name that is an outer lambda's
// parameter genuinely is a global reference when it appears inside an inner
// lambda, and reading it as the verb there is the correct behaviour.
Z A pfrm;
Z B isparm(S p,U n){
 I(!pfrm||_t(pfrm)!=tS,return 0)
 U m=_n(pfrm);
 F(m,S q=su((U)_I(pfrm)[i]);I(SL(q)==n&&!memcmp(q,p,n),return 1))
 return 0;}
Z A pt(C*v)_(C c=*s;                                                                                //parse term
 P(c=='`',qte(p1(N(pS('`')))))
 P(c=='"',p1(pC()))
 P(c=='[',s++;pb(GAP,']'))
 P(c=='(',s++;P(*s=='[',amtbl())P(*s==')',s++;emp(tA))A x=N(pb(MKL,')'));xn-2?x:las(x))
 P(c=='{',C k0=k;k=1;S s1=s0,t=s0=s++;A y=N(pp());
  // The body is parsed with this lambda's parameters in scope, so pt() below
  // does not read one of them as an infix verb. Saved and restored rather than
  // just cleared, because lambdas nest. An implicit-argument lambda has no
  // parameter list yet (y is au here; x/y/z are filled in after the body is
  // parsed), and none of those three single letters is an infix name.
  A pf0=pfrm;pfrm=y==au?0:y;
  A z=pb(GAP,'}');
  pfrm=pf0;
  P(!z,s0=s1;y(0))I(y==au,y=aS(k);F(3,yi='x'+i))A x=N(cpl(aCn(t,s-t),z,y));s0=s1;k=k0;x)
 // A name reads as an infix verb when it is one of the curated keywords or a
 // rank-2 global -- but NOT when it is a parameter of the lambda being parsed,
 // and NOT when the program has rebound it to something that is not a rank-2
 // function. Without those two exclusions the reading is made from the name
 // alone, and `{[r;ss] 1_ss}` or a top-level `ss:5` silently turned a drop into
 // a projection. `*s!=':'` still keeps the name an ordinary lvalue while it is
 // being defined or amended.
 P(id0(c),S p=s;A x=N(pP());I(s-p==1&&c-'y'<2u,k=MAX(k,c-'w'))
  I((infixkw(p,s-p)||am_infix_dyad(p,s-p))&&*s!=':'&&!isparm(p,s-p)&&!am_name_nonfn(p,s-p),*v=1)AO(p-s0,x))
 P(C09(c)&&s[1]==':',B u=s[2]==':';s+=2+u;U i=20+c-'0';P(i>25,ep0())*v=1;Lt(tv-u)|i)
 P(c=='0'&&s[1]=='x',s+=2;p1(p0x()))
 P(num(s)&&(c-'-'||s==s0||s==ppe||(!id1(s[-1])&&!strchr(")]}\"",s[-1]))),   //ppe: just past a lambda's [params], whose ] is no noun (digest #40)
  A tlit=c-'-'?pTmp():pTng();P(tlit,pTms(tlit))P(tbad,C b=tbad;tbad=0;b>1?ez0():ep0())
  B d=0,f=1;S p=s;c=*p;W(1,S q=p;p=pw(p);B(!f&&p==q||!num(p))f=0;p+=*p=='-';c=*p;B(!CA9(c))W(CA9(c)||c=='.'||c==':',d|=!!strchr(".nwef",c);c=*++p))p1(d?pF():pZ()))
 P(c>>7,S p=s;A x=N(pP());*v=1;AO(p-s0,x))
 U i=si("'/\\",c);P(i<3,c=*++s;B h=c==':';s+=h;*v=1;aw+i+3*h)i=si(vc,c);P(i>19,GAP)
 B u=*++s==':';s+=u;*v=1;Lt(tv-u)|i)
Z X1(pm,                                                                                            //monadify
 Rv(x^au^av)
 RA(I(xx==aw,x=mut(x);xA[xn-1]=pm(xA[xn-1]))x)
 Rs(S s=su(xv);U n;P(*s>>7&&s[(n=SL(s))-1]-':',C b[n+2];MC(b,s,n);b[n]=':';b[n+1]=0;sym(b))x)
 RS(xn==1?enl(pm(fir(x))):x)
 R_(x))
// Nesting depth: pe() recurses for each bracket, lambda and verb of a chain, and pT() deepens the
// tree once for each adverb or index applied to a term; the folding and compiling after the parse
// recurse as deep as the tree. Without a limit, deep input ran the C stack out (20000 parentheses,
// or 6000 terms of 1+1+...). 512 levels (a chain takes two for each verb) run in a 512 KB stack,
// and the bytecode buffer refuses chains long enough to reach them anyway.
#define PD 512
Z I pd;                                                                                             //depth of pe() calls in progress
Z A pT(C*v)_(A x=N(pt(v));U n=0;                                                                    //parse term and the adverbs or square brackets after it (v:verb?)
 W(1,C c=*s;U i=si("'/\\[",c);P(i>3,x)P(pd+ ++n>PD,x(ez0()))s++;
  I(i>2,x=AO(s-1-s0,N(pb(x,']')));I(xn==2,I(xy==GAP,xy=au)E(xx=pm(xx)))*v=0)
  E(I c=*s==':';s+=c;x=aA2(aw+i+3*c,x);*v=1))x)
Z A pe(A,C*);
Z A pe_(A x,C*v)_(s=pw(s);C c=*s;                                                                    //parse expression
 I(c=='/'&&(s==s0||s[-1]==32||s[-1]==10),
  I(s[1]==10,C*e=strstr(s+1,"\n\\\n");P(!e,ep0())s=e+2)
  E(W((c=*++s)&&c-10)))
 P(s>s0&&*s=='\\'&&s[-1]==32,s++;A y=pe(0,v);P(!y,x?x(0):0);*v=0;y=aA2(OUT,y);I(x,y=aA2(pm(x),y))y)
 UH o=s-s0;C b=0;A y=pT(&b);P(!y,x?x(0):0)P(y==GAP,x?x:y)
 P(!b,A z=pe(y,v);P(!x,z)Nx(z);*v?aA3(aw,x,z):AO(o,aA2(pm(x),z)))
 A z=pe(0,v);P(!z,y(x?x(0):0))P(z==GAP,*v=1;P(!x,y)Yu(ep(x))AO(o,aA3(y,x,z)))
 *v&=y!=av;I(!x,y=pm(y))*v?aA3(aw,x?AO(o,aA3(y,x,GAP)):y,z):AO(o,x?aA3(y,x,z):aA2(pm(y),z)))
Z A pe(A x,C*v)_(P(pd>=PD,x?x(ez0()):ez0())pd++;A r=pe_(x,v);pd--;r)                         //pe_, at most PD deep: 'limit beyond
Z A pb(A x,C c)_(x=x?aA1(x):emp(tA);                                                                //parse body (sequence of ;-separated expressions)
 W(1,C v=0;A y=Nx(pe(0,&v));PSH(x,c-']'&&y==GAP?au:y);P(y==GAP&&c==')',ep(x))B(*s-';'&&*s-10)B(c==10&&*s==10)s++)
 P(c==10&&!*s,x)P(*s-c,ep(x))s++;x)
Z A pk_(S*p,C c)_(s0=s=*p;ppe=0;A x=pb(GAP,c);*p=s;P(x,xn==2?las(x):x)eD(s0,SL(s0),s-s0);eQ(s0,SL(s0),s-s0);0)                  //parse either a group of lines (c='\n') or till '\0' (c='\0')
A pk(S*p,C c)_(P(!ray_rc_sync,pk_(p,c))plk(1);A x=pk_(p,c);plk(0);x)                               //pk_ under the peach parse lock (m.c plk)
// "D"$ "T"$ "P"$ (issue #62): the text as the literal reader reads it, its fields checked as there, else 'parse (they rolled
// over, and "D"$"abc" made a date of a function's address). A time also takes a sign and one to three fields ("10", "10:00"),
// a date a sign on its year, a timestamp a date alone; past the range of the type 'limit. Blanks around the text (spaces, tabs,
// carriage returns, as pw skips) are skipped, as q skips them. Out of line and after the parser, so the parser's hot loops keep
// their place in the binary.
Z B tdg(S*p,W*v)_(S s=*p;W u=0;W(C09(*s),u=tdd(u,*s);s++)*v=u;B n=s>*p;*p=s;n)   //digits: 1 if any; their value, held at 1e15 or more past it (tdd)
Z B ttx(S*p,W*h,W*m,W*c,W*f)_(S s=*p;*m=*c=*f=0;P(!tdg(&s,h),1)I(*s==':',s++;P(!tdg(&s,m),1)I(*s==':',s++;P(!tdg(&s,c),1)))   //H[:M[:S]][.f]: f in ns; 1 if bad
 I(*s=='.',s++;W k=0;W(C09(*s),I(k<9,*f=10**f+(W)(*s-'0'))k++;s++)W(k<9,*f*=10;k++))*p=s;*m>59||*c>59)
Z B tdx(S*p,L*d)_(S s=*p;B n=*s=='-';s+=n;W y,m,e;P(!tdg(&s,&y)||*s-'.',1)s++;P(!tdg(&s,&m)||*s-'.',1)s++;P(!tdg(&s,&e),1)L Y=n?-(L)y:(L)y;   //[-]Y.M.D: days; 1 if bad
 P(m<1||m>12||e<1||e>(m==2?28+(Y%4==0&&(Y%100!=0||Y%400==0)):30+((m+(m>7))&1)),1)*p=s;*d=ymd2days(Y,(L)m,(L)e);0)
NI A ptT(A x)_(P(_t(x)-tC,et(x))x=str0(x);S s=pw(xV);B n=*s=='-';s+=n;W h,m,c,f;P(ttx(&s,&h,&m,&c,&f)||*pw(s),x(ep0()))W v=h>596?WL:3600000*h+60000*m+1000*c+f/1000000;P(v>2147483647u+n,x(ez0()))x(atm(n?-(L)v:(L)v)))
NI A ptD(A x)_(P(_t(x)-tC,et(x))x=str0(x);S s=pw(xV);L d;P(tdx(&s,&d)||*pw(s),x(ep0()))P(d-(I)d,x(ez0()))x(adt(d)))
NI A ptP(A x)_(P(_t(x)-tC,et(x))x=str0(x);S s=pw(xV);L d;W h=0,m=0,c=0,f=0;P(tdx(&s,&d),x(ep0()))I(*s=='D',s++;I(*pw(s),P(ttx(&s,&h,&m,&c,&f),x(ep0()))))P(*pw(s),x(ep0()))   //a date alone, or "D" and nothing, is midnight
 __int128 t=(__int128)d*86400000000000ll+(__int128)h*3600000000000ll+60000000000ll*(L)m+1000000000ll*(L)c+(L)f;P(t-(L)t,x(ez0()))x(antp((L)t)))
