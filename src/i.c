/* ---- portability preamble: MUST precede every system header in this TU ----
 * Same class of bug as src/ar.c's strdup, caught by the same "Compile every TU
 * under strict -std=c99" job. Under `-std=c99` __STRICT_ANSI__ is defined and
 * glibc hides everything that is not ISO C, so this file loses TWO functions:
 *   fdopendir(3)  POSIX.1-2008, needs _POSIX_C_SOURCE >= 200809L
 *   wait4(2)      BSD, needs _DEFAULT_SOURCE (__USE_MISC) or _GNU_SOURCE
 * Both then fall back to an implicit `int f()`, and fdopendir's result is
 * assigned to a `DIR *` -- a 32-bit truncation of a real pointer on LP64, not
 * merely a diagnostic.
 *
 * _DARWIN_C_SOURCE accompanies _POSIX_C_SOURCE so that requesting strict POSIX
 * does not hide the BSD extensions this file also uses on Apple clang. All four
 * macros are purely ADDITIVE -- they only ever unhide declarations -- and match
 * the preamble src/a.c, src/arena.c and src/trace.c already carry.
 *
 * These must sit above the FIRST #include of the translation unit: any system
 * header may pull in <features.h> and latch the mode for the whole compilation. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include<dlfcn.h> // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include<sys/socket.h>
#include<sys/wait.h>
#include<netinet/in.h>
#include<netinet/tcp.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/ioctl.h>   // TIOCGWINSZ / struct winsize: cap plots to the terminal size
#include<sys/time.h>
#undef __USE_EXTERN_INLINES
#include<sys/stat.h>
#include<sys/mman.h>
#include<dirent.h>
#include"a.h"
#include<stdio.h>   // snprintf: axis tick labels (see fmtf below)
#include"arena.h"
Z U addr(S*p)_(S s=*p;P(!*s,0x0100007f)UC v[4];F(4,I(i,P(*s-'.',ed0())s++)v[i]=pu(&s);P(v[i]>255,ed0()))*p=s;*(U*)v)
Z I skt(U h,UH p)_(I f=socket(AF_INET,SOCK_STREAM,0);P(f<0,eo0())I v=setsockopt(f,IPPROTO_TCP,TCP_NODELAY,(I[]){1},4);P(v<0,eo0())
ST sockaddr_in a;a.sin_family=AF_INET;a.sin_addr.s_addr=h;a.sin_port=(UH)(p<<8|p>>8);P(connect(f,(ST sockaddr*)&a,SZ a)<0,eo0())f)
Z I osf(S s,L fl)_(P(!strchr(s,':'),I f=open(s,fl,0666);P(f<3/*fbsd*/,eo0())f)U h=addr(&s);P(*s-':',ed0())s++;W p=pu(&s);P(*s,ed0())skt(h,p))
Z I o(A x/*1*/,I fl)_(Xz(gl(x))Xs(xv?osf(su(xv),fl):1)XC(x=str0(x);I v;Mx(v=osf(xV,fl));v)et(x))
Z I fm(I f)_(ST stat s;fstat(f,&s)<0?0:s.st_mode)                                                                                                                 // get file mode
Z A frd(I f,N i,N n)_(P(i||n+1,en0())DIR*a=fdopendir(f);P(!a,ei0())A x=emp(tC);ST dirent*e;W((e=readdir(a)),S s=e->d_name;x=apc(cts(x,s,SL(s)),10))closedir(a);x) // read dir
Z A frS(I f,N n)_(C b[1024];A x=emp(tC);W(n,I k=read(f,b,MIN(SZ b,n));P(k<0,eo(x))n-=k;x=cts(x,b,k);P(k-SZ b,x))x)                                                // read stream (only length)
Z A frs(I f,N i,N n)_(I(i&&lseek(f,i,SEEK_CUR)<0,mr(N(frS(f,i))))frS(f,n))                                                                                        // read stream (offset too)
Z A frm(I f,N i,N n)_(L m=lseek(f,0,SEEK_END);P(m<0,eo0())n=MIN(n,MAX(0,m-i));n?mf(f,i,n):emp(tC))                                                                // read through mmap
Z A fr(A x/*1*/,N i,N n)_(Xz(frs(gl(x),i,n))I f=N(o(x,O_RDONLY));P(f<3,frs(f,i,n))I m=fm(f);x=(S_ISDIR(m)?frd:S_ISREG(m)?frm:frs)(f,i,n);close(f);x)              // read
void am_ln_sb_capture(const char*,unsigned long);// ln.c: tee stdout into the status-bar scroll-back ring
Z A fws(I f,S s,N n)_(I(f==1,am_ln_sb_capture(s,n))W(n>0,L k=write(f,s,n);P(k<0,eo0())P(!k,au)s+=k;n-=k)au)                                                         // write stream (fd 1 -> also scroll-back)
Z A fwm(I f,S s,N n)_(ftruncate(f,n);V*p=mmap(0,n,PROT_READ|PROT_WRITE,MAP_SHARED,f,0);MC(p,s,n);munmap(p,n);au)                                                  // write through mmap
Z X2(fw,Ril(I f=gl_(x);My(x=(f<3||!S_ISREG(fm(f))?fws:fwm)(f,yV,yn))x)R_(I f=N(o(xR,O_RDWR|O_CREAT|O_TRUNC));A z=v1c(ai(f),y);I(f>2,close(f))z))                   // write
ZN A dle()_(C*e=dlerror();I(e,os(e);os("\n"))eo0())
A1(opn,Xz(x)ai(N(o(x,O_RDWR|O_CREAT))))                                                                                     // <s
A cls(L n)_(close(n);au)                                                                                                    // >i
A1(u0c,spl(N(u1c(x))))                                                                                                      // 0:x
X1(u1c,RA(P(xn-2,el(x))P(!_tZ(xy),et(x))P(_n(xy)-2,el(x))A y=kv(&x);N i=gl(ii(y,0)),n=gl(ii(y,1));fr(x,i,n))R_(fr(x,0,-1))) // 1:x
A1(u2c,en(x))                                                                                                               // 2:x
Y2(v0c,RA(v0c(x,N(jc(10,y))))RC(v1c(x,apc(y,10)))R_(et(y)))                                                                 // x 0:y
Y2(v1c,RC(fw(x,y))R_(et(y)))                                                                                                // x 1:y
A2(v2c,P(!xts||!ytA,et(y))P(yn-2,el(y))P(!_ts(yx)||!_ti(yy),et(y))I k=_v(yy);y(0);P(!k||k>8u,ed0())                         // x 2:y
 V*l=dlopen(su(xv),RTLD_LAZY);P(!l,dle())V*f=dlsym(l,su(_v(yx)));P(!f,dle())ax(f,k))

