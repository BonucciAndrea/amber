#include <stdio.h>
/* Feature-test macro must precede every system header (a.h pulls in
 * <unistd.h> on its very first line) or a strict `-std=c99` build hides
 * BSD-ism declarations like MAP_ANON that mm() below already relies on.
 * This does not change any evaluation/memory logic -- it only unlocks
 * declarations the code already uses. */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include"arena.h"
#include<unistd.h>
#include"inspect.h" // \v rich variable inspector
#include"ast.h"     // \ast AST visualizer
#include"trace.h"   // \trace execution profiler
#include"vm.h"      // \disasm bytecode disassembler
#if defined(__x86_64__)
 #define AMARCH "x86-64"
#elif defined(__aarch64__)
 #define AMARCH "arm64"
#elif defined(__i386__)
 #define AMARCH "x86"
#else
 #define AMARCH "unknown"
#endif
#if defined(__clang__)
 #define AMCC __VERSION__
#elif defined(__GNUC__)
 #define AMCC "gcc " __VERSION__
#else
 #define AMCC "cc"
#endif
#include<fcntl.h>
#include<sys/mman.h>
#ifndef MAP_NORESERVE
 #define MAP_NORESERVE 0
#endif
#ifdef __LP64__
 #define AP(p) ((A)(p))
#else
 #define AP(p) ((A)(U)(p)) //A from pointer
#endif
#ifdef shared
__attribute((weak, visibility("default"))) V kinit();
#endif

// ---- lock-free per-thread allocation for peach's thread pool ---------------
// Amber's object allocator has two parts with very different access frequency:
//
//   1. bkt[] -- the size-class free lists that every aF()/aL()/aC()/an() hits.
//      This is now THREAD-LOCAL (see `Z AM_TLS A bkt[24]` below): each peach
//      worker keeps its own free lists, so the hot alloc (mb: pop a chunk) and
//      free (m0: push a chunk) paths take NO lock at all -- they are 100%
//      lock-free per thread. A chunk allocated by one worker and freed by
//      another simply migrates onto the freeing thread's list; that is safe
//      because a live chunk is only ever touched by its single current owner,
//      and the size class travels in the chunk header (_b), so any thread can
//      recycle it.
//
//   2. reg[] -- the mmap region table, touched only when a size class has no
//      free chunk and a fresh OS region must be carved (mm), or an oversized/
//      file-backed region retired (mu/mc). That is rare and amortized, so it
//      keeps ONE recursive lock (mm()->mc() nest on the same thread), engaged
//      only inside a peach scope (ray_rc_sync). us() (symbol intern) shares it.
//
// So during a parallel map the per-element allocation traffic never serializes;
// only the occasional region growth does. ALK()/AUL() compile to a single
// predictable branch outside a peach scope, so serial execution pays nothing.
#if !defined(wasm)
#include<pthread.h>
Z pthread_mutex_t g_alloc_mx;
Z V alloc_lock_init(){pthread_mutexattr_t a;pthread_mutexattr_init(&a);pthread_mutexattr_settype(&a,PTHREAD_MUTEX_RECURSIVE);pthread_mutex_init(&g_alloc_mx,&a);pthread_mutexattr_destroy(&a);}
#define ALK() do{if(ray_rc_sync)pthread_mutex_lock(&g_alloc_mx);}while(0)
#define AUL() do{if(ray_rc_sync)pthread_mutex_unlock(&g_alloc_mx);}while(0)
#else
#define alloc_lock_init() ((void)0)
#define ALK() ((void)0)
#define AUL() ((void)0)
#endif

Z ST{V*p;W n;B f;}reg[128];Z U nreg;Z UC pnd[128];Z U npnd;
Z V mc(){ALK();I(npnd,F(npnd,U j=pnd[i];munmap(reg[j].p,reg[j].n);reg[j].p=0)npnd=0;U j=0;F(nreg,I(reg[i].p,MC(reg+j,reg+i,SZ*reg);j++))nreg=j)AUL();}
Z A mu(V*p){ALK();F(nreg,I(reg[i].p==p,pnd[npnd++]=i;AUL();return 0;))AUL();return die("UNMAP");}
Z V*mm(W n,U f){ALK();V*p=mmap(0,n,PROT_READ|PROT_WRITE,MAP_NORESERVE|MAP_PRIVATE|MAP_ANON,-1,0);I(p==MAP_FAILED,AUL();return(V*)0;)I(nreg==L(reg),mc();I(nreg==L(reg),die("MMAP")))reg[nreg++]=(TY(*reg)){p,n,f};AUL();return p;}
A mf(U f,U i,U n)_(V*p=mm(pg+n,1);P(!p,eo0())P(mmap(p+pg,n,PROT_READ|PROT_WRITE,MAP_NORESERVE|MAP_PRIVATE|MAP_FIXED,f,i)!=p+pg,mu(p);eo0())A x=AP(p+pg);xb=0;xr=REFB;xT=tC;xn=n;x)

