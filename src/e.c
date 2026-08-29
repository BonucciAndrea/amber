#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include"diagnostic.h"
#include<stdlib.h>
// Thread-local error scratch: each peach worker that raises an error formats it
// into its OWN buffer (the parent re-raises a single clean 'worker error, so a
// worker's text is only ever read by that same worker). `r` is (re)pointed at
// `b` by err0() at the start of every error -- which always runs before eS(),
// try(), epr() or err() read the buffer -- so it needs no static initializer
// (a thread-local one cannot take the address of another thread-local anyway).
Z AM_TLS C b[4096];Z AM_TLS C*r;Z AM_TLS U d;
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
// amber 1.9.4: set to 1 by eS() once a rich diagnostic has been written to
// stderr for the CURRENT error, cleared by err0() when a new error starts and
// by try() when one is consumed. epr() consults it so an uncaught error is
// printed exactly ONCE: previously the Rust-style report went to stderr at
// creation time AND the legacy ngn/k caret block in b[] was dumped to stderr
// again on the way out, so every failing script printed the same error twice in
// two different formats. The buffer b[] itself is untouched -- .[f;a;h] handlers
// and `err still receive the identical string they always did.
I amdiagshown=0;

// ---- error catalogue -------------------------------------------------------
// One row per category in a.h's ERR macro. err0() has already written the
// category name into b[] by the time eS() runs (every call site in b.c/j.c does
// e*0() and then eS()), so the lookup needs no new plumbing through the
// interpreter -- the name is read straight back out of the error buffer.
typedef struct{CO C*name,*code,*title,*label,*help;}EDIAG;
Z CO EDIAG edtab[]={
 {"value","E0101","Undefined variable or value","not found in this scope",
  "Verify that the variable is defined in the current scope or check for typos."},
 {"type","E0102","Type mismatch or invalid operand type","wrong type for this operation",
  "Check operand types using `type or `@. Primitive operations require compatible types."},
 {"length","E0103","Vector length mismatch","operands have different counts",
  "Conforming operations require vectors of matching lengths or atom-vector pairs."},
 {"domain","E0104","Out of domain operation","outside this operation's domain",
  "The input value falls outside the mathematical or logical domain of this function (e.g. division by zero, log of negative number)."},
 {"parse","E0105","Syntax or parse error","unexpected token here",
  "Check for unbalanced brackets (), [], {}, unterminated strings, or invalid syntax."},
 {"index","E0106","Index out of bounds","index is outside the vector",
  "Ensure the index is non-negative and strictly less than the length of the vector/table."},
 {"rank","E0107","Function rank mismatch","wrong number of arguments",
  "Function called with the wrong number of arguments for its defined rank."},
 {"limit","E0108","Resource or allocation limit exceeded","exceeded a limit here",
  "The evaluation exceeded maximum allowable vector sizes, recursion depths, or memory limits."},
 {"io","E0109","Input/output failure","this operation failed",
  "Check that the target file path or network address exists and has appropriate permissions."},
 {"stack","E0110","Call stack depth exceeded","recursion continues here",
  "A function recursed deeper than the interpreter's call stack allows -- check for a missing base case."},
 {"compile","E0111","Expression could not be compiled","could not be compiled",
  "The expression parses but cannot be compiled; check argument counts, reserved names and assignment targets."},
 {"nyi","E0112","Operation not implemented for these types","unsupported for these types",
  "This primitive has no implementation for the given operand types yet -- see docs/MISSING.md."}};
// Exposed for the `dgn self-test so it can assert the whole category->code
// matrix without having to capture stderr.  which: 0=code 1=title 2=label 3=help.
CO C*edinfo(CO C*nm,I which);
Z B seq_(CO C*a,CO C*b)_(W(*a&&*a==*b,a++;b++)*a==*b)
Z CO EDIAG*edlook(CO C*nm)_(F(L(edtab),I(seq_(nm,edtab[i].name),return edtab+i))(CO EDIAG*)0)
CO C*edinfo(CO C*nm,I which)_(CO EDIAG*e=edlook(nm);P(!e,(CO C*)0)which==0?e->code:which==1?e->title:which==2?e->label:e->help)
// identifier byte: Amber names are alphanumeric plus _ and . (namespaces).
Z B idch_(C c)_(c=='_'||c=='.'||(c>='0'&&c<='9')||(c>='a'&&c<='z')||(c>='A'&&c<='Z'))
// etok: widen a bare byte offset to the full token that starts/covers it, so
// the underline spans `prices` rather than pointing at one letter of it. Falls
// back to a single character for operators, and covers the whole literal for a
// quoted string (which is the unit a reader thinks in).
Z V etok(CO C*s,U n,U i,U*a,U*b){
 U p=i,q=i;
 if(i>=n){*a=i;*b=i+1;return;}
 if(idch_(s[i])){while(p&&idch_(s[p-1]))p--;while(q<n&&idch_(s[q]))q++;}
 else if(s[i]=='"'){q=i+1;while(q<n&&s[q]!='"'){if(s[q]=='\\'&&q+1<n)q++;q++;}if(q<n)q++;}
 else q=i+1;
 *a=p;*b=q>p?q:p+1;}