Z A rda(I f)_(A x=aC(256-HD);L m=0,k;W((k=read(f,xV+m,xn-m))>0,m+=k;I(m+1000000>xn&&2*m>xn,A y=aC(2*xn+HD);MC(yV,xV,m);x=x(y)))close(f);AN(m,x))
Z I lC(A x)_(XA(F(xn,P(_t(xa)-tC,0))1)0)//list of strings?
A1(frk,P(!xtA||xn-2,et(x))A y=kv(&x);P(!lC(x)||!ytC,y(ed(x)))x=Ny(e1f(str0,x));S a[xn+1];F(xn,a[i]=_V(xa))a[xn]=0;I p[4];pipe(p);pipe(p+2);I pid=fork();
 P(!pid,dup2(*p,0);dup2(p[3],1);F(4,close(p[i]))exit(execve(*a,(C**)a,(C*CO*)env));0)close(*p);close(p[3]);N(v1c(ai(p[1]),x(y)));close(p[1]);A x=rda(p[2]);wait4(pid,0,0,0);x)
// amber: fork-based parallel-each.  x=(f;y): apply f to each item of y across
// AMBER_THREADS worker processes (default: online CPU count, see peachCPUs()
// below).  Each worker computes its slice,
// serialises the result (-8!, binary; was `k text before 1.9.3) and writes it
// down a pipe; the parent reads each (rda), deserialises (-9!) and concatenates.  Falls back to serial each for
// tiny inputs or AMBER_THREADS<2.  Correct because  (. `k v) ~ v  for all v.
// amber: default worker count = actual online CPU count (sysconf), not a
// hardcoded guess -- previously this always fell back to a fixed 4 regardless
// of the host, so on a 1-2 core box peach would oversubscribe (fork more
// workers than there are cores, making it *slower* than serial f'y through
// pure context-switch/fork overhead) and on a 16+ core box it would leave
// most cores idle. src/parallel.c's par_thread_count() already solves this
// correctly for the SIMD/vector engine (online_cpus() via
// sysconf(_SC_NPROCESSORS_ONLN)); this mirrors that same fix for peach so the
// two parallel primitives in Amber agree on what "auto" means. AMBER_THREADS
// still overrides explicitly, unchanged.
Z I peachCPUs(){
#if defined(_SC_NPROCESSORS_ONLN)
 L n=sysconf(_SC_NPROCESSORS_ONLN);if(n>0)return(I)n;
#endif
 return 4;//last-resort fallback if sysconf itself is unavailable
}
Z I peachNW(){S*e=env;I n=-1;if(e)while(*e){S p=*e++;if(!strncmp(p,"AMBER_THREADS=",14)){n=0;S q=p+14;while(*q>='0'&&*q<='9')n=n*10+(*q++-'0');if(n<1)n=peachCPUs();break;}}return n<0?peachCPUs():n;}
Z A eachR(A f,A y,U lo,U hi){U m=hi-lo;A u=aA0(m|!m);for(U i=0;i<m;i++){A v=_1(f,ii(y,lo+i));if(!v){mr(u);return 0;}u=psh(u,v);}return sqz(u);}
// amber 1.9.5: thread-pool parallel-each.  peach[f;y] applies f to each item of
// y across a PERSISTENT pool of POSIX worker threads (src/peachpool.c), which
// replaces the old fork()+pipe()+(-8!/-9!) model this function used to carry.
// Workers pull 1,024-element morsels and evaluate f straight into shared result
// slots -- no process spawn, no pipe write, no binary (de)serialisation on the
// hot path.  Correctness rests on scoped atomic refcounting (ray_rc_sync in
// a.h) and the ray_rc_sync-gated allocator/symbol-table lock in src/m.c, so the
// result is bit-identical to serial f'y across every datatype, list and table.
//
// A plain serial each is used when the input is trivially small (n<2), only one
// lane was asked for (AMBER_THREADS<2 / a single CPU), the call is already
// nested inside a peach dispatch (a worker's own f invoked peach again -- caught
// via ray_rc_sync), or the target has no pthreads (-Dwasm).  (-8!/-9! itself
// still lives in src/ser.c: it remains the `!`-verb serializer and is exercised
// directly by examples/peach_verify.k -- only peach's use of it is gone.)
A peachC(A x){P(_t(x)-tA||_n(x)-2,et(x))A fn=ii(x,0),dat=ii(x,1);U n=_N(dat);I nw=peachNW();if(nw>64)nw=64;
#if defined(wasm)
 {A r=eachR(fn,dat,0,n);mr(fn);mr(dat);return x(r);}                 // no threads in the wasm sandbox
#else
 if(nw<2||n<2||ray_rc_sync){A r=eachR(fn,dat,0,n);mr(fn);mr(dat);return x(r);}
 // Warm the lazily-initialised float format/parse tables (src/s.c I5/P5,
 // src/p.c powers) on THIS parent thread, so no worker is ever the first to
 // touch them and race on their one-time build.
 {C wb[64];L wd;F wv=1.5;MC(&wd,&wv,8);sf(wb,wd);S ws="1.5";pf(&ws);}
 A r=peach_pool(fn,dat,n,nw);
 mr(fn);mr(dat);
 P(!r,x(err0("worker error in peach")))
 return x(r);
#endif
}
// amber: window-join C kernel.  x=(qt;qcols;codes;w0;w1;gb;ge)  (marshalled by wj in amber.k)
//  qt    sorted long vector (ordering column; ascending within each group slice)
//  qcols list of numeric vectors (tF or tL) aligned to qt, one per aggregate
//  codes int vector, reducer per agg: 0=first 1=last 2=min 3=max 4=sum 5=avg 6=count
//  w0,w1 long window bounds per trade row (length nt)
//  gb,ge long group-slice [base,end) in q per trade row (length nt)
// returns list of nt-length result columns (tF for avg/float-source, tL otherwise).
//
// amber 1.9.5 kernel overhaul. The old shape was one fused loop nest per
// aggregate column, with EVERY type decision re-evaluated per row and, worse,
// per element: `isf?pf[k]:(F)pl[k]` inside the min/max/sum sweeps meant a
// data-independent branch on every single value, which is exactly what stops
// GCC/Clang from ever emitting a vector body. And the window probe -- two
// binary searches per row -- was repeated once per aggregate column even though
// the bounds depend only on (qt,w0,w1,gb,ge) and are therefore identical for
// all of them. Now:
//
//  1. ONE bounds pass (wjbounds) computes [lo,hi) for every row a single time
//     and parks it in two arena-bump scratch vectors reused by every column, so
//     an N-aggregate window join does 1/N of the probes it used to.
//  2. That pass uses the same monotone two-pointer merge as aj: consecutive
//     rows of a time-ordered table share a group slice and have non-decreasing
//     window edges, so both cursors walk forward and the run costs O(run+slice)
//     rather than O(run*log slice). Non-monotone rows fall back to the
//     branch-free binary probe, so results are input-order independent.
//  3. Reduction is done by fully specialized restrict-qualified kernels chosen
//     ONCE per column (wjrFF/wjrLL/wjrLF/wjrCNT). Inside those there is no type
//     test, no reducer test, no object header -- just a contiguous sweep over a
//     `const F*restrict` or `const L*restrict`, annotated with `omp simd
//     reduction(...)` so AVX2/NEON auto-vectorization actually fires.
//
// Three latent defects are fixed on the way through, all of which UBSan/ASan
// can see: `W1[i]+1` was signed overflow at WL (now amub, which needs no +1);
// an inverted window (w1<w0) made `hi-lo` underflow to a ~2^64 count and read
// off the end of the column (now clamped); and the integer `sum` accumulated in
// signed L, which is undefined on overflow (now accumulated in W, identical
// bits, defined wraparound -- the same treatment src/2.c's add kernels get).
// A pragma is emitted with _Pragma, and _Pragma is only well-defined at a plain
// STATEMENT position. Every reduction loop below is therefore written out with
// an explicit `for`, never through a.h's F()/S()/C()/D() macros: a _Pragma that
// materialises inside another macro's argument list has implementation-defined
// placement, and GCC resolves it by hoisting the directive out to where the
// reduction variable is not yet in scope -- which is a hard compile error
// ("'r' undeclared", "expected iteration declaration or initialization before
// 'i'"), not a warning, and it fires on some GCC builds while others accept the
// same source. src/3.c spells its reduction loops out for exactly this reason.
// Do not fold these back into F(...).
#ifdef _OPENMP
 #define WJPRAGMA(x) _Pragma(#x)
 #define WJSIMD(...) WJPRAGMA(omp simd reduction(__VA_ARGS__))
