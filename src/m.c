/* Feature-test macros MUST come before the first system header of the
 * translation unit -- <stdio.h> used to sit above them, which silently
 * defeated both of them under `cc -std=c99`. */
/* ---- portability preamble: MUST precede every system header in this TU ----
 * Defining _POSIX_C_SOURCE puts Darwin's headers into STRICT POSIX mode, which
 * hides the BSD extensions this file relies on -- MAP_ANON above all others.
 * That is exactly the macOS CI failure: source that compiles clean against
 * glibc fails on Apple clang with "MAP_ANON undeclared here". _DARWIN_C_SOURCE
 * puts those declarations back; _GNU_SOURCE and _DEFAULT_SOURCE do the
 * equivalent job on glibc/musl. All three are purely ADDITIVE -- they only ever
 * unhide declarations, so none of them can change behaviour. (Verified: this
 * tree calls no function whose semantics _GNU_SOURCE alters, i.e. no
 * strerror_r, basename or qsort_r.)
 * These must sit above the first #include of the translation unit, not merely
 * above <sys/mman.h>: any system header may pull in <features.h> first and
 * latch the mode for the whole compilation. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include <stdio.h>
/* _DEFAULT_SOURCE unlocks the BSD-isms (MAP_ANON) that mm() below relies on;
 * _POSIX_C_SOURCE 200809L unlocks PTHREAD_MUTEX_RECURSIVE, which glibc guards
 * behind __USE_UNIX98/__USE_XOPEN2K8.  Both are set at the very top of the
 * file -- amber 1.9.5; before that they sat below <stdio.h> and had no effect
 * on a strict `-std=c99` build, which therefore did not compile at all.
 * Neither macro changes a line of evaluation or memory logic. */
#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include"ln.h" // native line editor (replaces the old raw read + rlwrap)
#include"ext.h"
#include<stdlib.h>
#include"arena.h"
#include<unistd.h>
#include<errno.h>
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
#include<sys/stat.h>
/* MAP_ANON is the historical BSD spelling and MAP_ANONYMOUS the POSIX-ish one;
 * which of the two a platform exposes depends on the feature macros above, so
 * accept either and normalise to MAP_ANON, which mm() uses. The final fallback
 * is Linux's numeric value -- no target this project supports lacks BOTH
 * macros, so it exists only so the build cannot fail on an unknown one. */
#ifndef MAP_ANON
#  ifdef MAP_ANONYMOUS
#    define MAP_ANON MAP_ANONYMOUS
#  else
#    define MAP_ANON 0x20
#  endif
#endif
/* MAP_NORESERVE is Linux-only in practice (macOS defines it as a no-op); where
 * it is absent, 0 is the correct neutral flag. */
#ifndef MAP_NORESERVE
 #define MAP_NORESERVE 0
#endif
#ifdef __LP64__
 #define AP(p) ((A)(p))
#else
 #define AP(p) ((A)(U)(p)) //A from pointer
#endif
#ifdef AMBER_SHARED
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
Z pthread_mutex_t g_alloc_mx,g_parse_mx;
Z V alloc_lock_init(){pthread_mutexattr_t a;pthread_mutexattr_init(&a);pthread_mutexattr_settype(&a,PTHREAD_MUTEX_RECURSIVE);pthread_mutex_init(&g_alloc_mx,&a);pthread_mutex_init(&g_parse_mx,&a);pthread_mutexattr_destroy(&a);}
#define ALK() do{if(ray_rc_sync)pthread_mutex_lock(&g_alloc_mx);}while(0)
#define AUL() do{if(ray_rc_sync)pthread_mutex_unlock(&g_alloc_mx);}while(0)
// amber 2.3: the parser (p.c: s0/s/k) and the compiler (b.c: u/b/m/l/nb/nl) keep
// their state in file statics, so two peach workers doing . "..." at once parsed
// over each other: bogus 'parse, then a segfault (2.2 too). pk() and cpl() take
// this lock inside a peach scope, so parses queue up while the compiled code still
// runs in parallel. Recursive because pk() compiles lambda literals itself.
V plk(B on){if(ray_rc_sync)(on?pthread_mutex_lock:pthread_mutex_unlock)(&g_parse_mx);}
#else
V plk(B on){(V)on;}
#define alloc_lock_init() ((void)0)
#define ALK() ((void)0)
#define AUL() ((void)0)
#endif

