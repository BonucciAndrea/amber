#include<dlfcn.h> // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include<sys/socket.h>
#include<sys/wait.h>
#include<netinet/in.h>
#include<netinet/tcp.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/time.h>
#undef __USE_EXTERN_INLINES
#include<sys/stat.h>
#include<sys/mman.h>
#include<dirent.h>
#include"a.h"
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
Z A fws(I f,S s,N n)_(W(n>0,L k=write(f,s,n);P(k<0,eo0())P(!k,au)s+=k;n-=k)au)                                                                                    // write stream
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
 // no per-row or per-column allocation. evs() rewinds the arena at the end of
 // the eval cycle, so wjc deliberately does NOT arena_reset() and cannot stomp
 // scratch a caller still has live.
 P((N)nt>((N)-1)/SZ(U),mr(QT);mr(CD);mr(W0A);mr(W1A);mr(GBA);mr(GEA);ez(x))
 U*RES LO=(U*)arena_alloc((N)nt*SZ(U)),*RES HI=(U*)arena_alloc((N)nt*SZ(U));
 L*RES cb=(L*)arena_alloc((N)WJC_N*SZ(L));      // per-group cursor cache (~112 KB,
 L*RES c0=(L*)arena_alloc((N)WJC_N*SZ(L));      // L2-resident); see wjbounds above
 L*RES c1=(L*)arena_alloc((N)WJC_N*SZ(L));
 U*RES cl=(U*)arena_alloc((N)WJC_N*SZ(U));
 U*RES ch=(U*)arena_alloc((N)WJC_N*SZ(U));
 P(!LO||!HI||!cb||!c0||!c1||!cl||!ch,mr(QT);mr(CD);mr(W0A);mr(W1A);mr(GBA);mr(GEA);eo(x))
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
 return x(res);
}
// ============ Braille & Unicode terminal charting ============
// Braille dot bit for sub-cell (sx in 0..1, sy in 0..3).  U+2800 + bitmask -> 2x4 pixel cell.
Z CO UC BRA[4][2]={{0x01,0x08},{0x02,0x10},{0x04,0x20},{0x40,0x80}};
Z V pxset(UC*RES c,I W,I H,I px,I py){if(px>=0&&py>=0&&px<2*W&&py<4*H)c[(py>>2)*W+(px>>1)]|=BRA[py&3][px&1];}
Z V bres(UC*c,I W,I H,I x0,I y0,I x1,I y1){I ax=x1-x0,ay=y1-y0,dx=ax<0?-ax:ax,sx=x0<x1?1:-1,dy=ay<0?ay:-ay,sy=y0<y1?1:-1,er=dx+dy;//Bresenham
 while(1){pxset(c,W,H,x0,y0);if(x0==x1&&y0==y1)break;I e2=2*er;if(e2>=dy){er+=dy;x0+=sx;}if(e2<=dx){er+=dx;y0+=sy;}}}
Z C*ebr(C*p,UC b){U cp=0x2800+b;*p++=0xE2;*p++=0x80|(cp>>6&0x3F);*p++=0x80|(cp&0x3F);return p;}//emit braille char
Z C*elab(C*p,F v,I w){C b[40];L db;MC(&db,&v,8);C*e=sf(b,db);I ll=e-b;I(ll>w,ll=w)F(w-ll,*p++=' ')F(ll,*p++=b[i])return p;}//right-justified label
// plot: x = numeric vector, or (series;W;H).  Returns a multi-line UTF-8 braille line chart.
A plotC(A x){
 A dat;I W=70,H=15;
 if(_t(x)==tA){dat=N(ii(x,0));if(_n(x)>1)W=(I)gl(N(ii(x,1)));if(_n(x)>2)H=(I)gl(N(ii(x,2)));}else dat=_R(x);
 mr(x);A ser=N(cF(dat));U n=_n(ser);
 if(!n){mr(ser);return aCz("(empty)\n");}
 if(W<10)W=10;if(W>200)W=200;if(H<3)H=3;if(H>60)H=60;I PW=2*W,PH=4*H;
 CO F*d=(CO F*)_V(ser);F mn=d[0],mx=d[0];F(n,I(d[i]<mn,mn=d[i])I(d[i]>mx,mx=d[i]))F rng=mx-mn;I(rng<=0,rng=1)
 UC cells[12000];MS(cells,0,W*H);I ppx=0,ppy=0;
 F(n,I px=n>1?(I)((F)i*(PW-1)/(n-1)+.5):PW/2,py=(I)((PH-1)*(1.-(d[i]-mn)/rng)+.5);I(px>PW-1,px=PW-1)I(py>PH-1,py=PH-1)I(py<0,py=0)I(i,bres(cells,W,H,ppx,ppy,px,py))E(pxset(cells,W,H,px,py))ppx=px;ppy=py)
 A out=aC(H*(16+W*3)+64);C*p=(C*)_V(out);
 F(H,p=elab(p,i==0?mx:i==H-1?mn:mx-rng*i/(H-1),8);*p++=' ';*p++=0xE2;*p++=0x94;*p++=0x82;/*│*/
  Fj(W,UC b=cells[i*W+j];I(b,p=ebr(p,b))E(*p++=' '))*p++='\n')
 mr(ser);return AN(p-(C*)_V(out),out);}
// candle: x = (open;high;low;close) 4 numeric vectors.  Box wicks + block bodies + ANSI colour.
A candleC(A x){
 P(_t(x)-tA||_n(x)-4,et(x))
 A o=N(cF(N(ii(x,0)))),h=N(cF(N(ii(x,1)))),l=N(cF(N(ii(x,2)))),c=N(cF(N(ii(x,3))));mr(x);
 U n=_n(o);I H=15;if(!n){mr(o);mr(h);mr(l);mr(c);return aCz("(empty)\n");}
 CO F*O=_V(o),*Hi=_V(h),*Lo=_V(l),*Cl=_V(c);
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