#else
 #define WJSIMD(...)
#endif
// Forward cursor probes: walk at most AMGALLOP steps from the previous row's
// answer, then finish with the branch-free binary probe over what is left.
Z U wjfwd_lb(CO L*RES a,U cur,U hi,L key){U lim=hi-cur>AMGALLOP?cur+AMGALLOP:hi,j=cur;
 W(j<lim&&a[j]<key,j++)I(j==lim&&lim<hi,j=amlb(a,lim,hi,key))return j;}
Z U wjfwd_ub(CO L*RES a,U cur,U hi,L key){U lim=hi-cur>AMGALLOP?cur+AMGALLOP:hi,j=cur;
 W(j<lim&&a[j]<=key,j++)I(j==lim&&lim<hi,j=amub(a,lim,hi,key))return j;}
// Pass 1: per-row half-open window [LO[i],HI[i]) inside the row's group slice.
// A null/empty/out-of-range slice, or an inverted window, yields the empty
// range 0,0 -- every reducer below then produces that reducer's identity.
// Per-group cursor cache, identical in construction and rationale to ajc()'s in
// src/a.c -- see the long comment there. wj needs TWO cursors per group (the
// window has a lower and an upper edge), and both advance monotonically for as
// long as that group's window edges do. Without this the cursors reset on every
// row of a time-ordered multi-symbol tape and both edges fall back to a binary
// probe, which is the O(N log M) behaviour the merge exists to avoid.
#define WJC_BITS 12u
#define WJC_N    (1u<<WJC_BITS)
#define WJC_H(b) ((U)(((W)(b)*0x9E3779B97F4A7C15ull)>>(64u-WJC_BITS)))
Z V wjbounds(CO L*RES T,U nq,CO L*RES W0,CO L*RES W1,CO L*RES GB,CO L*RES GE,U nt,U*RES LO,U*RES HI,
              L*RES cbase,L*RES ck0,L*RES ck1,U*RES clo_,U*RES chi_){
 MS(cbase,0xff,(N)WJC_N*SZ(L));       // -1: no real slice base is negative
 F(nt,
   L b=GB[i],en=GE[i];
   I(b==NL||en==NL||en<=b||(U)en>nq,LO[i]=0;HI[i]=0;continue)
   L k0=W0[i],k1=W1[i];U h=(U)en,g=WJC_H(b),lo,hi;
   I(cbase[g]==b&&ck0[g]<=k0&&ck1[g]<=k1,
     lo=wjfwd_lb(T,clo_[g],h,k0);hi=wjfwd_ub(T,chi_[g],h,k1))
   E(lo=amlb(T,(U)b,h,k0);hi=amub(T,(U)b,h,k1))
   I(hi<lo,hi=lo)                      // inverted window -> empty, never a wrapped count
   LO[i]=lo;HI[i]=hi;
   cbase[g]=b;ck0[g]=k0;ck1[g]=k1;clo_[g]=lo;chi_[g]=hi;)}
// Pass 2, float column -> float result. c: 0=first 1=last 2=min 3=max 4=sum 5=avg.
Z V wjrFF(CO F*RES p,CO U*RES LO,CO U*RES HI,U nt,I c,F*RES o){
 switch(c){
 case 0: for(U i=0;i<nt;i++){U a=LO[i];o[i]=a<HI[i]?p[a]:NF;} break;
 case 1: for(U i=0;i<nt;i++){U b=HI[i];o[i]=LO[i]<b?p[b-1]:NF;} break;
 case 2: for(U i=0;i<nt;i++){U a=LO[i],b=HI[i];F r=WF;
           WJSIMD(min:r)
           for(U k=a;k<b;k++)r=p[k]<r?p[k]:r;
           o[i]=r;} break;
 case 3: for(U i=0;i<nt;i++){U a=LO[i],b=HI[i];F r=-WF;
           WJSIMD(max:r)
           for(U k=a;k<b;k++)r=p[k]>r?p[k]:r;
           o[i]=r;} break;
 case 4: for(U i=0;i<nt;i++){U a=LO[i],b=HI[i];F r=0;
           WJSIMD(+:r)
           for(U k=a;k<b;k++)r+=p[k];
           o[i]=r;} break;
 default: for(U i=0;i<nt;i++){U a=LO[i],b=HI[i];F r=0;   /* 5 = avg */
           WJSIMD(+:r)
           for(U k=a;k<b;k++)r+=p[k];
           o[i]=b>a?r/(F)(b-a):NF;} break;
 }}
// Pass 2, long column -> long result. c: 0=first 1=last 2=min 3=max 4=sum.
Z V wjrLL(CO L*RES p,CO U*RES LO,CO U*RES HI,U nt,I c,L*RES o){
 switch(c){
 case 0: for(U i=0;i<nt;i++){U a=LO[i];o[i]=a<HI[i]?p[a]:NL;} break;
 case 1: for(U i=0;i<nt;i++){U b=HI[i];o[i]=LO[i]<b?p[b-1]:NL;} break;
 case 2: for(U i=0;i<nt;i++){U a=LO[i],b=HI[i];L r=WL;
           WJSIMD(min:r)
           for(U k=a;k<b;k++)r=p[k]<r?p[k]:r;
           o[i]=r;} break;
 case 3: for(U i=0;i<nt;i++){U a=LO[i],b=HI[i];L r=-WL;
           WJSIMD(max:r)
           for(U k=a;k<b;k++)r=p[k]>r?p[k]:r;
           o[i]=r;} break;
 // W accumulator: defined two's-complement wraparound, bit-identical to the
 // signed sum the old code computed but without the undefined behaviour.
 default: for(U i=0;i<nt;i++){U a=LO[i],b=HI[i];W r=0;   /* 4 = sum */
           WJSIMD(+:r)
           for(U k=a;k<b;k++)r+=(W)p[k];
           o[i]=(L)r;} break;
 }}