Z ST{V*p;W n;B f;}reg[1024];Z U nreg;Z U pnd[1024];Z U npnd;   //mapped regions: 1024 (was 128, issue #9)
Z V mc(){ALK();I(npnd,F(npnd,U j=pnd[i];munmap(reg[j].p,reg[j].n);reg[j].p=0)npnd=0;U j=0;F(nreg,I(reg[i].p,MC(reg+j,reg+i,SZ*reg);j++))nreg=j)AUL();}
Z A mu(V*p){ALK();F(nreg,I(reg[i].p==p,pnd[npnd++]=i;AUL();return 0;))AUL();return die("UNMAP");}
// amber 2.2: every region of 2MB or more asks for transparent huge pages. A
// fresh 4K page costs a fault each (1-4us under a hypervisor: WSL2, cloud VMs),
// so a program that touches 80MB of new vectors paid 20k faults before it did
// any work; with 2MB pages that is 40. No-op where THP is "never" or absent.
Z V*mm(W n,U f){ALK();V*p=mmap(0,n,PROT_READ|PROT_WRITE,MAP_NORESERVE|MAP_PRIVATE|MAP_ANON,-1,0);I(p==MAP_FAILED,AUL();return(V*)0;)
#ifdef MADV_HUGEPAGE
 I(n>=(W)1<<21,madvise(p,n,MADV_HUGEPAGE);)
#endif
I(nreg==L(reg),mc();I(nreg==L(reg),die("MMAP")))reg[nreg++]=(TY(*reg)){p,n,f};AUL();return p;}
// amber 2.3: Cygwin can't lay a file view over part of an anonymous mapping
// (Windows sections don't nest), so the MAP_FIXED below always failed there and
// every 0:, 1: and \l read came back 'io. When it fails, read into a plain vector.
#if !defined(wasm)
// (a GiB a call: macOS's pread refuses more than INT_MAX bytes, so a read of 2 GiB or more was 'io; again after a signal,
// as the line editor's SIGWINCH handler is not SA_RESTART)
Z A mfr(U f,W i,W n)_(A x=an((U)n,tC);W o=0;W(o<n,L r=pread(f,(C*)_V(x)+o,MIN(n-o,(W)1<<30),(off_t)(i+o));I(r<0&&errno==EINTR,continue)P(r<=0,mr(x);eo0())o+=(W)r)x)
#else
Z A mfr(U f,W i,W n)_(eo0())
#endif
// amber 2.7: the on-disk columns (src/dsk.c). n>0 bytes of file f from offset i (a multiple of the page size)
// mapped as a payload, a header page in front, 64-bit sizes; 0 when the map fails and the caller reads instead.
// Private and copy-on-write, so an in-place write never reaches the file; freed like mf's (bucket 0).
#if !defined(wasm)
A mfw(I f,W i,W n){P(!n||i%(W)pg,0)V*p=mm(pg+n,1);P(!p,0)
 I(mmap((C*)p+pg,n,PROT_READ|PROT_WRITE,MAP_NORESERVE|MAP_PRIVATE|MAP_FIXED,f,(off_t)i)!=(C*)p+pg,mu(p);mc();return 0;)
 A x=AP((C*)p+pg);xb=0;xr=REFB;return x;}
#else
A mfw(I f,W i,W n){(V)f;(V)i;(V)n;return 0;}
#endif
// n<2^32 bytes of file f from offset i: both 64-bit (they were unsigned int, so an offset of 2^32+k read at k)
A mf(U f,W i,W n)_(V*p=mm(pg+n,1);P(!p,eo0())P(mmap(p+pg,n,PROT_READ|PROT_WRITE,MAP_NORESERVE|MAP_PRIVATE|MAP_FIXED,f,(off_t)i)!=p+pg,mu(p);mfr(f,i,n))A x=AP(p+pg);xb=0;xr=REFB;xT=tC;xn=(U)n;x)