// Per-thread size-class free lists: each peach worker recycles chunks on its
// OWN lists with zero locking. Thread-local storage is initial-exec here (no
// -fPIC/shared), i.e. a single %fs-relative load, so the serial path is as fast
// as the old global array. Only a free-list miss (mb -> mm) or an oversized
// free (m0 -> mu) reaches the reg[] lock.
Z AM_TLS_IE A bkt[24];DBG(Z U lck;)
Z W cap(A x/*0*/)_((HD<<xb)-HD)
Z A mb(U i){I(i>=L(bkt),V*p=mm(HD<<i,0);P(!p,die("OOM"))return AP(p+HD);)A x=bkt[i];I(x,bkt[i]=xX;DBG(xX=0;)return x;)x=mb(i+1);A y=x+(HD<<i);MS(yV-HD,0,HD);yb=i;yX=bkt[i];bkt[i]=y;return x;}
// release one reference (r0). The decrement is atomic in a peach scope (RC_DECV)
// and plain otherwise; only the LAST owner (previous count == REFB) frees. The
// bkt[] push is thread-local and lock-free; only the oversized/file-backed
// paths (mu) touch the shared region table under its lock.
A m0(A x){DBG(lck++;)Q(x)XP(0)I(RC_DECV(x)>REFB,return 0;)
 I(TR(xT),mrn(xn|!xn,xA);xT=tL)U i=xb;
 I(!i,return mu(xV-pg);)I(i>=L(bkt),return mu(xV-HD);)
 xX=bkt[i];bkt[i]=x;xr=0;return x;}
DBG(A1(m1,lck--;P(!x||!xb,0)MS(xV,0xab,cap(x));xn=-1;xT=0;0))
A1(_R,Q(x)XP(x)RC_INC(x);x)
A1(mr,DBG(m1)(m0(x)))
V mRn(U n,CO A*a){F(n,_R(a[i]))}
V mrn(U n,CO A*a){F(n,mr(a[i]))}
A1(mRa,mRn(xn,xA);x)

NI A an(U n,C t)_(Q(!lck)Q(tA<=t)Q(t<tn)Q(!TP(t))U i=58-CLZ(HD|HD-1+(((W)n<<Tw[t])+7>>3));A x=mb(i);xb=i;xr=REFB;xT=t;xn=n;_at(x)=0;x)
A aV(C t,U n,CO V*v)_(A x=an(n,t);MC(xV,v,((W)n<<Tw[t])+7>>3);x)
A aa(U n,A x/*1*/)_(P(MINE(x)&&((W)n<<xw)+7>>3<=cap(x),AN(n,x))A y=an(n,xt);MC(yV,xV,((W)xn<<Tw[xt])+7>>3);I(ytR,I(MINE(x),AZ(x))E(mRn(xn,xA)))x(y))//realloc
A aA0(U n)_(A x=AN(0,aA(n));xx=emp(tC);x)
A1(aA1,aV(tA,1,&x))
A2(aA2,/*11*/aV(tA,2,A(x,y)))
A3(aA3,/*111*/aV(tA,3,A(x,y,z)))
A2(aM,/*11*/Q(xtMT)Q(ytA )Q(xN==yN)aV(tM,2,A(x,y)))
A2(am,/*11*/Q(xtMT)Q(ytMT)Q(xN==yN)aV(tm,2,A(x,y)))
A aA(U n)_(an(n,tA))
A aB(U n)_(an(n,tB))
A aG(U n)_(an(n,tG))
A aI(U n)_(an(n,tI))
A aL(U n)_(an(n,tL))
A aF(U n)_(an(n,tF))
A aC(U n)_(an(n,tC))
A aS(U n)_(an(n,tS))
A aCn(S s,U n)_(aV(tC,n,s))
A aCm(S p,S q)_(aCn(p,q-p))
A aCz(S s)_(aCn(s,SL(s)))
A az(L n)_(n-(I)n?al(n):ai(n))
A al(L v)_(aV(tl,1,&v))
A af(F v)_(aV(tf,1,&v))
A aE(L i,L j)_(Q(i<=j)P(i==j,emp(tG))A x=an(tE,2);*xL=i;xL[1]=j;x)
A1(mut,XP(x)P(MINE(x),x)x=x(aV(xt,xn,xV));XR(mRa(x))x)
C tZ(L v)_(G(tL,tL,tL,tL,tI,tI,tH,tG)[CLZ(v^v>>63|1)-1>>3])
A kv(A*p)_(A x=*p;Q(xn==2);P(!MINE(x),--xr;*p=_R(xx);_R(xy))*p=xx;AZ(x);x(xy))
L gl_(A x)_(XP(xv)*xL)
L gl(A x)_(L v=gl_(x);x(0);v)
F gf(A x)_(F v=*xF;x(0);v)
A AT(W t,A x)_(Q(t<tn);P(TP(t),Lt(t)|-1ull<<56&x)xT=t;x)
A AW(C w,A x)_(Q(w<6u);xE=w;x)
A AK(C k,A x)_(Q(k<9u);xk=k;x)
A AO(UC o,A x)_(Xs(x&~(0xffll<<32)|(W)o<<32)_O(x)=o;x)
A AN(U n,A x)_(xn=n;x)
A1(AZ,xT=tG;x)