// Pass 2, long column -> float result (avg only: the one reducer that widens).
Z V wjrLF(CO L*RES p,CO U*RES LO,CO U*RES HI,U nt,F*RES o){
 for(U i=0;i<nt;i++){U a=LO[i],b=HI[i];F r=0;
   WJSIMD(+:r)
   for(U k=a;k<b;k++)r+=(F)p[k];
   o[i]=b>a?r/(F)(b-a):NF;}}
// Pass 2, count: source-independent, it is just the window width.
Z V wjrCNT(CO U*RES LO,CO U*RES HI,U nt,L*RES o){F(nt,o[i]=(L)(HI[i]-LO[i]))}
A wjc(A x){
 P(_t(x)-tA||_n(x)-7,et(x))
 A*e=(A*)_V(x);
 P(!_n(e[1]),x(emp(tA)))
 // normalise all integer inputs to 64-bit long (columns/times/bounds may be squeezed to G/H/I widths)
 A QT=N(cL(_R(e[0]))),CD=N(cL(_R(e[2]))),W0A=N(cL(_R(e[3]))),W1A=N(cL(_R(e[4]))),GBA=N(cL(_R(e[5]))),GEA=N(cL(_R(e[6])));
 // Raw contiguous primitive column pointers, extracted ONCE before any loop.
 CO L*RES T=_V(QT),*RES W0=_V(W0A),*RES W1=_V(W1A),*RES GB=_V(GBA),*RES GE=_V(GEA),*RES cod=_V(CD);
 U nt=_n(W0A),na=_n(e[1]),nq=_n(QT);
 A*QC=(A*)_V(e[1]);
 // The two bounds vectors are the kernel's ONLY workspace and are bump-allocated
 // from the thread-local arena exactly once, before the column loop -- no heap,
 // no per-row or per-column allocation. They are bracketed with
 // arena_mark()/arena_release() rather than left for evs() to rewind: the
 // end-of-cycle reset does not run at all on the library-mode evs() path
 // (amber_eval_str's early return for the final statement), and would not
 // bound `{wj[...]}'xs` even where it does. arena_release() frees exactly what
 // this call took and nothing older, so it still cannot stomp a caller's live
 // scratch.
 P((N)nt>((N)-1)/SZ(U),mr(QT);mr(CD);mr(W0A);mr(W1A);mr(GBA);mr(GEA);ez(x))
 ArenaMark wjmk=arena_mark();
 U*RES LO=(U*)arena_alloc((N)nt*SZ(U)),*RES HI=(U*)arena_alloc((N)nt*SZ(U));
 L*RES cb=(L*)arena_alloc((N)WJC_N*SZ(L));      // per-group cursor cache (~112 KB,
 L*RES c0=(L*)arena_alloc((N)WJC_N*SZ(L));      // L2-resident); see wjbounds above
 L*RES c1=(L*)arena_alloc((N)WJC_N*SZ(L));
 U*RES cl=(U*)arena_alloc((N)WJC_N*SZ(U));
 U*RES ch=(U*)arena_alloc((N)WJC_N*SZ(U));
 P(!LO||!HI||!cb||!c0||!c1||!cl||!ch,arena_release(wjmk);mr(QT);mr(CD);mr(W0A);mr(W1A);mr(GBA);mr(GEA);eo(x))
 wjbounds(T,nq,W0,W1,GB,GE,nt,LO,HI,cb,c0,c1,cl,ch);
 A res=aA(na);A*R=(A*)_V(res);
 for(U a=0;a<na;a++){
  A col=QC[a];I c=(I)cod[a];
  // Every type/reducer decision is resolved HERE, once per column, outside any
  // loop over rows or elements.
  B isf=_t(col)==tF,flo=(c==5)||(isf&&c!=6);
  A out=flo?aF(nt):aL(nt);
  A colL=(isf||c==6)?0:N(cL(_R(col)));   // count never reads the column at all
  if(c==6)                       wjrCNT(LO,HI,nt,(L*)_V(out));
  else if(isf)                   wjrFF((CO F*)_V(col),LO,HI,nt,c,(F*)_V(out));
  else if(c==5)                  wjrLF((CO L*)_V(colL),LO,HI,nt,(F*)_V(out));
  else                           wjrLL((CO L*)_V(colL),LO,HI,nt,c,(L*)_V(out));
  if(colL)mr(colL);
  R[a]=out;
 }
 mr(QT);mr(CD);mr(W0A);mr(W1A);mr(GBA);mr(GEA);
 arena_release(wjmk);
 return x(res);
}
// ============ Braille & Unicode terminal charting ============
// A chart here is three planes over a WxH grid of CHARACTER cells, each cell
// holding a 2x4 braille dot matrix (U+2800 + bitmask) -- so the drawing surface
// is 2W x 4H pixels, eight times the resolution of the text grid:
//   dot[]  the data pixels
//   own[]  which series owns a cell, so the emitter can colour it
//   grd[]  the gridline pixels, kept SEPARATE so a gridline can never be
//          mistaken for data: any cell that carries data drops its grid dots.
Z CO UC BRA[4][2]={{0x01,0x08},{0x02,0x10},{0x04,0x20},{0x40,0x80}};
Z V pxset(UC*RES c,I W,I H,I px,I py){if(px>=0&&py>=0&&px<2*W&&py<4*H)c[(py>>2)*W+(px>>1)]|=BRA[py&3][px&1];}
Z V pxs2(UC*RES c,UC*RES k,I W,I H,I px,I py,UC s){
 if(px>=0&&py>=0&&px<2*W&&py<4*H){I o=(py>>2)*W+(px>>1);c[o]|=BRA[py&3][px&1];k[o]=s;}}
Z V bres(UC*c,UC*k,I W,I H,I x0,I y0,I x1,I y1,UC s){I ax=x1-x0,ay=y1-y0,dx=ax<0?-ax:ax,sx=x0<x1?1:-1,dy=ay<0?ay:-ay,sy=y0<y1?1:-1,er=dx+dy;//Bresenham
 while(1){pxs2(c,k,W,H,x0,y0,s);if(x0==x1&&y0==y1)break;I e2=2*er;if(e2>=dy){er+=dy;x0+=sx;}if(e2<=dx){er+=dx;y0+=sy;}}}
Z C*ebr(C*p,UC b){U cp=0x2800+b;*p++=0xE2;*p++=0x80|(cp>>6&0x3F);*p++=0x80|(cp&0x3F);return p;}//emit braille char
Z C*elab(C*p,F v,I w){C b[40];L db;MC(&db,&v,8);C*e=sf(b,db);I ll=e-b;I(ll>w,ll=w)F(w-ll,*p++=' ')F(ll,*p++=b[i])return p;}//right-justified label
Z C*puts_(C*p,S s){while(*s)*p++=*s++;return p;}