// Per-thread size-class free lists: each peach worker recycles chunks on its
// OWN lists with zero locking. Thread-local storage is initial-exec here (no
// -fPIC/shared), i.e. a single %fs-relative load, so the serial path is as fast
// as the old global array. Only a free-list miss (mb -> mm) or an oversized
// free (m0 -> mu) reaches the reg[] lock.
Z AM_TLS_IE A bkt[24];DBG(Z U lck;)
// Colour. A block is aligned to its size (HD<<b) within a page-aligned region, so every block of 64 KB or more
// put its payload 64 bytes past a multiple of the page, and of the L1 set stride; big vectors used together (a
// window's input and output, a group-by's columns) fought over the same L1 sets. anc() moves such a payload, with
// its header, c whole 64-byte lines into its block's spare room (no change of class): a thread's n-th large block
// gets c=CU*(n*199 mod CP/64/CU), cut down to leave 32 bytes after the payload (writers that work in 32-byte
// groups, the gathers iC..o8 among them, write up to 31 bytes past n). Whole lines keep payloads 64-byte aligned
// (amber item 8).
// c is kept in a spare header byte (_cl), at the payload and at the block's start, where the heap walk (OBS) finds
// it. The payload's own header gives its class as one less: half the block is always less than the room after
// the payload, so cap() and aa()'s in-place test are unchanged. The block's start keeps the true class, and m0()
// moves the header back there (aunc), so the free lists and mb()'s splitting only see uncoloured blocks. Small
// blocks keep their path: below CB, an() takes a block from its free list itself (mbs), and m0() sends class CB-1
// and up out of line (m0c). Mapped files (b 0) and blocks past the classes are never coloured.
// CP, the period, is the L1 set stride. CU, the step: on x86 a loop that reads one vector and writes another slows
// by 4-48% when the two sit one or two lines apart mod 4 KB (4K aliasing: Zen 3 when the output is one line
// before, Intel when it is one or two after); steps of 3 lines keep any two differently coloured large blocks at
// least 3 lines apart there.
#if defined(__aarch64__)
#define CP 16384//Apple M-series 128 KB 8-way; Neoverse N1/N2/V1 and Cortex-A76 64 KB 4-way; Cortex-A72 32 KB 2-way
#define CU 1    //256 colours
#else
#define CP 4096 //x86: 32-48 KB, 8-12-way
#define CU 3    //21 colours, 0 to 60 lines
#endif
_Static_assert(CP/64<=256,"a colour is one byte, in lines");
#define CB 10   //smallest coloured class (64 KB blocks)
Z AM_TLS_IE U acn;//large blocks so far (this thread)
V acs(U i){acn=i;}//a peach worker starts its colours at its number (1 up; the main thread at 0), so workers do not hand out one sequence in step
Z W cap(A x/*0*/)_((HD<<xb)-HD)
Z A mb(U i){I(i>=L(bkt),V*p=mm(HD<<i,0);P(!p,die("OOM"))return AP(p+HD);)A x=bkt[i];I(x,bkt[i]=xX;DBG(xX=0;)return x;)x=mb(i+1);A y=x+(HD<<i);MS(yV-HD,0,HD);yb=i;yX=bkt[i];bkt[i]=y;return x;}
NI Z A aunc(A x)_(A y=x-((W)_cl(x)<<6);MC((V*)(y-32),(V*)(x-32),32);_cl(y)=0;_b(y)++;y)//m0: a coloured block's header back to its start, with its true class
// release one reference (r0). The decrement is atomic in a peach scope (RC_DECV)
// and plain otherwise; only the LAST owner (previous count == REFB) frees. The
// bkt[] push is thread-local and lock-free; only the oversized/file-backed
// paths (mu) touch the shared region table under its lock.
NI Z A m0c(A x,U i){I(i>=L(bkt),return mu(xV-HD);)I(_cl(x),x=aunc(x);i=xb)xX=bkt[i];bkt[i]=x;xr=0;return x;}//m0's large blocks: oversized, or coloured
A m0(A x){DBG(lck++;)Q(x)XP(0)I(RC_DECV(x)>REFB,return 0;)
 I(TR(xT),mrn(xn|!xn,xA);xT=tL)U i=xb;
 I(!i,return mu(xV-pg);)I(__builtin_expect(i>=CB-1,0),return m0c(x,i);)
 xX=bkt[i];bkt[i]=x;xr=0;return x;}
DBG(A1(m1,lck--;P(!x||!xb,0)MS(xV,0xab,cap(x));xn=-1;xT=0;0))
A1(_R,Q(x)XP(x)RC_INC(x);x)
A1(mr,DBG(m1)(m0(x)))
V mRn(U n,CO A*a){F(n,_R(a[i]))}
V mrn(U n,CO A*a){F(n,mr(a[i]))}
A1(mRa,mRn(xn,xA);x)

Z A mbs(U i){A x=bkt[i];I(x,bkt[i]=xX;DBG(xX=0;)return x;)return mb(i);}//mb() below class CB: its free list, else mb() splits
NI Z A anc(U i,W nb,U n,C t){A x=mb(i);I(i<L(bkt),W r=(HD<<i)-HD-nb,c=acn++*199u%((CP>>6)/CU);r=r>32?(r-32>>6)/CU:0;I(c>r,c%=r+1)c*=CU;
 I(c,_b(x)=i;_cl(x)=c;x+=c<<6;MS((V*)(x-HD),0,HD);_cl(x)=c;i--))xb=i;xr=REFB;xT=t;xn=n;_at(x)=0;return x;}//an's large blocks, coloured
NI A an(U n,C t)_(Q(!lck)Q(tA<=t)Q(t<tn)Q(!TP(t))W nb=((W)n<<Tw[t])+7>>3;U i=58-CLZ(HD|HD-1+nb);P(__builtin_expect(i>=CB,0),anc(i,nb,n,t))A x=mbs(i);xb=i;xr=REFB;xT=t;xn=n;_at(x)=0;x)
A aV(C t,U n,CO V*v)_(A x=an(n,t);MC(xV,v,((W)n<<Tw[t])+7>>3);x)
// realloc. Grown in place for its sole owner, who then writes the new tail --
// so no attribute survives it (amber 2.3: `s,:v kept `s on unsorted data).
// A copy to a bigger block is not coloured (an0): a coloured payload's header gives half its block, so a vector
// grown an item at a time would be copied again within each class.
NI Z A an0(U n,C t)_(U i=58-CLZ(HD|HD-1+(((W)n<<Tw[t])+7>>3));A x=mb(i);xb=i;xr=REFB;xT=t;xn=n;_at(x)=0;x)//an(), uncoloured
A aa(U n,A x/*1*/)_(P(MINE(x)&&((W)n<<xw)+7>>3<=cap(x),_at(x)=0;AN(n,x))A y=an0(n,xt);MC(yV,xV,((W)xn<<Tw[xt])+7>>3);I(ytR,I(MINE(x),AZ(x))E(mRn(xn,xA)))x(y))
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
/* Does the 64-bit n fit in an I?  Answered by ROUND-TRIPPING it, not by
 * n-(I)n: for n = LLONG_MAX (k's 0W) the truncation is -1 and the subtraction
 * LLONG_MAX-(-1) overflows, which is undefined behaviour -- UBSan flags it on
 * any qSQL path that produces 0W.  The round-trip is exactly equivalent for
 * every in-range value and well-defined for the rest. */