Z C s0[1<<16],*s1=s0+1;
S su(U u)_(P(u&1u<<31,s0-(I)u)Z W r;r=u;(V*)&r)
// Symbol intern. Short symbols (<=4 bytes without the high bit) are packed into
// the id itself and never touch the shared table, so they need no lock. The
// table-append path (scan for an existing name, else copy the bytes and bump
// s1) mutates s0/s1 shared across peach workers -- serialized by the allocator
// lock, engaged only in a ray_rc_sync scope.
U us(S s){U n=SL(s);I(n<4||(n==4&&!(s[3]&128)),U v=0;MC(&v,s,n);return v;)
 ALK();S p=s0+1;W(p<s1,I(!strcmp(p,s),U r=(U)(s0-p);AUL();return r;)p+=SL(p)+1)
 n++;I(s1+n>s0+SZ s0,AUL();die("SYMS");)MC(s1,s,n);s1+=n;U r=(U)(s0-s1+n);AUL();return r;}
A sym(S s)_(as(us(s)))

Z U gd,gn;Z W gk[4096];A gv[4096];
Z W gkk(A x/*0*/)_(Xs((U)xv)Q(xtS)xn?(W)_v(jS(drp(-1,xR)))<<32|(U)_v(ii(x,xn-1)):0)
U gi(A x/*0*/)_(W k=gkk(x);I(!(k>>32)&&id0(*su(k)),k|=(W)gd<<32)U i=fL(gk,gn,k);P(i<gn,i)P(gn>=L(gv),die("GLOBALS"))gk[gn]=k;gv[gn]=0;gn++)
A gg(A x/*1*/)_(//get value of global
 P(xtS&&!xn,x(0);A x=emp(tS),y=emp(tA);F(gn,I(gv[i],L k=gk[i];PSH(x,k-(U)k?jS(aV(tS,2,A((I)(k>>32),k))):as(k));PSH(y,_R(gv[i]))))am(x,y))//special case for 0#`
 W k=gkk(x);x(0);U i=fL(gk,gn,k);i<gn&&gv[i]?_R(gv[i]):ev0())
A*gp(A x/*1*/)_(U i=gi(x);x(0);gv+i)//get pointer to global
A gns(U k)_(I a[L(gk)];U n=0;F(gn,I(gk[i]>>32==k,a[n++]=gk[i]))aV(tS,n,a))//list namespace