// ---- axis arithmetic -------------------------------------------------------
// No <math.h> in this translation unit -- see the note in ast.c about the a.h
// collision -- and nothing below needs it. The decimal exponent comes from a
// loop rather than log10(); it runs once per axis, not once per point.
Z F pw10(I e){F r=1;if(e<0){F(-e,r/=10)}else{F(e,r*=10)}return r;}
Z F fab(F v){return v<0?-v:v;}
Z I dxp(F v){I e=0;if(!(v>0))return 0;while(v>=10&&e<300){v/=10;e++;}while(v<1&&e>-300){v*=10;e--;}return e;}
// (F)(L)v is only defined while v fits a long long; past 2^53 a double is
// integral anyway, so returning it unchanged is both safe and exact.
Z F ffl(F v){if(!(v>-1e15&&v<1e15))return v;F t=(F)(L)v;return t>v?t-1:t;}
Z F fcl(F v){if(!(v>-1e15&&v<1e15))return v;F t=(F)(L)v;return t<v?t+1:t;}
// The 1-2-5 ladder: the only tick steps a reader decodes without arithmetic.
Z F nicen(F r,B rnd){if(!(r>0))return 1;I e=dxp(r);F f=r/pw10(e),nf;
 if(rnd) nf=f<1.5?1:f<3?2:f<7?5:10; else nf=f<=1?1:f<=2?2:f<=5?5:10;
 return nf*pw10(e);}
// This is the whole of "zoomed enough, but not too much": take the data's own
// range and snap it OUTWARD to the nearest 1/2/5 boundary. Not the raw min/max
// (which pins the extremes to the frame, where they read as clipped), and not a
// fixed percentage pad (which lands the axis on unreadable numbers).
// One notch DOWN the 1-2-5 ladder: 10 -> 5 -> 2 -> 1 -> 0.5.
Z F stepdn(F st){if(!(st>0))return 0;I e=dxp(st);F f=st/pw10(e);
 if(f>4.5)return 2*pw10(e);if(f>1.5)return pw10(e);return 5*pw10(e-1);}
Z V axcalc(F lo,F hi,I want,F*olo,F*ohi,F*ost){
 if(hi<lo){F t=lo;lo=hi;hi=t;}
 if(!(hi>lo)){F c=lo,d=fab(c)>0?fab(c)*0.5:0.5;lo=c-d;hi=c+d;}   // flat series: give it a band
 if(want<1)want=1;
 F st=nicen((hi-lo)/want,1);
 if(!(st>0)||!(st<1e300)){*olo=lo;*ohi=hi;*ost=hi-lo;return;}
 F alo=ffl(lo/st)*st,ahi=fcl(hi/st)*st,dr=hi-lo;
 // Snapping outward must not cost a third of the view to empty margin: a series
 // spanning -14..14 on a step of 10 lands on a -20..20 axis and throws away 30%
 // of the height. Walk DOWN the ladder while that is true.
 for(I k=0;k<3&&dr>0&&(ahi-alo)>1.34*dr;k++){
  F ns=stepdn(st);if(!(ns>0))break;
  st=ns;alo=ffl(lo/st)*st;ahi=fcl(hi/st)*st;}
 if(!(ahi>alo)){*olo=lo;*ohi=hi;*ost=hi-lo;return;}
 // The label step is computed separately from the snapping step, so tightening
 // the view above cannot flood the gutter with a label on every row.
 F ls=nicen((ahi-alo)/want,1);if(!(ls>0))ls=ahi-alo;
 *olo=alo;*ohi=ahi;*ost=ls;}
Z I axdec(F st){I e=dxp(st);I d=e<0?-e:0;if(d>6)d=6;return d;}
// Tick labels never go through %f at extreme magnitudes: 1e300 would want 300
// digits of it. Anything outside a comfortable fixed-point band gets %g.
Z I fmtf(C*b,N bn,F v,I d){F a=fab(v);I r;
 if(v!=v)r=snprintf(b,bn,"nan");
 else if(a!=0&&(a>=1e7||a<1e-4))r=snprintf(b,bn,"%.4g",v);
 else r=snprintf(b,bn,"%.*f",d,a==0?0.0:v);
 if(r<0)r=0;if((N)r>=bn)r=(I)bn-1;return r;}
// Value -> pixel, guarded. The quotient is unbounded when explicit limits zoom
// far inside the data, and (I) of a double past INT_MAX is undefined, so the
// clamp happens in the float domain BEFORE the cast.
Z I vpx(F v,F lo,F sp,I n){F t=((F)(n-1))*(v-lo)/sp+0.5;if(!(t>-1e6))t=-1e6;if(t>1e6)t=1e6;return (I)t;}
Z I vpy(F v,F lo,F sp,I n){F t=((F)(n-1))*(1.0-(v-lo)/sp)+0.5;if(!(t>-1e6))t=-1e6;if(t>1e6)t=1e6;return (I)t;}

#define PLTMAXS 12          /* more series than this and no legend is readable */
#define PLTSPECN 10         /* elements in the canonical spec sys.k's plot builds */

// One series onto the dot/own planes.
// sty: 0 line, 1 scatter, 2 step, 3 area.
// When x is non-decreasing AND there are more points than pixel columns, the
// series is drawn as a per-column min/max ENVELOPE. That is the difference
// between a readable chart and the solid black smear a million points make when
// each one is joined to the next: the envelope keeps every spike (it is exactly
// the range the column covers) while drawing one vertical segment per column.
// A non-monotonic x means a parametric curve that doubles back, where a column
// envelope would be wrong, so those fall back to segment-by-segment.
Z V drawser(UC*dot,UC*own,I W,I H,CO F*Y,CO F*X,U n,F xlo,F xsp,F ylo,F ysp,I sty,UC sid){
 I pw=2*W,ph=4*H;
 I base=vpy(0.0>ylo?(0.0<ylo+ysp?0.0:ylo+ysp):ylo,ylo,ysp,ph);   // area baseline: y=0 if in view
 B mono=1;if(X){for(U i=1;i<n;i++)if(X[i]<X[i-1]){mono=0;break;}}
 B env=mono&&n>(U)(2*pw);
 I ppx=0,ppy=0;B have=0;
 I cpx=-1,cmn=0,cmx=0;                                           // current envelope column
 for(U i=0;i<n;i++){
  F cy=Y[i],cx=X?X[i]:(F)i;
  if(cy!=cy||cx!=cx){have=0;continue;}                           // a null breaks the line
  I px=vpx(cx,xlo,xsp,pw),py=vpy(cy,ylo,ysp,ph);
  if(env){
   if(px!=cpx){
    if(cpx>=0){bres(dot,own,W,H,cpx,cmn,cpx,cmx,sid);            // the column's whole range
               if(have)bres(dot,own,W,H,ppx,ppy,cpx,cmn>ppy?cmn:cmx,sid);
               ppx=cpx;ppy=cmn>cmx?cmn:cmx;have=1;}
    cpx=px;cmn=cmx=py;}
   else{if(py<cmn)cmn=py;if(py>cmx)cmx=py;}
   continue;}
  if(sty==1){pxs2(dot,own,W,H,px,py,sid);pxs2(dot,own,W,H,px+1,py,sid);pxs2(dot,own,W,H,px,py+1,sid);pxs2(dot,own,W,H,px+1,py+1,sid);}
  else if(sty==3){bres(dot,own,W,H,px,py,px,base,sid);}
  else if(have&&sty==2){bres(dot,own,W,H,ppx,ppy,px,ppy,sid);bres(dot,own,W,H,px,ppy,px,py,sid);}
  else if(have){
   // Both endpoints off the same edge means the segment is entirely outside the
   // view; drawing it clamped would paint a false flat line along the frame.
   B skip=(py<0&&ppy<0)||(py>=ph&&ppy>=ph)||(px<0&&ppx<0)||(px>=pw&&ppx>=pw);
   if(!skip)bres(dot,own,W,H,ppx,ppy,px,py,sid);}
  else pxs2(dot,own,W,H,px,py,sid);
  ppx=px;ppy=py;have=1;}
 if(env&&cpx>=0)bres(dot,own,W,H,cpx,cmn,cpx,cmx,sid);}