A az(L n)_(n!=(L)(I)n?al(n):ai(n))
A al(L v)_(aV(tl,1,&v))
A af(F v)_(aV(tf,1,&v))
A aE(L i,L j)_(Q(i<=j)P(i==j,emp(tG))A x=an(tE,2);*xL=i;xL[1]=j;x)
// amber 2.3: mut() hands back a vector the caller is about to write into; when
// that is the object itself (sole owner), its attribute must go -- `s[0]:9000`
// on a sorted vector used to keep `s#, and find then binary-searched data that
// was no longer sorted (a silent 0N). Every in-place write is preceded by mut()
// or clears the byte itself (2.c, 1.c, o.c).
// amber 2.7: an empty general list carries a type witness in slot 0 (aA0), which its free releases: the copy
// has to carry it too, with its reference (`s#() on a shared () freed garbage there: 'UNMAP)
A1(mut,XP(x)P(MINE(x),_at(x)=0;x)U n=xn;B w=!n&&xt==tA;x=x(aV(xt,n|w,xV));AN(n,x);XR(mRn(n|w,xA);x)x)
C tZ(L v)_(G(tL,tL,tL,tL,tI,tI,tH,tG)[CLZ(v^v>>63|1)-1>>3])
A kv(A*p)_(A x=*p;Q(xn==2);P(!MINE(x),A k=_R(xx),v=_R(xy);I(ray_rc_sync,mr(x))E(--xr);*p=k;v)*p=xx;AZ(x);x(xy))   //shared: the halves taken, then x let go: in peach by mr, atomic (a plain --xr raced in workers reading one table), else --xr as before
L gl_(A x)_(XP(xv)*xL)
L gl(A x)_(L v=gl_(x);x(0);v)
F gf(A x)_(F v=*xF;x(0);v)
A AT(W t,A x)_(Q(t<tn);P(TP(t),Lt(t)|-1ull<<56&x)xT=t;x)
A AW(C w,A x)_(Q(w<6u);xE=w;x)
A AK(C k,A x)_(Q(k<9u);xk=k;x)
A AO(UC o,A x)_(Xs(x&~(0xffll<<32)|(W)o<<32)_O(x)=o;x)
A AN(U n,A x)_(xn=n;x)
A1(AZ,xT=tG;x)

// ---- symbol table -----------------------------------------------------------
// Symbols longer than 4 bytes live in one append-only byte region, and a
// symbol's id is its NEGATED offset there (high bit set; su() undoes it). Ids
// must never move, so the region cannot be realloc'd.
//
// amber 2.3: the region used to be a fixed 64 KB static array, and interning
// was a linear strcmp scan over all of it. Loading a few thousand long names
// (a day of vessel names, a CSV symbol column, IPC keys) died with 'SYMS, and
// every intern cost O(bytes interned so far) -- O(m^2) for m symbols, paid on
// every `$ over a string vector and every identifier the parser meets.
// Now: a large address-space RESERVATION (MAP_NORESERVE, so only the pages
// actually written are ever committed) plus an open-addressed hash index of the
// ids. Ids are byte-for-byte what the old table assigned for the same intern
// order, so nothing that stores or compares ids changes.
#if defined(wasm)
Z C s0a[1<<20];Z C*s0=s0a,*s1=s0a+1,*sE=s0a+SZ s0a;
#define SYMRESERVE() 1
#else
Z C*s0,*s1,*sE;
Z B symreserve(){P(s0,1)
 // 1 GB of address space, halving on failure (a strict-overcommit host may
 // refuse a large reservation); never below the old 64 KB.
 for(W z=(W)1<<30;z>=(W)1<<16;z>>=1){
  V*p=mmap(0,z,PROT_READ|PROT_WRITE,MAP_NORESERVE|MAP_PRIVATE|MAP_ANON,-1,0);
  I(p!=MAP_FAILED,s0=(C*)p;s1=s0+1;sE=s0+z;return 1;)}
 return 0;}