// try_rewrite: apply the K-level `qrw` SQL-syntax rewriter (defined in
// qsql.k, e.g. `select sym,px from t`) to raw input text, IF qrw is
// currently defined (i.e. the stdlib has been loaded). Copies the
// rewritten text into `buf` (size `n`) and returns it; falls back to the
// untouched `raw` pointer if qrw isn't loaded or the rewrite itself fails,
// so it is always safe to call. Used by \trace (trace.c) so tracing a
// SQL-style query traces the plain-K expression it rewrites to -- exactly
// what the interactive REPL's own line1 already does (see repl.k).
S try_rewrite(S raw, C *buf, N n) {
    A nm = sym("qrw");
    W k = gkk(nm);
    U i = fL(gk, gn, k);
    if (i >= gn || !gv[i]) return raw;      // qsql.k not loaded: leave as-is
    A f = _R(gv[i]);
    A y = _1(f, aCz(raw));                  // qrw[raw] -> rewritten string (or 0 on error)
    if (!y || _t(y) != tC) { if (y) mr(y); return raw; }
    N m = _n(y); if (m >= n) m = n - 1;
    MC(buf, _C(y), m); buf[m] = 0;
    mr(y);
    return buf;
}

Z A bs0(S s)_(en0())
Z A bsbs(S s)_(exit(0);0)
Z A bscd(S s)_(P(!*s,C b[256];getcwd(b,SZ b)?eo0():aCz(b))chdir(s)?eo0():au)
Z A bsd(S s)_(P(!*s,as(gd))s+=*s=='.';gd=us(s);au)
  A bsl(S s)_(I f=open(s,0,0);A x=u1c(ai(f));close(f);N(x);P(!xn,x(au))C*p=xC,*e=p+xn-1;P(*e-10,x(err0("eoleof")))*e=0;I(*p=='#'&&p[1]=='!',p=strchrnul(p,10);p+=!!*p)x(evs(p,1)))
Z A bsf(S s)_(K1("{`0:($!h),'\":\",'`k'. h:(&x=^`o`p`q`r`u`v`w`x?@'h)#h:``repl_.:0#`}",ai(!s)))
Z A bst(S s)_(L n=s[-1]=='t'&&*s==':'?++s,pl(&s):1;S p=s;A x=N(pk(&p,10));x=N(cpl(aCm(s,p),x,0));L t=now();F(n,mr(Nx(run(x,0,0))))x(az((now()-t+500)/1000)))
// \v: walk the global symbol table (gk/gn/gd -- file-local to m.c) and hand
// each non-function global to inspect.[ch] for classification/formatting.
Z V iv_render(V){iv_begin();F(gn,I(gv[i],A x=gv[i];I(TU(_t(x)),continue)W k=gk[i];U ns=(U)(k>>32),sy=(U)k;C nb[80];I(ns,snprintf(nb,SZ nb,"%s.%s",su(ns),su(sy)))E(snprintf(nb,SZ nb,"%s",su(sy)))iv_add(nb,x)))iv_print();}
Z A bsv(S s)_(iv_render();au)
// \ast <expr>: parse-only tree visualizer (ast.h). Never compiles/runs `s`.
Z A bsast(S s)_(ast_cmd(s))
// \trace <expr>: timed parse/compile/exec/print wrapper (trace.h). Evaluates
// `s` exactly like a normal line, plus prints a phase-timing breakdown.
Z A bstrc(S s)_(trace_cmd(s))
// \disasm <expr>: compile-only bytecode disassembler (vm.h). Never runs `s`.
Z A bsvmd(S s)_(vm_disasm_cmd(s))
Z A bs_(S*p)_(C b[256];S s=*p,e=strchrnul(s,10);P(e-s+1>=L(b),ez0())MC(b,s,e-s);b[e-s]=0;*p=e+!!*e;C c=*b,d=b[1];P(c=='c'&&d=='d'&&(!b[2]||b[2]==32),bscd(b+2+(b[2]==32)))
 P(!strncmp(b,"trace",5)&&(!b[5]||b[5]==32),bstrc(b+5+(b[5]==32)))
 P(!strncmp(b,"disasm",6)&&(!b[6]||b[6]==32),bsvmd(b+6+(b[6]==32)))
 P(!strncmp(b,"ast",3)&&(!b[3]||b[3]==32),bsast(b+3+(b[3]==32)))
 P(!d||d==10||d==32||d==':',G(&bsl,bst,bsd,bsbs,bsf,bsv,bsm,bs0)[si("ltd\\fvm",c)](b+1+(d==32)))K1("0x0a\\`x(,,\"/bin/sh\"),,:",aCz(b)))