// The legacy surface: `plt v` / `plt (v;W;H)` -- a bare braille canvas with a
// value gutter and no frame. Kept byte-for-byte because it is the primitive the
// examples build on, and test-ext.k pins its row count.
Z A plotBare(A x){
 A dat;I W=70,H=15,mW=120,mH=32;
 {ST winsize ws;if(ioctl(1,TIOCGWINSZ,&ws)==0&&ws.ws_col>20){mW=(I)ws.ws_col-11;if(ws.ws_row>8)mH=(I)ws.ws_row-4;}}
 if(mW>200)mW=200;if(mW<10)mW=10;if(mH>60)mH=60;if(mH<3)mH=3;
 if(W>mW)W=mW;if(H>mH)H=mH;
 if(_t(x)==tA){dat=N(ii(x,0));if(_n(x)>1)W=(I)gl(N(ii(x,1)));if(_n(x)>2)H=(I)gl(N(ii(x,2)));}else dat=_R(x);
 mr(x);A ser=N(cF(dat));U n=_n(ser);
 if(!n){mr(ser);return aCz("(empty)\n");}
 if(W<10)W=10;if(W>mW)W=mW;if(H<3)H=3;if(H>mH)H=mH;I PW=2*W,PH=4*H;
 CO F*d=(CO F*)_V(ser);F mn=d[0],mx=d[0];F(n,I(d[i]<mn,mn=d[i])I(d[i]>mx,mx=d[i]))F rng=mx-mn;I(rng<=0,rng=1)
 ArenaMark mk=arena_mark();
 UC*cells=(UC*)arena_alloc((N)W*H),*own=(UC*)arena_alloc((N)W*H);
 if(!cells||!own){arena_release(mk);mr(ser);return eo(ser);}
 MS(cells,0,(N)W*H);MS(own,0,(N)W*H);I ppx=0,ppy=0;
 F(n,I px=n>1?(I)((F)i*(PW-1)/(n-1)+.5):PW/2,py=(I)((PH-1)*(1.-(d[i]-mn)/rng)+.5);I(px>PW-1,px=PW-1)I(py>PH-1,py=PH-1)I(py<0,py=0)I(i,bres(cells,own,W,H,ppx,ppy,px,py,1))E(pxs2(cells,own,W,H,px,py,1))ppx=px;ppy=py)
 A out=aC((N)H*(16+W*3)+64);if(!out){arena_release(mk);mr(ser);return 0;}C*p=(C*)_V(out);
 F(H,p=elab(p,i==0?mx:i==H-1?mn:mx-rng*i/(H-1),8);*p++=' ';*p++=0xE2;*p++=0x94;*p++=0x82;/*│*/
  Fj(W,UC b=cells[i*W+j];I(b,p=ebr(p,b))E(*p++=' '))*p++='\n')
 arena_release(mk);mr(ser);return AN(p-(C*)_V(out),out);}