#define SYMRESERVE() symreserve()
#endif
Z U*sh,shc,shn;//hash index: slot -> id (0 = empty), shc slots, shn used
Z U shh(S s)_(U h=2166136261u;W(*s,h=(h^(UC)*s++)*16777619u)h^h>>15)//FNV-1a, folded
Z B shgrow()_(U c=shc?shc*2:1024;U*t=calloc(c,SZ(U));P(!t,0)
 F(shc,U v=sh[i];I(v,U j=shh(s0-(I)v)&(c-1);W(t[j],j=(j+1)&(c-1))t[j]=v))
 free(sh);sh=t;shc=c;1)
// su(): for a packed id (<=4 bytes) the bytes ARE the id, so they are copied
// into a scratch word and a pointer to that returned. That scratch used to be
// ONE static word, so two su() results in the same expression aliased each other
// (`"%s.%s",su(ns),su(sy)` printed one name twice) and peach workers raced on
// it. Eight rotating thread-local words fix both.
S su(U u)_(P(u&1u<<31,s0-(I)u)Z AM_TLS W r[8];Z AM_TLS U ri;W*p=r+(ri++&7);*p=u;(V*)p)
// Symbol intern. Short symbols (<=4 bytes without the high bit) are packed into
// the id itself and never touch the shared table, so they need no lock. The
// table path mutates s0/s1/sh shared across peach workers -- serialized by the
// allocator lock, engaged only in a ray_rc_sync scope.
U us(S s){U n=SL(s);I(n<4||(n==4&&!(s[3]&128)),U v=0;MC(&v,s,n);return v;)
 ALK();I(!SYMRESERVE()||(!shc&&!shgrow()),AUL();die("SYMS");)
 U h=shh(s),j=h&(shc-1);
 W(sh[j],I(!strcmp(s0-(I)sh[j],s),U r=sh[j];AUL();return r;)j=(j+1)&(shc-1))
 n++;I((W)(sE-s1)<n,AUL();die("SYMS");)MC(s1,s,n);s1+=n;U r=(U)(s0-s1+n);
 sh[j]=r;I(++shn*2>=shc,I(!shgrow(),AUL();die("SYMS");))AUL();return r;}
A sym(S s)_(as(us(s)))

// The globals: 65536 slots, the most a compiled reference's two-byte index can name (issue #9; it
// was 4096, and the next one ended the process). Static, so a slot never moves while a peach worker
// reads it; untouched pages cost nothing. Past the last, gi() returns slot 0 and sets gfull, which
// the compiler and gp() turn into a trappable 'limit.
Z U gd,gn;Z W gk[65536];A gv[65536];B gfull;
Z W gkk(A x/*0*/)_(Xs((U)xv)Q(xtS)xn?(W)_v(jS(drp(-1,xR)))<<32|(U)_v(ii(x,xn-1)):0)
// gi() appends a new name when it has not been seen; a lambda compiled inside a
// peach worker (`value`, a projection built at run time) reaches it concurrently,
// so the lookup+append runs under the allocator lock (a no-op outside peach).
U gi(A x/*0*/)_(W k=gkk(x);I(!(k>>32)&&id0(*su(k)),k|=(W)gd<<32)ALK();U i=fL(gk,gn,k);P(i<gn,AUL();i)P(gn>=L(gv),AUL();gfull=1;0)gk[gn]=k;gv[gn]=0;gn++;AUL();i)
A gg(A x/*1*/)_(//get value of global
 P(xtS&&!xn,x(0);A x=emp(tS),y=emp(tA);F(gn,I(gv[i],L k=gk[i];PSH(x,k-(U)k?jS(aV(tS,2,A((I)(k>>32),k))):as(k));PSH(y,_R(gv[i]))))am(x,y))//special case for 0#`
 W k=gkk(x);x(0);U i=fL(gk,gn,k);i<gn&&gv[i]?_R(gv[i]):ev0())
A*gp(A x/*1*/)_(U i=gi(x);x(0);P(gfull,gfull=0;ez0();(A*)0)gv+i)//get pointer to global; 0 (and 'limit) when the table is full
A*gq(A x/*0*/)_(W k=gkk(x);I(!(k>>32)&&id0(*su(k)),k|=(W)gd<<32)U i=fL(gk,gn,k);i<gn?gv+i:0)//as gp, but 0 where x names none yet (none added)
A gns(U k)_(U n=0;F(gn,n+=gk[i]>>32==k)A y=an(n,tS);n=0;F(gn,I(gk[i]>>32==k,_I(y)[n++]=(I)gk[i]))y)//list namespace (built on the heap: the table no longer fits a stack array)
// amber 2.0.0: is `p[0..n)` the name of an already-defined rank-2 (dyadic) global
// function?  The parser (p.c) uses this to make ANY binary library verb infix --
// `` `a xkey t `` -> xkey[`a;t] -- uniformly, instead of a hard-coded name list.
// Only NAMED identifiers reach here (the caller is in pt()'s id0 branch) and only
// genuine rank-2 functions qualify, so monadic verbs and inline {lambda}s (which
// are not names) are never coerced infix -- the constraint that broke `avg {x*x} x`.
B am_infix_dyad(S p,U n){
    C b[64]; W k; U i; A v;
    if (!n || n >= sizeof b) return 0;
    MC(b, p, n); b[n] = 0;
    k = us(b);
    if (!(k >> 32)) k |= (W)gd << 32;
    i = fL(gk, gn, k);
    if (i >= gn || !gv[i]) return 0;
    v = gv[i];
    /* Only a HEAP function object carries a valid arity byte at v-10.  A tagged/
     * immediate value -- a scalar, or a built-in verb like `+` which is already
     * infix -- must NOT be dereferenced there; a non-zero _t0 flags those, so we
     * bail before _k(v) reads a wild address. */
    return !_t0(v) && TU(_T(v)) && _k(v) == 2;
}

