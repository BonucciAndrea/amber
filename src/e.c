#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include"diagnostic.h"
#include<stdlib.h>
Z C b[4096],*r=b;Z U d;
// amber: is the Rust-style stderr diagnostic (src/diagnostic.c) enabled?
//   -1 = not resolved yet, resolve from $AMBER_DIAG on first use
//    0 = off, 1 = on
// Errors are rendered at CREATION time, which means a caller that goes on to
// catch the error with .[f;args;handler] has already had the full diagnostic
// splashed across stderr. That is right for an interactive line and wrong for
// code whose whole job is to provoke errors (a test suite's must-raise cases,
// `protect`, any retry loop), which drowned in red for errors it handled
// perfectly. `diag 0 turns the report off and returns the previous setting;
// the compact caret text is unaffected -- it is buffered in b[] and still
// handed to the trap handler and to `err, so nothing is lost.
I amdiag=-1;
NI A err0(S s)_(r=b;d=0;N n=MIN(SL(s),32);r=b;*r++='\'';MC(r,s,n);r+=n;*r++=10;0)
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
 r+=q-p;*r++=10;MS(r,32,o);r+=o;*r++='^';*r++=10;}
// eS renders the source location of a runtime/parse/type error.  All such errors
// funnel through here, so they are ALSO shown as a Rust-style diagnostic
// (src/diagnostic.c) on top of the compact caret line.  This is ON by default;
// set AMBER_DIAG=0 in the environment, or call `diag 0 at runtime, to suppress
// the diagnostic report (the compact caret line is unaffected either way).
// The source noun (xV) is length-prefixed, not NUL-terminated, so copy a bounded,
// NUL-terminated slice before handing it to the (C-string) formatter.
NI V eS(A x/*0*/,U i)_(I(amdiag<0,amdiag=({S d=getenv("AMBER_DIAG");!d||*d!='0';}))
 I(amdiag,
  C sb[1024];U n=xn<SZ sb-1?xn:SZ sb-1;MC(sb,xV,n);sb[n]=0;U io=i<n?i:n;
  Span sp=span_at(sb,io,io+1);report_diagnostic_stderr("E0001","evaluation error","<amber>",sp,0,0,"the caret marks the offending token"))
 eQ(xV,xn,i))
A3(try,/*100*/x=x(dot(x,yR));P(x,x)I(ztU,z=z1(aCn(b,r-b)))E(zR)r=b;d=0;z)
A1(epr,write(2,b,r-b);r=b;x)
A1(err,XC(x=str0(x);err1(x,xV))P(x==au,aCn(b,r-b))err1(x,"err"))
NI A die(S s)_(U n=SL(s);C v[n+1];MC(v,s,n);v[n]=10;write(1,"'",1);write(2,v,n+1);exit(1);0)

#define M(t,m)\
 NI A0(e##t##0,err0(    #m))\
 NI A1(e##t   ,err1(x,  #m))\
 NI AA(e##t##8,err8(a,n,#m))
ERR