Z A evs1(S*p)_(S s=*p;P(*s=='\\',++*p;bs_(p))A x=pk((V*)p,10);N(x);x=N(cpl(aCm(s,*p),x,0));x(run(x,0,0)))
A evs(S s,B r)_(W(*s,A x=evs1(&s);P(!x,I(r,s=strchrnul(s,10);s+=!!*s;epr(0))0)I(r,x(out(x)))E(P(!*s,x)x(0))mc();arena_reset())au)//arena_reset: rewind HFT scratchpad at end of each eval cycle
B rep()_(Z C b[256];C*s=b,*q;
 W(1,L n=read(0,s,b-s+SZ b);P(n<=0,0)s+=n;q=memchr(s-n,10,n);
     P(q,C*p=b;W(q,*q=0;evs(p,1);p=q+1;q=memchr(p,10,s-p))MC(b,p,s-p);s+=b-p;1)
     P(b+SZ b<=s,die("LONGLINE")))1)
V repl(){W(rep())}

A cns,cn[tn];Z A ce[tn];S*argv,*env;
V kinit(){Z B l;P(l)l=1;alloc_lock_init();pg=sysconf(_SC_PAGESIZE);A b[32],*c=b;
 F(tS-tA+1,*c++=ce[tA+i]=an(0,tA+i))*c++=ce[tm]=am(emp(tS),emp(tA));_x(ce[tA])=_R(ce[tC]);ce[tM]=ce[tA];F(tn-ti,Q(!ce[i+ti]);ce[i+ti]=ce[tA])//empties
 cn[tA]=ce[tC];*c++=cn[ti]=cn[tl]=al(NL);F(tL-tE+1,cn[tE+i]=cn[ti])*c++=cn[tF]=cn[tf]=af(NF);cn[tC]=cn[tc]=ac(32);cn[tS]=cn[ts]=as(0);F(tn-to,cn[to+i]=au)//nulls
 Q(c-b<=32);cns=aV(tA,c-b,b);arena_init(0);}//arena_init: reserve the 16MB HFT scratchpad
V kargs(I n,S*a){argv=(S*)a;env=(S*)a+n+1;n=MAX(0,n-2);A x=n?aA(n):emp(tA);F(n,xa=aCz(a[2+i]))gk[gn]='x';gv[gn++]=x;}
A emp(U t)_(_R(ce[t]))

ZN U ow(S s,U n)_(write(1,s,n))
ZN V o8(W v){C b[16],*s=b;F(16,C c=v>>4*(15-i)&15;*s++="0W"[9<c]+c)ow(b,16);}
U os(S s)_(ow(s,SL(s)))
W ov_(S s,W v)_(os(s);o8(v);ow("\n",1);v)
ZN V od(L v){C b[32];ow(b,sl(b,v)-b);}
ZN V osd(S s,L v){os(s);od(v);}
ZN A1(ox,o8(x);osd(" b",xb);C t=xT;os(" t");I(LH(1,t,tn),ow(&TS[t],1))E(od(t))osd(" r",xr);osd(" n",xn);F(MIN(5,cap(x)/8),os(" ");o8(xl))os("\n");x)
// amber: engine metadata -> (heapBytes; nRegions; arch; compiler; version)
// (used by the REPL banner and by `amber --version`; version comes from
// AMBER_VERSION in a.h so there is exactly one place to bump on a release)
A1(binfo,L tot=0,nr=0;F(nreg,I(reg[i].p,tot+=reg[i].n;nr++))A a[]={al(tot),al(nr),aCz(AMARCH),aCz(AMCC),aCz(AMBER_VERSION)};x(aV(tA,5,a)))
#define RGS(a...) F(nreg,B f=reg[i].f;V*p=reg[i].p,*q=f?p:p+reg[i].n;a)
#define OBS(a...) RGS(A x=(A)(p+HD*!f+pg*f),y=(A)q;W(x<y,a;x+=HD<<xb))
#define XYS(a...) OBS(I(xtR,F(xn|!xn,A y=xa;a)))
#define RTS(a...) {A x=cns;a;F(gn,I(x=gv[i],a))}
A bsm(S s)_(XYS(I(!ytP,yr--))RTS(I(!xtP,xr--))OBS(I(xr,os("!refc:");ox(x)))RTS(I(!xtP,xr++))XYS(I(!ytP,yr++))
 OBS(I(xT>=tn,os("!type:");ox(x)))OBS(I(xtA&&!xn&&!xx,os("!prot:");ox(x)))XYS(I(!yt,os("!dngl:");ox(x);ox(y)))au)