// amber 2.2: is `p[0..n)` a global that is BOUND to something which is NOT a
// rank-2 function?
//
// src/p.c decides whether an identifier reads as an INFIX verb, and one half of
// that decision is a hard-coded keyword list (infixkw: in, within, like, the
// join family, ss, sv, vs, xasc, xdesc, ...). That list claims a name whatever
// the name currently holds, so
//     ss:5
//     1_ss
// parsed `ss` as the string-search verb and built a PROJECTION instead of
// dropping -- silently, with no error. Consulting this function first means a
// name the program has rebound to data stops being read as a verb.
//
// A name that is not bound AT ALL answers 0, and that is what keeps the
// bootstrap working: while amber.k is still being loaded, `in`, `except` and
// the rest are undefined, and they must stay infix or the library's own source
// does not parse.
B am_name_nonfn(S p, U n) {
    C b[64]; W k; U i; A v;
    if (!n || n >= sizeof b) return 0;
    MC(b, p, n); b[n] = 0;
    k = us(b);
    if (!(k >> 32)) k |= (W)gd << 32;
    i = fL(gk, gn, k);
    if (i >= gn || !gv[i]) return 0;        // unbound: leave the keyword list alone
    v = gv[i];
    return !(!_t0(v) && TU(_T(v)) && _k(v) == 2);
}

// ---- 1.9.5: workspace introspection -----------------------------------------
// gk/gn are file-local to m.c, so the two readers that describe the workspace
// live here.  Both are pure reads: no refcount is touched, nothing is copied
// out of the heap except names and shapes, and neither can fail (they truncate
// instead).  src/ln.c uses am_globals() for instant Tab completion, and
// am_schema_brief() renders the one-line digest of the session that `\v and
// any installed extension (src/ext.h) both want.
unsigned am_globals(char *dst, size_t cap) {
    size_t o = 0; unsigned cnt = 0;
    if (!dst || cap < 2) { if (dst && cap) dst[0] = 0; return 0; }
    for (U i = 0; i < gn; i++) {
        char nb[96]; int m; W k; U ns, sy;
        if (!gv[i]) continue;
        k = gk[i]; ns = (U)(k >> 32); sy = (U)k;
        m = ns ? snprintf(nb, sizeof nb, "%s.%s", su(ns), su(sy))
               : snprintf(nb, sizeof nb, "%s", su(sy));
        if (m <= 0 || (size_t)m >= sizeof nb) continue;
        if (o + (size_t)m + 2 >= cap) break;
        memcpy(dst + o, nb, (size_t)m + 1); o += (size_t)m + 1; cnt++;
    }
    dst[o] = 0;
    return cnt;
}

// Is x a {names;values} pair whose names are symbols?  (Amber tables and dicts
// share the tM/tm tag; "table" is a structural question -- see src/inspect.c.)
Z I am_isdict(A x, A *kk, A *vv) {
    UC t = _t(x); A k, v;
    if (t != tM && t != tm) return 0;
    k = _x(x); v = _y(x);
    if (!k || !v || _t(k) != tS || _t(v) != tA) return 0;
    if (_n(k) != _n(v)) return 0;
    *kk = k; *vv = v; return 1;
}