// The full chart. spec, built by sys.k's `plot`, is positional so that the C
// side stays a pure renderer and every default lives in readable k:
//   0 ys      list of numeric vectors, one per series
//   1 xs      list of numeric vectors (or () to plot against the index)
//   2 opt     (W;H;grid;axis;legend;colour)      colour: 0 off 1 on 2 auto
//   3 lim     (ylo;yhi;xlo;xhi)                  0n on any = autoscale it
//   4 title   char vector ("" for none)
//   5 xlab    char vector
//   6 ylab    char vector
//   7 names   list of char vectors, for the legend (or ())
//   8 cols    256-colour codes, one per series (or () for the default palette)
//   9 styles  0 line, 1 scatter, 2 step, 3 area, one per series (or ())
Z A plotSpec(A x){
 A*e=(A*)_V(x);
 A ysL=e[0];P(_t(ysL)-tA||!_n(ysL),et(x))
 U ns=_n(ysL);if(ns>PLTMAXS)ns=PLTMAXS;
 A xsL=e[1];B hasx=_t(xsL)==tA&&_n(xsL)>=ns;
 A optA=N(cL(_R(e[2]))),limA=N(cF(_R(e[3])));
 if(!optA||!limA||_n(optA)<6||_n(limA)<4){if(optA)mr(optA);if(limA)mr(limA);return et(x);}
 CO L*opt=(CO L*)_V(optA);CO F*lim=(CO F*)_V(limA);
 I W=(I)opt[0],H=(I)opt[1];B grid=opt[2]!=0,axis=opt[3]!=0,leg=opt[4]!=0;
 I cmode=(I)opt[5];B colr=cmode==1||(cmode==2&&isatty(1));
 if(W<8)W=8;if(W>400)W=400;if(H<2)H=2;if(H>120)H=120;
 I pw=2*W,ph=4*H;
 A title=e[4],xlab=e[5],ylab=e[6],names=e[7];
 A colA=N(cL(_R(e[8]))),styA=N(cL(_R(e[9])));
 CO L*cols=colA&&_n(colA)>=ns?(CO L*)_V(colA):0;
 CO L*stys=styA&&_n(styA)>=ns?(CO L*)_V(styA):0;
 Z CO L DFC[PLTMAXS]={39,208,78,203,141,179,45,211,116,222,99,150};

 // materialise every series as float64 once
 A sY[PLTMAXS],sX[PLTMAXS];U nn[PLTMAXS];U nsv=0;
 for(U s=0;s<ns;s++){
  A cy=N(cF(_R(((A*)_V(ysL))[s])));if(!cy)continue;
  A cx=0;if(hasx){A xrf=((A*)_V(xsL))[s];if(_t(xrf)!=tA||_n(xrf))cx=N(cF(_R(xrf)));}
  U m=_n(cy);if(cx&&_n(cx)<m)m=_n(cx);
  if(!m){mr(cy);if(cx)mr(cx);continue;}
  sY[nsv]=cy;sX[nsv]=cx;nn[nsv]=m;nsv++;}
 if(!nsv){mr(optA);mr(limA);if(colA)mr(colA);if(styA)mr(styA);mr(x);return aCz("(empty)\n");}

 // data extent, skipping nulls and infinities so one bad point cannot flatten
 // every other series into a single row
 F ymn=0,ymx=0,xmn=0,xmx=0;B first=1;
 for(U s=0;s<nsv;s++){CO F*Y=(CO F*)_V(sY[s]),*X=sX[s]?(CO F*)_V(sX[s]):0;
  for(U i=0;i<nn[s];i++){F v=Y[i],u=X?X[i]:(F)i;
   if(v!=v||!(v>-1e308&&v<1e308))continue;
   if(u!=u||!(u>-1e308&&u<1e308))continue;
   if(first){ymn=ymx=v;xmn=xmx=u;first=0;}
   else{if(v<ymn)ymn=v;if(v>ymx)ymx=v;if(u<xmn)xmn=u;if(u>xmx)xmx=u;}}}
 if(first){ymn=0;ymx=1;xmn=0;xmx=1;}

 I ynt=H/3;if(ynt<2)ynt=2;if(ynt>8)ynt=8;
 I xnt=W/14;if(xnt<2)xnt=2;if(xnt>8)xnt=8;
 F ylo,yhi,yst,xlo,xhi,xst;
 axcalc(ymn,ymx,ynt,&ylo,&yhi,&yst);
 axcalc(xmn,xmx,xnt,&xlo,&xhi,&xst);
 // Explicit limits are taken verbatim -- an axis the caller pinned must not be
 // silently widened to a round number -- but still need a tick step.
 if(lim[0]==lim[0]||lim[1]==lim[1]){
  if(lim[0]==lim[0])ylo=lim[0];if(lim[1]==lim[1])yhi=lim[1];
  if(!(yhi>ylo)){yhi=ylo+1;}yst=nicen((yhi-ylo)/ynt,1);}
 if(lim[2]==lim[2]||lim[3]==lim[3]){
  if(lim[2]==lim[2])xlo=lim[2];if(lim[3]==lim[3])xhi=lim[3];
  if(!(xhi>xlo)){xhi=xlo+1;}xst=nicen((xhi-xlo)/xnt,1);}
 F ysp=yhi-ylo,xsp=xhi-xlo;if(!(ysp>0))ysp=1;if(!(xsp>0))xsp=1;

 ArenaMark mk=arena_mark();
 UC*dot=(UC*)arena_alloc((N)W*H),*own=(UC*)arena_alloc((N)W*H),*grd=(UC*)arena_alloc((N)W*H);
 if(!dot||!own||!grd){arena_release(mk);mr(optA);mr(limA);if(colA)mr(colA);if(styA)mr(styA);
  for(U s=0;s<nsv;s++){mr(sY[s]);if(sX[s])mr(sX[s]);}return eo(x);}
 MS(dot,0,(N)W*H);MS(own,0,(N)W*H);MS(grd,0,(N)W*H);

 // gridlines first, into their own plane, dashed so they read as background
  // Horizontal gridlines are the ones that earn their clutter -- they let a
 // reader put a value on a point. Vertical ones are opt-in (grid:2).
 if(grid){
  for(F v=fcl(ylo/yst)*yst;v<=yhi+yst*1e-9;v+=yst){I py=vpy(v,ylo,ysp,ph);
   if(py<0||py>=ph)continue;for(I px=0;px<pw;px+=8)pxset(grd,W,H,px,py);}
  if(grid>1)for(F v=fcl(xlo/xst)*xst;v<=xhi+xst*1e-9;v+=xst){I px=vpx(v,xlo,xsp,pw);
   if(px<0||px>=pw)continue;for(I py=0;py<ph;py+=8)pxset(grd,W,H,px,py);}}

 for(U s=0;s<nsv;s++)
  drawser(dot,own,W,H,(CO F*)_V(sY[s]),sX[s]?(CO F*)_V(sX[s]):0,nn[s],xlo,xsp,ylo,ysp,
          stys?(I)stys[s]:0,(UC)(s+1));

 // ---- y tick labels, and the gutter width they imply ----------------------
 I ydc=axdec(yst);
 C ylb[128][24];I yrow[128];I nyl=0;
 if(axis){
  for(F v=fcl(ylo/yst)*yst;v<=yhi+yst*1e-9&&nyl<120;v+=yst){
   I py=vpy(v,ylo,ysp,ph);if(py<0||py>=ph)continue;I r=py>>2;
   B dup=0;F(nyl,I(yrow[i]==r,dup=1))if(dup)continue;
   fmtf(ylb[nyl],24,v,ydc);yrow[nyl]=r;nyl++;}}
 I gw=0;F(nyl,I l=(I)strlen(ylb[i]);I(l>gw,gw=l))
 if(axis&&gw<1)gw=1;

 // ---- x tick labels, packed into one row so they cannot collide -----------
 I xdc=axdec(xst);
 C xrow[512];I xrn=W+2;if(xrn>510)xrn=510;
 MS(xrow,' ',(N)xrn);xrow[xrn]=0;
 UC xtk[400];MS(xtk,0,(N)W);
 if(axis){I used=-1;
  for(F v=fcl(xlo/xst)*xst;v<=xhi+xst*1e-9;v+=xst){
   I px=vpx(v,xlo,xsp,pw);if(px<0||px>=pw)continue;I c=px>>1;
   if(c<W)xtk[c]=1;
   C b[24];I l=fmtf(b,24,v,xdc);I st=c-l/2;if(st<0)st=0;if(st+l>xrn)st=xrn-l;if(st<0)continue;
   if(st<=used)continue;                                  // would touch the previous label
   MC(xrow+st,b,(N)l);used=st+l;}}

 // ---- emit ----------------------------------------------------------------
 // Worst case per cell is a colour change (11 bytes) plus a 3-byte glyph; the
 // +80 per row covers the gutter, the frame and the trailing reset.
 N cap=(N)(H+8)*((N)W*20+(N)gw+96)+1024;
 A out=aC(cap);if(!out){arena_release(mk);mr(optA);mr(limA);if(colA)mr(colA);if(styA)mr(styA);
  for(U s=0;s<nsv;s++){mr(sY[s]);if(sX[s])mr(sX[s]);}return 0;}
 C*p=(C*)_V(out);
 I inner=W,total=(axis?gw+1+2:0)+inner;
 C cb[24];
 #define SETC(n) do{if(colr){snprintf(cb,24,"\033[38;5;%dm",(I)(n));p=puts_(p,cb);}}while(0)
 #define RSTC()  do{if(colr)p=puts_(p,"\033[0m");}while(0)

 if(_t(title)==tC&&_n(title)){I l=(I)_n(title);I pad=(total-l)/2;if(pad<0)pad=0;
  F(pad,*p++=' ')if(colr)p=puts_(p,"\033[1m");MC(p,(CO C*)_V(title),(N)l);p+=l;if(colr)p=puts_(p,"\033[0m");*p++='\n';}
 if(_t(ylab)==tC&&_n(ylab)){I l=(I)_n(ylab);SETC(245);MC(p,(CO C*)_V(ylab),(N)l);p+=l;RSTC();*p++='\n';}

 if(axis){SETC(240);F(gw+1,*p++=' ')p=puts_(p,"┌");F(W,p=puts_(p,"─"))p=puts_(p,"┐");RSTC();*p++='\n';}

 for(I r=0;r<H;r++){
  if(axis){
   I li=-1;F(nyl,I(yrow[i]==r,li=i))
   if(li>=0){I l=(I)strlen(ylb[li]);SETC(245);F(gw-l,*p++=' ')MC(p,ylb[li],(N)l);p+=l;RSTC();}
   else F(gw,*p++=' ')
   *p++=' ';SETC(240);p=puts_(p,li>=0?"┤":"│");RSTC();}
  I cur=-1;
  for(I j=0;j<W;j++){
   UC b=dot[r*W+j];I want;
   if(b)want=(I)(cols?cols[own[r*W+j]-1]:DFC[(own[r*W+j]-1)%PLTMAXS]);
   else if(grid&&grd[r*W+j]){b=grd[r*W+j];want=237;}
   else{if(cur!=-1){RSTC();cur=-1;}*p++=' ';continue;}
   if(colr&&want!=cur){snprintf(cb,24,"\033[38;5;%dm",want);p=puts_(p,cb);cur=want;}
   p=ebr(p,b);}
  if(cur!=-1)RSTC();
  if(axis){SETC(240);p=puts_(p,"│");RSTC();}
  *p++='\n';}

 if(axis){SETC(240);F(gw+1,*p++=' ')p=puts_(p,"└");
  F(W,p=puts_(p,xtk[i]?"┬":"─"))p=puts_(p,"┘");RSTC();*p++='\n';
  I last=xrn;while(last>0&&xrow[last-1]==' ')last--;
  if(last>0){SETC(245);F(gw+2,*p++=' ')MC(p,xrow,(N)last);p+=last;RSTC();*p++='\n';}}

 if(_t(xlab)==tC&&_n(xlab)){I l=(I)_n(xlab);I pad=(total-l)/2;if(pad<0)pad=0;
  F(pad,*p++=' ')SETC(245);MC(p,(CO C*)_V(xlab),(N)l);p+=l;RSTC();*p++='\n';}

 if(leg&&_t(names)==tA&&_n(names)){U nl=_n(names);if(nl>nsv)nl=nsv;
  F(gw+2,*p++=' ')
  for(U s=0;s<nl;s++){A nm=((A*)_V(names))[s];if(_t(nm)!=tC)continue;
   SETC(cols?(I)cols[s]:(I)DFC[s%PLTMAXS]);p=puts_(p,"──");RSTC();*p++=' ';
   I l=(I)_n(nm);if(l>24)l=24;MC(p,(CO C*)_V(nm),(N)l);p+=l;
   if(s+1<nl){*p++=' ';*p++=' ';}}
  *p++='\n';}
 #undef SETC
 #undef RSTC

 arena_release(mk);mr(optA);mr(limA);if(colA)mr(colA);if(styA)mr(styA);
 for(U s=0;s<nsv;s++){mr(sY[s]);if(sX[s])mr(sX[s]);}
 mr(x);
 return AN((U)(p-(C*)_V(out)),out);}

