#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
Z C b[4096],*r=b;Z U d;
Z S etag;//last raw builtin error tag (for the value-error name hint); 0 for custom errors
Z S edsc(S s){//plain-English description for a raw error tag, or 0 if unknown
 ST{S k,v;}m[]={{"value","undefined name or empty value"},{"type","wrong type for this operation"},
  {"length","operands have mismatched lengths"},{"rank","wrong number of arguments to a function"},
  {"index","index out of range"},{"domain","value outside the valid domain for this operation"},
  {"nyi","not implemented for these types"},{"parse","malformed expression"},
  {"compile","expression could not be compiled"},{"stack","call depth exceeded (recursion too deep)"},
  {"io","input/output error (missing file or handle)"},{"limit","implementation limit reached"}};
 F(SZ(m)/SZ(*m),I(!strcmp(s,m[i].k),return m[i].v))return 0;}
// err0: build "'tag: plain-English description".  Runs for scripts and the REPL alike,
// so every error explains itself, not just labels itself.
NI A err0(S s)_(r=b;d=0;S e=edsc(s);etag=e?s:0;N n=MIN(SL(s),32);*r++='\'';MC(r,s,n);r+=n;I(e,*r++=':';*r++=' ';N k=SL(e);MC(r,e,k);r+=k)*r++=10;0)
ZN A err1(A x,S s)_(x(0);err0(s))
ZN A err8(CO A*a,U n,S s)_(mrn(n,a);err0(s))
NI V eQ(S s,U n,U i){
 I(++d>=5,I(d==5,MC(r," ..\n",4);r+=4)return)
 S p=s+i,q=p,t=p;U h=64,o=1;
 W(s<p&&t-h<p&&p[-1]&&p[-1]-10,p--;o+=(*p&0xc0)!=0x80)
 W(q<s+n&&q<=t+h&&*q&&*q-10,q++)
 *r++=32;MC(r,p,q-p);
 I(p<=t-h,*r=r[1]='.')
 I(q>t+h,r[q-p-2]=r[q-p-1]='.')
 r+=q-p;*r++=10;MS(r,32,o);r+=o;*r++='^';*r++=10;
 // for an undefined-name error, name the offending token right under the caret
 I(etag&&d==1&&!strcmp(etag,"value"),U a=i,e2=i;W(a>0&&(CA9(s[a-1])||s[a-1]==95),a--)W(e2<n&&(CA9(s[e2])||s[e2]==95),e2++)
  I(e2>a,MC(r,"  hint: `",9);r+=9;MC(r,s+a,e2-a);r+=e2-a;MC(r,"` is not defined\n",17);r+=17))}
NI V eS(A x/*0*/,U i)_(eQ(xV,xn,i))
A3(try,/*100*/x=x(dot(x,yR));P(x,x)I(ztU,z=z1(aCn(b,r-b)))E(zR)r=b;d=0;z)
A1(epr,write(2,b,r-b);r=b;x)
A1(err,XC(x=str0(x);err1(x,xV))P(x==au,aCn(b,r-b))err1(x,"err"))
NI A die(S s)_(U n=SL(s);C v[n+1];MC(v,s,n);v[n]=10;write(1,"'",1);write(2,v,n+1);exit(1);0)

#define M(t,m)\
 NI A0(e##t##0,err0(    #m))\
 NI A1(e##t   ,err1(x,  #m))\
 NI AA(e##t##8,err8(a,n,#m))
ERR