void am_schema_brief(char *dst, size_t cap) {
    size_t o = 0;
    if (!dst || cap < 2) { if (dst && cap) dst[0] = 0; return; }
    dst[0] = 0;
    for (U i = 0; i < gn; i++) {
        char nb[512]; int m = 0; W key; U ns, sy; A x, k, v;
        if (!(x = gv[i])) continue;
        if (TU(_t(x))) continue;                       // skip functions
        key = gk[i]; ns = (U)(key >> 32); sy = (U)key;
        if (ns) continue;                              // root namespace only
        if (am_isdict(x, &k, &v)) {
            U nc = _n(k), c; L rows = -1; I tbl = 1; char cb[400]; size_t co = 0;
            for (c = 0; c < nc; c++) {
                A col = _A(v)[c]; UC ct;
                if (!col || _t(col) > tS) { tbl = 0; break; }
                ct = _t(col);
                if (rows < 0) rows = (L)_n(col); else if (rows != (L)_n(col)) tbl = 0;
                if (co + 40 < sizeof cb) {
                    UC at = _tP(col) ? 0 : _at(col);
                    co += (size_t)snprintf(cb + co, sizeof cb - co, "%s%s:%c%s",
                                           c ? " " : "", su((U)_I(k)[c]), TS[ct],
                                           at ? (at==1?"`s":at==2?"`u":at==3?"`p":"`g") : "");
                }
            }
            if (!nc) tbl = 0;
            m = snprintf(nb, sizeof nb, "%s%s:%s(%s)%s", o ? " " : "", su(sy),
                         tbl ? "table" : "dict", co ? cb : "",
                         "");
            if (tbl && rows > 0 && (size_t)m + 24 < sizeof nb)
                m += snprintf(nb + m, sizeof nb - (size_t)m, "[%lld rows]", (long long)rows);
        } else {
            UC t = _t(x);
            if (_tP(x)) m = snprintf(nb, sizeof nb, "%s%s:%c", o ? " " : "", su(sy), TS[t]);
            else m = snprintf(nb, sizeof nb, "%s%s:%c[%lu]", o ? " " : "", su(sy),
                              TS[t], (unsigned long)_n(x));
        }
        if (m <= 0 || (size_t)m >= sizeof nb) continue;
        if (o + (size_t)m + 1 >= cap) break;
        memcpy(dst + o, nb, (size_t)m + 1); o += (size_t)m;
    }
    dst[o] = 0;
}

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
// amber 2.7: \l on a directory maps a database (hdb.k's loaddb), as q's \l db does
#if !defined(wasm)
Z B bdir(S s)_(struct stat st;!stat(s,&st)&&S_ISDIR(st.st_mode))
#else
Z B bdir(S s)_((V)s;0)
#endif
Z A bsl1(S s)_(P(bdir(s),K1("{loaddb x;}",aCz(s)))I f=open(s,0,0);A x=u1c(ai(f));close(f);N(x);P(!xn,x(au))C*p=xC,*e=p+xn-1;P(*e-10,x(err0("eoleof")))*e=0;I(*p=='#'&&p[1]=='!',p=strchrnul(p,10);p+=!!*p)
  // amber 2.0.0: run the source through the K qSQL rewriter (qrwf, qsql.k) so
  // bare `select .. from ..` works in a .k file exactly as it does at the REPL
  // prompt -- no sel"..." wrapper. Guarded so nothing changes until qsql.k is
  // loaded: qrwf undefined (bootstrap) or any rewrite error makes the trap
  // return the `ERR symbol, and we fall back to the verbatim source. On success
  // qrwf returns a char vector (type tC); the ternary distinguishes the two.
  // The qrwf lookup is deferred inside {qrwf x} so `.`'s handler also catches
  // the undefined-variable error during bootstrap (before qsql.k defines qrwf);
  // `diag is toggled off around the probe so that recovered error never prints.
  A rw=K1("{d:`diag 0;r:.[{qrwf x};,x;{`ERR}];`diag d;r}",aCz(p));A r=_t(rw)==tC?(rw=str0(rw),evs(_C(rw),1)):evs(p,1);mr(rw);x(r))
// a script that loads itself (or two that load each other) nested evs and bsl on the C stack with no limit and
// crashed it; the VM counts its own depth (run), but a \l line does not go through it. 'stack after 512, as q's
// 'stack after 500 (per thread, as run's count)
  A bsl(S s)_(Z AM_TLS_IE I d;P(d>=512,es0())d++;A r=bsl1(s);d--;r)
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
 // 2.7.3: q's \ts (time and space) is \t here. repl.k did this for the terminal only, so a script or the browser
 // engine took \ts for a shell command, and in the browser that crashed (there is no shell)
 I(c=='t'&&d=='s'&&(!b[2]||b[2]==32||b[2]==':'),{C*q=b+1;W((*q=q[1]),q++)}d=b[1])
 P(!d||d==10||d==32||d==':',G(&bsl,bst,bsd,bsbs,bsf,bsv,bsm,bs0)[si("ltd\\fvm",c)](b+1+(d==32)))
 // amber 1.9.5: an installed extension (src/ext.h) claims its own \\-commands
 // here, BEFORE the historical "anything else is a shell command" fallback --
 // so `\\ai why` reaches the agent instead of being handed to /bin/sh, and an
 // engine with no extension behaves exactly as it always did.
 I(am_ext_bs,A amrv=am_ext_bs(b);P(amrv,amrv))
 K1("0x0a\\`x(,,\"/bin/sh\"),,:",aCz(b)))