// plt: the canonical 10-element spec goes to the full renderer; a bare vector
// or the legacy (v;W;H) triple keeps the original bare-canvas behaviour.
A plotC(A x){
 if(_t(x)==tA&&_n(x)==PLTSPECN&&_t(((A*)_V(x))[0])==tA)return plotSpec(x);
 return plotBare(x);}
// candle: x = (open;high;low;close) 4 numeric vectors.  Box wicks + block bodies + ANSI colour.
A candleC(A x){
 P(_t(x)-tA||_n(x)-4,et(x))
 A o=N(cF(N(ii(x,0)))),h=N(cF(N(ii(x,1)))),l=N(cF(N(ii(x,2)))),c=N(cF(N(ii(x,3))));mr(x);
 U n=_n(o);I H=15;if(!n){mr(o);mr(h);mr(l);mr(c);return aCz("(empty)\n");}
 CO F*O=_V(o),*Hi=_V(h),*Lo=_V(l),*Cl=_V(c);
 /* cap to the terminal: show only the most recent candles that fit its width,
  * and shrink the height on a short screen, so a big series never overflows. */
 {I mW=120;ST winsize ws;if(ioctl(1,TIOCGWINSZ,&ws)==0&&ws.ws_col>20){mW=((I)ws.ws_col-10)/2;if(mW<1)mW=1;if(ws.ws_row>10&&H>(I)ws.ws_row-4)H=(I)ws.ws_row-4;}
  if(n>(U)mW){U off=n-(U)mW;O+=off;Hi+=off;Lo+=off;Cl+=off;n-=off;}}
 F mn=Lo[0],mx=Hi[0];F(n,I(Lo[i]<mn,mn=Lo[i])I(Hi[i]>mx,mx=Hi[i]))F rng=mx-mn;I(rng<=0,rng=1)
 A out=aC(H*(9+n*24)+64);C*p=(C*)_V(out);
 F(H,I r=i;p=elab(p,i==0?mx:i==H-1?mn:mx-rng*i/(H-1),8);*p++=0xE2;*p++=0x94;*p++=0x82;
  Fj(n,I rh=(I)((H-1)*(1.-(Hi[j]-mn)/rng)+.5),rl=(I)((H-1)*(1.-(Lo[j]-mn)/rng)+.5),bt=O[j]>Cl[j]?O[j]:Cl[j],bb=O[j]<Cl[j]?O[j]:Cl[j];
   I rbt=(I)((H-1)*(1.-(bt-mn)/rng)+.5),rbb=(I)((H-1)*(1.-(bb-mn)/rng)+.5);B up=Cl[j]>=O[j];
   S col=up?"\033[32m":"\033[31m";
   I(r>=rbt&&r<=rbb,MC(p,col,5);p+=5;*p++=0xE2;*p++=0x96;*p++=0x88;/*█*/MC(p,"\033[0m",4);p+=4)
   J(r>=rh&&r<=rl,MC(p,col,5);p+=5;*p++=0xE2;*p++=0x94;*p++=0x82;/*│*/MC(p,"\033[0m",4);p+=4)
   E(*p++=' ')*p++=' ')*p++='\n')
 mr(o);mr(h);mr(l);mr(c);return AN(p-(C*)_V(out),out);}
L now()_(ST timeval t;gettimeofday(&t,0);1000000ll*t.tv_sec+t.tv_usec)