NI A err0(S s)_(amdiagshown=0;r=b;d=0;N n=MIN(SL(s),32);r=b;*r++='\'';MC(r,s,n);r+=n;*r++=10;0)
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
// eS renders the source location of a runtime/parse/type error as a single
// rich, Rust-style diagnostic on stderr: category-specific code and title, the
// offending TOKEN underlined across its full width with an inline label, and an
// actionable help note (see edtab above).  It also still fills b[] with the
// compact caret text via eQ(), because that buffer is what .[f;args;handler]
// hands to a trap handler and what `err returns -- changing it would change a
// documented, test-covered contract.  What changed in 1.9.4 is that b[] is no
// longer ALSO dumped to stderr afterwards (see epr): the error prints once.
// AMBER_DIAG=0 or `diag 0 suppresses the report, in which case epr() falls back
// to printing the compact form so an error is never silently swallowed.
NI V eD(CO C*esrc,U en,U i){I(amdiag<0,amdiag=({S d=getenv("AMBER_DIAG");!d||*d!='0';}))
 I(amdiag,
  C sb[1024];U n=en<SZ sb-1?en:SZ sb-1;MC(sb,esrc,n);sb[n]=0;U io=i<n?i:n;
  // recover the category name err0() just wrote as "'<name>\n"
  // NOTE: locals here deliberately avoid g.h's bare accessor-macro namespace
  // (ul, xn, tl, ... are all macros meaning "field of x/u/t"), hence the dg
  // prefixes -- a plain `ul` here expands to _l(u) and will not compile.
  C dgnm[32];U dgk=0;
  I(r>b&&*b=='\'',U dgj=1;W(dgj<(U)(r-b)&&b[dgj]!=10&&b[dgj]!=' '&&dgk<SZ dgnm-1,dgnm[dgk++]=b[dgj++]))
  dgnm[dgk]=0;
  CO EDIAG*dgc=edlook(dgnm);
  U dga,dgb;etok(sb,n,io,&dga,&dgb);
  // Title: name the offending token where doing so is unambiguous. For an
  // undefined name that is the whole story ("Undefined variable `r`"), so it is
  // promoted into the title; other categories keep their category title and
  // carry the token in the inline label instead.
  C dgt[256];U dgtl=0;CO C*dgbase=dgc?dgc->title:"Evaluation error";
  U dgbl=SL(dgbase);I(dgbl>SZ dgt-1,dgbl=SZ dgt-1)MC(dgt,dgbase,dgbl);dgtl=dgbl;
  I(dgc&&seq_(dgnm,"value")&&dgb>dga&&idch_(sb[dga]),
    CO C*dgp="Undefined variable `";U dgpl=SL(dgp);U dgm=dgb-dga;
    I(dgpl+dgm+2<SZ dgt,MC(dgt,dgp,dgpl);dgtl=dgpl;MC(dgt+dgtl,sb+dga,dgm);dgtl+=dgm;dgt[dgtl++]='`'))
  dgt[dgtl]=0;
  Span sp=span_at(sb,dga,dgb);
  report_diagnostic_ex_stderr(dgc?dgc->code:"E0100",dgt,"<amber>",sp,dgc?dgc->label:"here",
                              0,0,dgc?dgc->help:"Run \\ast <expr> to inspect how this expression parses.",0);
  amdiagshown=1)}
// eS: the A-valued wrapper used by the compiler/evaluator call sites.
NI V eS(A x/*0*/,U i)_(eD(xV,xn,i);eQ(xV,xn,i))
A3(try,/*100*/x=x(dot(x,yR));P(x,x)I(ztU,z=z1(aCn(b,r-b)))E(zR)r=b;d=0;amdiagshown=0;z)
// Print the compact caret block ONLY if no rich diagnostic was already shown
// for this error -- otherwise the same failure appears twice, once per format.
void am_ln_sb_capture(const char*,unsigned long);// ln.c: tee errors into the scroll-back ring
A1(epr,I(!amdiagshown,write(2,b,r-b);am_ln_sb_capture(b,r-b))amdiagshown=0;r=b;x)
A1(err,XC(x=str0(x);err1(x,xV))P(x==au,aCn(b,r-b))err1(x,"err"))
NI A die(S s)_(U n=SL(s);C v[n+1];MC(v,s,n);v[n]=10;write(1,"'",1);write(2,v,n+1);exit(1);0)

#define M(t,m)\
 NI A0(e##t##0,err0(    #m))\
 NI A1(e##t   ,err1(x,  #m))\
 NI AA(e##t##8,err8(a,n,#m))
ERR