Z A evs1(S*p)_(S s=*p;P(*s=='\\',++*p;bs_(p))A x=pk((V*)p,10);N(x);x=N(cpl(aCm(s,*p),x,0));x(run(x,0,0)))
// arena: rewind the HFT scratchpad at the end of EVERY statement, on every
// exit -- including the library-mode early return that hands back the final
// statement's value. That early return (`P(!*s,x)`) used to skip the rewind
// entirely, so the REPL (r=1, which falls through to the reset) was clean
// while every libamber.so consumer -- python-amber, amber-arrow, amberd, the
// LSP -- leaked one statement's scratch per call, forever.
// arena_release(mark) rather than arena_reset() because evs() is re-entrant:
// `. "expr"` reaches it through val() in src/a.c, and a full reset there would
// stomp scratch the OUTER expression still has live. A mark frees exactly what
// this statement took and nothing older, and marks nest LIFO by construction.
A evs(S s,B r)_(W(*s,ArenaMark am_=arena_mark();A x=evs1(&s);P(!x,I(r,s=strchrnul(s,10);s+=!!*s;epr(0))arena_release(am_);0)I(r,x(out(x)))E(P(!*s,arena_release(am_);x)x(0))mc();arena_release(am_))au)
// amber 1.9.5: the bare REPL (./amber with no script) reads through the native
// line editor (src/ln.c) -- editing, history and Tab completion -- and falls
// back to the historical raw read(2) only when stdin is not a terminal.
B rep()_(I(am_ln_interactive(),N x=0;C*p=am_repl_getline("",&x);P(!p,0)evs(p,1);free(p);1)
 Z C*b;Z W m,k;C*q;//b holds m bytes; its first k are an incomplete line, kept until its newline is read
 I(!b,P(!(b=malloc(m=256)),die("OOM")))
 W(1,I(k==m,C*t=realloc(b,2*m);P(!t,die("OOM"))b=t;m*=2)//a line longer than the buffer: grow it
     L n=read(0,b+k,m-k);P(n<=0,0)q=memchr(b+k,10,n);k+=n;
     P(q,C*p=b;W(q,*q=0;evs(p,1);p=q+1;q=memchr(p,10,b+k-p))k=b+k-p;memmove(b,p,k);1))1)
V repl(){W(rep())}

A cns,cn[tn];Z A ce[tn];S*argv,*env;
V kinit(){Z B l;P(l)l=1;alloc_lock_init();pg=sysconf(_SC_PAGESIZE);A b[32],*c=b;
 F(tS-tA+1,*c++=ce[tA+i]=an(0,tA+i))*c++=ce[tm]=am(emp(tS),emp(tA));_x(ce[tA])=_R(ce[tC]);ce[tM]=ce[tA];F(tn-ti,Q(!ce[i+ti]);ce[i+ti]=ce[tA])//empties
 cn[tA]=ce[tC];*c++=cn[ti]=cn[tl]=al(NL);F(tL-tE+1,cn[tE+i]=cn[ti])*c++=cn[tF]=cn[tf]=af(NF);cn[tC]=cn[tc]=ac(32);cn[tS]=cn[ts]=as(0);F(tn-to,cn[to+i]=au)//nulls
 Q(c-b<=32);cns=aV(tA,c-b,b);arena_init(0);}//arena_init: reserve the 16MB HFT scratchpad
// amber 2.3: envp sits right after argv's NULL on Linux and macOS, not on Cygwin
// (its DLL builds argv on the heap), where env walked garbage and `pe crashed.
#if defined(__CYGWIN__)
extern char**environ;
#define AM_ENVP(a,n) ((S*)environ)
#else
#define AM_ENVP(a,n) ((S*)(a)+(n)+1)
#endif
V kargs(I n,S*a){argv=(S*)a;env=AM_ENVP(a,n);n=MAX(0,n-2);A x=n?aA(n):emp(tA);F(n,xa=aCz(a[2+i]))gk[gn]='x';gv[gn++]=x;}
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
// amber 1.9.5: a 6th element carries the banner string of whatever extension is
// compiled in (src/ext.h, am_ext_banner), or "" when the build is stock. Purely
// additive -- every existing caller indexes 0..4 and is unaffected.
A1(binfo,L tot=0,nr=0;F(nreg,I(reg[i].p,tot+=reg[i].n;nr++))A a[]={al(tot),al(nr),aCz(AMARCH),aCz(AMCC),aCz(AMBER_VERSION),aCz(am_ext_banner?am_ext_banner:""),al((L)am_ln_term_cols()),al((L)am_ln_term_rows())};x(aV(tA,8,a)))
#define RGS(a...) F(nreg,B f=reg[i].f;V*p=reg[i].p,*q=f?p:p+reg[i].n;a)
#define OBS(a...) RGS(A z=(A)(p+HD*!f+pg*f),y=(A)q;W(z<y,A x=z+((W)_cl(z)<<6);a;z+=HD<<_b(z)))
#define XYS(a...) OBS(I(xtR,F(xn|!xn,A y=xa;a)))
#define RTS(a...) {A x=cns;a;F(gn,I(x=gv[i],a))}
A bsm(S s)_(XYS(I(!ytP,yr--))RTS(I(!xtP,xr--))OBS(I(xr,os("!refc:");ox(x)))RTS(I(!xtP,xr++))XYS(I(!ytP,yr++))
 OBS(I(xT>=tn,os("!type:");ox(x)))OBS(I(xtA&&!xn&&!xx,os("!prot:");ox(x)))XYS(I(!yt,os("!dngl:");ox(x);ox(y)))au)
