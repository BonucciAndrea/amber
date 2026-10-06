/* clock_gettime()/CLOCK_MONOTONIC (used by the `simd` self-test/benchmark
 * builtin below) need `_POSIX_C_SOURCE >= 199309L`, which must be defined
 * before the first system header (a.h's own <unistd.h>) is pulled in --
 * same reasoning as trace.c/arena.c. Pure feature-test addition, no
 * behaviour change. */
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
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include <stdlib.h>   // malloc/free for the parallel kernels (exp); getenv (qdiag)
#include"ext.h"// amber 1.9.5: out-of-tree verb registry (see src/ext.h)
#include"arena.h"
#include"diagnostic.h"
#include"simd.h"
#include"vm.h"
#include"parallel.h"
#include"csv.h"
#include"ast.h"
#include<time.h>
#include<stdio.h>
// Definition of the scoped-atomic-refcount flag declared in a.h. Thread-local,
// default false: every thread starts in the fast serial (non-atomic) refcount
// mode and only peachC (src/i.c) flips it true around a parallel dispatch.
AM_TLS_IE B ray_rc_sync=false;
// ---- HFT as-of join kernel ------------------------------------------------
// branch-free lower_bound over a sorted long slice a[lo,hi): first i with a[i]>=key.
// The ternaries lower to cmov under -O3, so there are no data-dependent branches.
// amber: THE lower_bound for every sorted-long probe in the engine (aj, wj).
// Previously duplicated as ajlb() here and wjlb() in i.c with identical
// semantics but different (branchy vs branch-free) codegen; now one exported
// definition, so both join kernels get the cmov version.
U amlb(CO L*RES a,U lo,U hi,L key){
 U n=hi-lo,pos=lo;
 while(n>0){U half=n>>1,mid=pos+half;int lt=a[mid]<key;pos=lt?mid+1:pos;n=lt?n-half-1:half;}
 return pos;}
// branch-free upper_bound: first i in [lo,hi) with a[i]>key.
// amber: aj used to spell this amlb(...,key+1), which is signed overflow --
// undefined behaviour -- when a trade timestamp is WL (and UBSan flags it).
// Comparing <= directly is the same cmov sequence with no key arithmetic at all,
// so the overflow simply cannot arise. Exported alongside amlb because wj's
// upper window edge (w1) needs exactly the same "+1" and had the same hazard.
U amub(CO L*RES a,U lo,U hi,L key){
 U n=hi-lo,pos=lo;
 while(n>0){U half=n>>1,mid=pos+half;int le=a[mid]<=key;pos=le?mid+1:pos;n=le?n-half-1:half;}
 return pos;}
// aj[syms;trade;quote] as-of match kernel.  x=(qt;tt;gb;ge)  (marshalled by aj in amber.k)
//  qt     sorted long quote-timestamp vector (ascending within each group slice)
//  tt     long trade-timestamp vector (length nt)
//  gb,ge  per-trade group slice [base,end) into qt (length nt)
// returns long vector m (length nt): global index of the most-recent quote whose
//  timestamp is on-or-before the trade, or 0N (NL) when the slice is empty or no quote
//  precedes the trade.
//
// amber 1.9.5 kernel overhaul:
//  * Raw contiguous primitive column pointers (CO L*RES) are extracted ONCE up
//    front; the row loop never re-derives a base pointer or re-reads an object
//    header, so every access is a plain indexed load off a register.
//  * Two-pointer merge fast path. Rows of a real trade table arrive already
//    sorted by (group,time) -- amber.k xasc's the quote side and the trade side
//    is normally ascending within each symbol -- so consecutive rows usually
//    share a group slice AND have non-decreasing timestamps. In that case the
//    previous row's answer is a valid lower bound for this row's, and the cursor
//    just walks forward: the whole run costs O(run + slice) instead of
//    O(run * log slice), i.e. the O(N+M) merge the join wants. The moment
//    monotonicity actually breaks (a new slice, or a timestamp that goes
//    backwards) the row falls back to the branch-free binary probe, so the
//    result is bit-identical to the pure-amub version on ANY input -- including
//    the unsorted and null-slice cases test.k's ajNull/ajNoGrp/ajNs pin down.
//  * Scoped transient allocations. The old version bump-allocated an nt-long
//    arena scratch vector, filled it, then copied it element-by-element into the
//    result -- two full passes over nt longs and an arena_reset() that stomped
//    any scratch a caller still had live. Results are now written straight into
//    the freshly allocated result vector (which cannot alias any input), so aj
//    performs one pass over nt and never writes the result twice.
//    NOTE: the per-group cursor cache added below DOES take arena scratch --
//    the "touches the arena not at all" claim this comment used to make was
//    true only of the revision that predates it. The cache is bracketed with
//    arena_mark()/arena_release(), so the kernel is arena-neutral to its
//    caller in the sense that matters: its peak is one generation, and it
//    gives back everything it took before it returns.
// A time column as 64-bit keys for the time joins. Ints and temporals are themselves (cL); floats
// (f set when any time column is float) become keys in value order -- every NaN one key, first,
// then by value with -0.0 the same as 0.0 (issue #15) -- where cL used to truncate them (0.5 and
// 0.7 both 0). The zero is mapped by its bits: the build's -fno-signed-zeros lets a compare ignore it.
A tkey(A x,B f)_(I(_t(x)==tE,x=gZ(x))P(!f,cL(x))x=cF(x);   //a lazy range (!n as a time column) is expanded first: cL kept it a 2-item range
 P(!x,0)U n=_n(x);A y=aL(n);CO W*RES s=_V(x);L*RES d=_V(y);
 F(n,W b=s[i];b=b==1ull<<63?0:b;d[i]=b<<1>0xffe0000000000000ull?NL+1:(L)(b>>63?b^0x7fffffffffffffffull:b))x(y))
// A time column of dates, times or timestamps (a generic list) goes to the joins as its numbers (days, ms or
// ns: their order). tjk is what a time column is to them: tdt, ttm or tnp for a generic list of that kind
// alone; 1 for an empty generic list; -1 for a generic list holding one among items of other kinds (or ::),
// whose items would be read by their addresses; 0 for anything else. tjs: 'type (0) when one of two time
// columns is temporal and the other is not of its kind (an empty generic list goes with any), or either
// mixes, as in q. tjn: a time column as the kernels read it, a new reference.
I tjk(A x){P(_t(x)-tA,0)U n=_n(x);P(!n,1)CO A*a=_V(x);UC t=_t(*a);B m=0,o=0;F(n,UC u=_t(a[i]);m|=u==tdt||u==ttm||u==tnp;o|=u!=t)P(!m,0)return o?-1:t;}
B tjs(A a,A b){I p=tjk(a),q=tjk(b);P(p<0||q<0,0)P(p==q||p==1||q==1,1)return p<2&&q<2;}
A tjn(A x){I k=tjk(x);P(k==1,aL(0))P(k<2,_R(x))U n=_n(x);CO A*a=_V(x);A y=aL(n);L*RES v=_V(y);I(k==tnp,F(n,v[i]=*(L*)_V(a[i])))E(F(n,v[i]=(I)a[i]))return y;}
A ucb(A);
// Amber 2.5 (exp): one slice of trade rows for the parallel aj (see the parallel branch in ajc). Same row
// logic as ajc's serial loop, with this slice's own cursor cache.
#define PAJ_BITS 12u
#define PAJ_N (1u<<PAJ_BITS)
#define PAJ_H(b) ((U)(((W)(b)*0x9E3779B97F4A7C15ull)>>(64u-PAJ_BITS)))
#define PAJ_MIN (1u<<17)
TD struct{CO L*qt,*tt,*gb,*ge;L*m;U n,nq,np;L*cb[PAR_MAX_THREADS];L*ck[PAR_MAX_THREADS];U*cc[PAR_MAX_THREADS];}AJ;
Z V ajrows(V*c_,int t){AJ*c=c_;CO L*RES qt=c->qt,*RES tt=c->tt,*RES gb=c->gb,*RES ge=c->ge;L*RES m=c->m;U nq=c->nq;
 U s=(U)((W)c->n*t/c->np),e=(U)((W)c->n*(t+1)/c->np);L*RES cbase=c->cb[t],*RES ckey=c->ck[t];U*RES ccur=c->cc[t];
 MS(cbase,0xff,(N)PAJ_N*SZ(L));
 for(U i=s;i<e;i++){L b=gb[i],en=ge[i],key=tt[i];
  if(b==NL||en==NL||en<=b||(U)en>nq){m[i]=NL;continue;}
  U lo=(U)b,hi=(U)en,h=PAJ_H(b),j;
  if(cbase[h]==b&&ckey[h]<=key){U cur=ccur[h],lim=hi-cur>AMGALLOP?cur+AMGALLOP:hi;j=cur;while(j<lim&&qt[j]<=key)j++;if(j==lim&&lim<hi)j=amub(qt,lim,hi,key);}
  else j=amub(qt,lo,hi,key);
  m[i]=j>lo?(L)(j-1):NL;cbase[h]=b;ckey[h]=key;ccur[h]=j;}}
A ajc(A x){
 P(_t(x)-tA||_n(x)-4,et(x))
 A*e=(A*)_V(x);P(!tjs(e[0],e[1]),et(x))P(_N(e[2])-_N(e[1])||_N(e[3])-_N(e[1]),el(x))   //a slice per trade row
 A q0=tjn(e[0]),t0=tjn(e[1]);B f=_t(q0)==tF||_t(t0)==tF;
 B c_=_t(q0)==tC;A QT=N(tkey(ucb(q0),f),mr(t0);x(0)),TT=N(tkey(c_?ucb(t0):t0,f),mr(QT);x(0)),GB=N(tkey(_R(e[2]),0),mr(QT);mr(TT);x(0)),GE=N(tkey(_R(e[3]),0),mr(QT);mr(TT);mr(GB);x(0));   //float times as floats (tkey), char times as unsigned bytes (ucb)
 CO L*RES qt=_V(QT),*RES tt=_V(TT),*RES gb=_V(GB),*RES ge=_V(GE);
 U nt=_n(TT),nq=_n(QT);
 // On a 32-bit target (wasm32) size_t is 32 bits, so nt*sizeof(L) can wrap.
 P((N)nt>((N)-1)/SZ(L),mr(QT);mr(TT);mr(GB);mr(GE);ez(x))
 A out=aL(nt);L*RES m=_V(out);
 // ---- per-group merge cursors ----------------------------------------------
 // A single loop-carried cursor only merges when CONSECUTIVE trade rows fall in
 // the same group slice. That is the wrong assumption for the data this join
 // actually runs on: a trade tape is ordered by TIME across symbols, so row i
 // and row i+1 are almost always different symbols, the run breaks every single
 // row, and every lookup degrades to the cold binary probe -- O(N log M), which
 // is precisely the "still doing per-row lookups" behaviour a profile shows.
 //
 // The merge is per GROUP, so the cursor has to be per group too. Each group's
 // cursor only ever advances forward across the whole pass, so the total
 // forward walk is bounded by the sum of the slice widths -- i.e. M -- giving
 // O(N + M) overall no matter how the symbols interleave.
 //
 // Cursors live in a direct-mapped cache keyed on the slice base (which
 // uniquely identifies a group, since the slices are disjoint), rather than a
 // slot per group: there is no group-id column in the marshalled arguments, and
 // an exact table would need either a hash of arbitrary int64 bases or one slot
 // per quote row (16 MB on a 2M-row book). AJC_N slots cost 96 KB and stay
 // resident in L2. A miss or a collision is not a correctness problem -- it
 // just takes the binary probe for that row -- so the cache can be lossy and
 // the result is bit-identical to a pure-probe implementation on every input.
 #define AJC_BITS 12u
 #define AJC_N    (1u<<AJC_BITS)
 // Fibonacci hash: multiply by 2^64/phi and take the high bits. Slice bases are
 // strongly clustered (they are running offsets, often near-multiples of a
 // common group size), which is exactly the pattern a low-bit mask aliases
 // badly and a multiplicative hash spreads.
 #define AJC_H(b) ((U)(((W)(b)*0x9E3779B97F4A7C15ull)>>(64u-AJC_BITS)))
 // Scoped scratch. The cursor cache is dead the moment this kernel returns, so
 // it is bracketed with arena_mark()/arena_release() exactly as arena.h
 // prescribes for "a kernel that runs MANY times inside a single expression".
 // Relying on evs()'s end-of-cycle arena_reset() instead is not enough on two
 // counts: `{aj[c;t;q]}'xs` runs the kernel n times inside ONE statement and
 // would hold n generations of cache live at once, and the library-mode
 // evs() path (amber_eval_str, and therefore every libamber.so consumer)
 // returns the final statement's value through an early return that never
 // reaches the reset at all. Measured before this change: 41.5 KB of RSS
 // permanently per aj call, constant in row count; 200k joins reached 5 GB.
 // Two stores on the slab fast path, so it costs nothing.
 {int np=nt<PAJ_MIN?1:par_thread_count(nt);
  I(np>1,AJ c={.qt=qt,.tt=tt,.gb=gb,.ge=ge,.m=m,.n=nt,.nq=nq,.np=(U)np};B ok=1;
   F(np,c.cb[i]=malloc((N)PAJ_N*SZ(L));c.ck[i]=malloc((N)PAJ_N*SZ(L));c.cc[i]=malloc((N)PAJ_N*SZ(U));I(!c.cb[i]||!c.ck[i]||!c.cc[i],ok=0))
   I(ok,par_run(np,ajrows,&c))
   F(np,free(c.cb[i]);free(c.ck[i]);free(c.cc[i]))
   I(ok,mr(QT);mr(TT);mr(GB);mr(GE);return x(out);))}   //parallel: done; else the serial loop below
 ArenaMark ajmk=arena_mark();
 L*RES cbase=(L*)arena_alloc((N)AJC_N*SZ(L));   // slice base occupying the slot
 L*RES ckey =(L*)arena_alloc((N)AJC_N*SZ(L));   // that group's last probed key
 U*RES ccur =(U*)arena_alloc((N)AJC_N*SZ(U));   // that group's cursor position
 P(!cbase||!ckey||!ccur,arena_release(ajmk);mr(QT);mr(TT);mr(GB);mr(GE);mr(out);eo(x))
 MS(cbase,0xff,(N)AJC_N*SZ(L));                 // -1: no real slice base is negative
 F(nt,
   L b=gb[i],en=ge[i],key=tt[i];
   // Hoisted validity gate: a null or empty group slice yields a null match
   // without ever touching qt.
   I(b==NL||en==NL||en<=b||(U)en>nq,m[i]=NL;continue)
   U lo=(U)b,hi=(U)en,h=AJC_H(b),j;
   // Warm slot for THIS group, and this group's keys have not gone backwards:
   // resume the merge where this group left off, however many other symbols'
   // rows have been processed in between.
   I(cbase[h]==b&&ckey[h]<=key,
     U cur=ccur[h],lim=hi-cur>AMGALLOP?cur+AMGALLOP:hi;
     j=cur;W(j<lim&&qt[j]<=key,j++)
     I(j==lim&&lim<hi,j=amub(qt,lim,hi,key)))   // walked the cap out: finish by probe
   E(j=amub(qt,lo,hi,key))                       // cold: miss, collision, or key went back
   m[i]=j>lo?(L)(j-1):NL;                        // step back to on-or-before, else null
   cbase[h]=b;ckey[h]=key;ccur[h]=j;)
 #undef AJC_BITS
 #undef AJC_N
 #undef AJC_H
 mr(QT);mr(TT);mr(GB);mr(GE);
 arena_release(ajmk);
 return x(out);}

// ---- `ajs : is this table ALREADY in as-of-join order? ---------------------
// x = (gcols; tcol)   gcols = list of group columns (possibly empty), tcol = the
//                     ordering (time) column.  Returns 1b if aj/wj can consume
//                     the table as-is, 0b if it must be xasc'd first.
//
// Why this exists: aj[] unconditionally re-sorts its right-hand table
// (`y:xasc[c;y]`), which on a 2M-row book costs ~1.1 s EVEN WHEN THE BOOK IS
// ALREADY SORTED -- and a tick store hands you quotes already in (sym,time)
// order, so that is the normal case, not the exception. This predicate answers
// "is the sort necessary?" in one O(n) pass so the O(n log n) sort can be
// skipped entirely.
//
// It deliberately does NOT test "is this lexicographically ascending". That
// would have to agree with xasc's collation, and xasc grades SYMBOLS BY NAME
// while a symbol column stores interned ids -- id order and name order are
// unrelated, so an id comparison would report every xasc'd table as unsorted
// and the fast path would never fire. Instead it tests the two properties the
// join machinery actually depends on, both of which need only EQUALITY on the
// group columns and are therefore collation-independent:
//
//   (1) the ordering column is non-decreasing within each run of equal group
//       keys -- what the merge cursors in ajc()/wjbounds() assume; and
//   (2) each distinct group key occupies ONE contiguous run -- what wjbnd()
//       assumes when it takes (first index, count) as a group's whole slice.
//
// (2) is decided by hashing each run's key and looking for a repeat. A hash
// collision makes two distinct keys look like a repeat, which reports "not
// sorted" and falls back to the sort: wrong-but-safe in the only direction that
// matters. Two genuinely equal keys always hash equal, so a real violation can
// never be missed -- the predicate cannot return 1b for a table that would give
// a wrong join.
//
// Anything it does not understand (an exotic column type, an over-wide run
// table) also returns 0b, so a new type can never silently skip the sort.
#define AJS_MAXRUN (1u<<22)   /* cap on distinct groups before we just sort */
Z W ajs_h(A c,U i){    // 64-bit hash contribution of column c at row i
 W v;
 switch(_t(c)){
  case tG: case tC: v=(W)(UC)((CO G*)_V(c))[i];break;
  case tH: v=(W)(UH)((CO H*)_V(c))[i];break;
  case tI: case tS: v=(W)(U)((CO I*)_V(c))[i];break;
  case tL: v=(W)((CO L*)_V(c))[i];break;
  case tF: {F f=((CO F*)_V(c))[i];MC(&v,&f,8);I(f!=f,v=0x7ff8000000000000ull)E(I(!(v<<1),v=0));break;}   //one key per value, as = groups: both zeros, every NaN
  default: v=0;
 }
 return v*0x9E3779B97F4A7C15ull;}
Z W ajs_fk(F f){W b;MC(&b,&f,8);P(f!=f,0)b=b==1ull<<63?0:b;return(b>>63?~b:b|1ull<<63)+2;}   //the join's key order (tkey): every NaN one value, first; -0.0 is 0.0
A ajsC(A x){
 P(_t(x)-tA||(_n(x)-2&&_n(x)-3),et(x))
 A*e=(A*)_V(x);
 A gcs=e[0],tcol=e[1];
 // Optional third argument: the caller has established from the COLUMN
 // ATTRIBUTES (`s sorted / `p parted, see _at() in a.h) that the group keys are
 // already confined to contiguous runs. That is precisely what pass 3 below
 // spends its time proving, so an attributed column lets us skip it outright --
 // the whole point of carrying an attribute is not having to re-derive it.
 B trust=_n(x)==3&&_t(e[2])!=tA&&_v(e[2])!=0;
 U ng=_t(gcs)==tA?_n(gcs):0;
 A*gc=ng?(A*)_V(gcs):0;
 P(_tP(tcol)||_t(tcol)>=tM,x(al(0)))                // an atom or a dict: not a column, so not known sorted
 U n=_N(tcol);                                    // _N: a lazy range's length, not its two ends
 P(n<2,x(al(1)))                                  // 0 or 1 row is trivially ordered
 // ---- pass 1: run boundaries -------------------------------------------
 // chg[r] = "row r starts a new group". The type switch is hoisted OUT of the
 // row loop -- one dispatch per column, not one per element.
 UC*RES chg=(UC*)arena_alloc((N)n);
 P(!chg,x(al(0)))
 MS(chg,0,(N)n);
 #define AJS_NE(T) {CO T*RES p=_V(c);for(U r=1;r<n;r++)chg[r]|=(UC)(p[r]!=p[r-1]);}
 F(ng,A c=gc[i];
   P(_tP(c)||_t(c)>=tM||_n(c)-n,x(al(0)))                            // ragged column: don't guess
   switch(_t(c)){
    case tG: case tC: AJS_NE(G) break;
    case tH: AJS_NE(H) break;
    case tI: case tS: AJS_NE(I) break;             // tS stores packed 32-bit ids
    case tL: AJS_NE(L) break;
    case tF: {CO F*RES p=_V(c);for(U r=1;r<n;r++)chg[r]|=(UC)!(p[r]==p[r-1]||(p[r]!=p[r]&&p[r-1]!=p[r-1]));} break;   //by value, NaN as NaN: as = groups
    default: return x(al(0));                      // unknown type: sort, don't guess
   })
 #undef AJS_NE
 // ---- pass 2: ordering column non-decreasing inside each run ------------
 #define AJS_ORD(T) {CO T*RES p=_V(tcol);for(U r=1;r<n;r++)if(!chg[r]&&p[r]<p[r-1])return x(al(0));}
 switch(_t(tcol)){
  case tG: AJS_ORD(G) break;
  case tC: AJS_ORD(UC) break;   // chars: unsigned, as xasc sorts them
  case tH: AJS_ORD(H) break;
  case tI: AJS_ORD(I) break;
  case tL: AJS_ORD(L) break;
  case tE: break;                                  // a range (!n, a+!n) rises
  case tF: {CO F*RES p=_V(tcol);for(U r=1;r<n;r++)if(!chg[r]&&ajs_fk(p[r])<ajs_fk(p[r-1]))return x(al(0));} break;   //the join's order: a NaN no longer hides an unsorted run
  case tA: {CO A*RES p=_V(tcol);UC k=_t(*p);   //dates, times or timestamps of one kind (tjk): by their numbers, as the joins read them
   I(k==tnp,L v=*(L*)_V(*p);for(U r=1;r<n;r++){A y=p[r];P(_t(y)-tnp,x(al(0)))L u=*(L*)_V(y);if(!chg[r]&&u<v)return x(al(0));v=u;})
   J(k==tdt||k==ttm,I v=(I)*p;for(U r=1;r<n;r++){A y=p[r];P(_t(y)-k,x(al(0)))I u=(I)y;if(!chg[r]&&u<v)return x(al(0));v=u;})
   E(return x(al(0));)} break;
  default: return x(al(0));                        // unknown ordering column: sort
 }
 #undef AJS_ORD
 P(!ng,x(al(1)))                                   // no grouping: pass 2 was the whole test
 P(trust,x(al(1)))                                 // attribute already guarantees contiguity
 // ---- pass 3: no group key may occupy two separate runs -----------------
 // Open-addressed hash set over the runs' FNV-folded keys. Deliberately NOT
 // qsort(): <stdlib.h> cannot be included after a.h, whose single-letter
 // function-like macros (F, I, W, S, C, D, P, ...) rewrite the declarations in
 // any libc header pulled in behind it -- the same collision a.h already
 // documents for <math.h>. A hash set is also O(r) rather than O(r log r).
 U nr=1;F(n,nr+=(i&&chg[i]))
 // Bail BEFORE building the set, not after. A table whose group count is a
 // large fraction of its row count is not join-shaped -- it is a tape with the
 // symbols interleaved, i.e. exactly the input that must be sorted anyway. On a
 // 2M-row interleaved book the set would be ~4M slots (32 MB) and 2M scattered
 // probes, which measured ~330 ms: a real regression on the path that gains
 // nothing. Counting runs is one cheap pass over a byte array, so deciding here
 // costs almost nothing and caps the predicate's downside.
 // The n>=1024 guard matters: on a 4-row, 2-group table nr>(n>>2) is 2>1, which
 // would reject a perfectly ordered little table. The ratio only means anything
 // once there are enough rows for "groups per row" to be a real signal.
 P(nr>AJS_MAXRUN||(n>=1024u&&nr>(n>>2)),x(al(0)))
 U cap=8;while(cap<(nr<<1))cap<<=1;
 W*RES tbl=(W*)arena_alloc((N)cap*SZ(W));
 P(!tbl,x(al(0)))
 MS(tbl,0,(N)cap*SZ(W));                           // 0 marks an empty slot
 for(U r=0;r<n;r++){
   if(r&&!chg[r])continue;                         // not a run start
   W h=1469598103934665603ull;                     // FNV-1a offset basis
   for(U k=0;k<ng;k++){h^=ajs_h(gc[k],r);h*=1099511628211ull;}
   if(!h)h=1;                                      // keep 0 reserved as "empty"
   U s=(U)(h&(cap-1));
   while(tbl[s]){
     if(tbl[s]==h) return x(al(0));                // this key already had a run
     s=(s+1)&(cap-1);
   }
   tbl[s]=h;
 }
 return x(al(1));}
// ---- `wjb : per-trade-row group slice [base,end) into a grouped quote table --
// x = (qg; tg)   qg = quote-side group columns, tg = trade-side group columns
//                (same count, matching types).
// returns (gb; ge), two long vectors of length nt, or () to tell the caller to
// fall back to the K implementation (wjbndK).
//
// This replaces the K expression
//     gdi:=rows[g#+q]; bas:*:'value gdi; len:#:'value gdi; ix:(!gdi)?rows[g#+t]
// which measured ~968 ms on a 1M x 2M join -- the second biggest cost in aj
// after the sort, and bigger than the as-of kernel itself by a factor of ~24.
// Almost none of that was the grouping: `rows[...]` is `+. (g#+q)`, which
// MATERIALISES ONE K LIST PER ROW -- 2M heap objects for the quote side and
// another 1M for the trade side -- purely so that `=` and `?` have something
// row-shaped to hash. The group dictionary then allocates an index vector per
// group on top.
//
// Here nothing row-shaped is ever built. The quote side is walked once to find
// its run boundaries (a table that reached this point via ajord/xasc is already
// contiguously grouped, which is exactly what a `p`-parted or `s`-sorted column
// asserts), each run is inserted into an open-addressed table keyed on the group
// value, and each trade row does one probe. O(nq + nt), zero per-row objects.
//
// Correctness note: a hash collision here would silently attach a trade to the
// WRONG symbol's quotes, so -- unlike ajsC's contiguity check, where a collision
// only costs a sort -- the probe never trusts the hash. Every candidate slot is
// confirmed by comparing the actual group-column VALUES between the trade row
// and the run's representative quote row. The hash only chooses where to look.
// If the quote side turns out not to be contiguously grouped (a group key
// reappears after its run ended) the kernel returns () and the caller uses the
// K path, which handles that case.
Z UC wjbcode(UC t){switch(t){
  case tG: case tC: return 0; case tH: return 1;
  case tI: return 2; case tL: return 3; case tS: return 4;
  default: return 255;}}                      // unsupported: caller falls back
// One group-column value as a signed 64-bit scalar. Widths are normalised so a
// tI column on one side and a tL column on the other still compare by VALUE;
// tS is read unsigned because a packed symbol id legitimately uses bit 31.
Z L wjbgv(CO V*p,UC c,U r){switch(c){
  case 0: return (L)((CO G*)p)[r]; case 1: return (L)((CO H*)p)[r];
  case 2: return (L)((CO I*)p)[r]; case 3: return ((CO L*)p)[r];
  default: return (L)(U)((CO I*)p)[r];}}
#define WJB_MAXCOL 16u
A wjbC(A x){
 P(_t(x)-tA||_n(x)-2,et(x))
 A*e=(A*)_V(x);
 A QG=e[0],TG=e[1];
 P(_t(QG)-tA||_t(TG)-tA,x(emp(tA)))
 U ng=_n(QG);
 P(!ng||_n(TG)-ng||ng>WJB_MAXCOL,x(emp(tA)))
 A*qc=(A*)_V(QG),*tc=(A*)_V(TG);
 CO V*qp[WJB_MAXCOL],*tp[WJB_MAXCOL];UC cd[WJB_MAXCOL];
 U nq=_n(qc[0]),nt=_n(tc[0]);
 // Hoisted, once per column: type code and raw base pointer. The row loops
 // below never touch an object header.
 F(ng,UC a=wjbcode(_t(qc[i])),b=wjbcode(_t(tc[i]));
   P(a==255||a-b||_n(qc[i])-nq||_n(tc[i])-nt,x(emp(tA)))
   cd[i]=a;qp[i]=_V(qc[i]);tp[i]=_V(tc[i]);)
 // amber 2.1: ONE key column with a small value range (a symbol column's
 // interned ids, an int sym code) -> direct table indexed by value, no hashing
 // and no per-row compare. Measured on the 1M-trade x 200k-quote as-of join:
 // 32 ms -> ~2 ms for the slice pass. Falls through to the hash path for wide
 // or multi-column keys; returns () like the hash path when a key reappears.
 I(ng==1&&nq,{UC c0=cd[0];L lo=wjbgv(qp[0],c0,0),hi=lo;
  for(U r=1;r<nq;r++){L v=wjbgv(qp[0],c0,r);if(v<lo)lo=v;if(v>hi)hi=v;}
  W rg=(W)hi-(W)lo+1;
  I(rg&&rg<=((W)1<<22),{
   ArenaMark mk_=arena_mark();
   I*RES sl=(I*)arena_alloc((N)rg*SZ(I));L*RES bs=(L*)arena_alloc((N)(nq+1)*SZ(L));L*RES en=(L*)arena_alloc((N)(nq+1)*SZ(L));
   I(sl&&bs&&en,{
    MS(sl,0xff,(N)rg*SZ(I));U nr_=0,st=0;B ok=1;
    for(U r=1;r<=nq;r++){
      if(r<nq&&wjbgv(qp[0],c0,r)==wjbgv(qp[0],c0,r-1))continue;
      W k=(W)wjbgv(qp[0],c0,st)-(W)lo;
      if(sl[k]>=0){ok=0;break;}                 // key reappeared: not parted
      sl[k]=(I)nr_;bs[nr_]=(L)st;en[nr_]=(L)r;nr_++;st=r;}
    P(!ok,arena_release(mk_);x(emp(tA)))
    A gb=aL(nt),ge=aL(nt);L*RES B_=_V(gb),*RES E_=_V(ge);
    switch(c0){
     case 0:{CO G*p=tp[0];for(U i=0;i<nt;i++){W k=(W)(L)p[i]-(W)lo;I s=k<rg?sl[k]:-1;B_[i]=s<0?NL:bs[s];E_[i]=s<0?NL:en[s];}break;}
     case 1:{CO H*p=tp[0];for(U i=0;i<nt;i++){W k=(W)(L)p[i]-(W)lo;I s=k<rg?sl[k]:-1;B_[i]=s<0?NL:bs[s];E_[i]=s<0?NL:en[s];}break;}
     case 2:{CO I*p=tp[0];for(U i=0;i<nt;i++){W k=(W)(L)p[i]-(W)lo;I s=k<rg?sl[k]:-1;B_[i]=s<0?NL:bs[s];E_[i]=s<0?NL:en[s];}break;}
     case 3:{CO L*p=tp[0];for(U i=0;i<nt;i++){W k=(W)p[i]-(W)lo;I s=k<rg?sl[k]:-1;B_[i]=s<0?NL:bs[s];E_[i]=s<0?NL:en[s];}break;}
     default:{CO I*p=tp[0];for(U i=0;i<nt;i++){W k=(W)(L)(U)p[i]-(W)lo;I s=k<rg?sl[k]:-1;B_[i]=s<0?NL:bs[s];E_[i]=s<0?NL:en[s];}break;}}
    arena_release(mk_);
    return x(aA2(gb,ge));})
   arena_release(mk_);})})
 #define WJB_QQ(a,b) ({int q_=1;for(U k=0;k<ng;k++)if(wjbgv(qp[k],cd[k],a)!=wjbgv(qp[k],cd[k],b)){q_=0;break;}q_;})
 #define WJB_TQ(a,b) ({int q_=1;for(U k=0;k<ng;k++)if(wjbgv(tp[k],cd[k],a)!=wjbgv(qp[k],cd[k],b)){q_=0;break;}q_;})
 #define WJB_H(P_,r) ({W h_=1469598103934665603ull;for(U k=0;k<ng;k++){h_^=(W)wjbgv(P_[k],cd[k],r)*0x9E3779B97F4A7C15ull;h_*=1099511628211ull;}h_?h_:1ull;})
 // pass 1: count runs on the quote side
 U nr=0;
 for(U r=0;r<nq;r++)if(!r||!WJB_QQ(r,r-1))nr++;
 U cap=8;while((W)cap<(W)nr*2)cap<<=1;
 W*RES HT=(W*)arena_alloc((N)cap*SZ(W));       // 0 = empty slot
 U*RES QR=(U*)arena_alloc((N)cap*SZ(U));       // representative quote row
 L*RES BS=(L*)arena_alloc((N)cap*SZ(L));
 L*RES EN=(L*)arena_alloc((N)cap*SZ(L));
 P(!HT||!QR||!BS||!EN,x(emp(tA)))
 MS(HT,0,(N)cap*SZ(W));
 // pass 2: one table entry per run
 {U st=0;
  for(U r=1;r<=nq;r++){
    if(r<nq&&WJB_QQ(r,r-1))continue;           // still inside the current run
    W h=WJB_H(qp,st);U s=(U)(h&(cap-1));
    while(HT[s]){
      if(HT[s]==h&&WJB_QQ(QR[s],st))return x(emp(tA)); // key reappeared: not parted
      s=(s+1)&(cap-1);}
    HT[s]=h;QR[s]=st;BS[s]=(L)st;EN[s]=(L)r;
    st=r;}}
 // pass 3: one probe per trade row. A trade whose group has no quotes at all
 // yields a null slice, which ajc()/wjbounds() already read as "no match" --
 // the same answer the K path's out-of-range index produced.
 A gb=aL(nt),ge=aL(nt);L*RES B=_V(gb),*RES E=_V(ge);
 for(U i=0;i<nt;i++){
   W h=WJB_H(tp,i);U s=(U)(h&(cap-1));L b=NL,en=NL;
   while(HT[s]){
     if(HT[s]==h&&WJB_TQ(i,QR[s])){b=BS[s];en=EN[s];break;}
     s=(s+1)&(cap-1);}
   B[i]=b;E[i]=en;}
 #undef WJB_QQ
 #undef WJB_TQ
 #undef WJB_H
 return x(aA2(gb,ge));}
// arena self-test builtin (`arn): exercise bump / reset / overflow -> 1 on success.
A1(arnT,arena_init(1<<16);B ok=1;
 C*p=(C*)arena_alloc(100),*q=(C*)arena_alloc(200);ok&=!!p&&!!q&&(q>=p+100);
 F(100,p[i]=(C)i)ok&=p[42]==42;ok&=arena_used()>=300;
 arena_reset();ok&=arena_used()==0;
 C*big=(C*)arena_alloc(1<<20);ok&=!!big;I(big,big[0]=7;big[(1<<20)-1]=9;ok&=big[0]==7&&big[(1<<20)-1]==9)
 arena_reset();ok&=arena_used()==0;
 // Genuinely exercise the OVERFLOW path: ask for strictly more than the slab
 // holds so arena_alloc() has to fall back to a tracked heap block. The old
 // version asked for 1 MB against a >=16 MB slab (arena_init(1<<16) only
 // rewinds an already-larger slab, it never shrinks it), so the overflow
 // branch this self-test advertises was never actually taken.
 {N big2=arena_capacity()+(1<<16);C*ov=(C*)arena_alloc(big2);ok&=!!ov;
  I(ov,ov[0]=3;ov[big2-1]=5;ok&=ov[0]==3&&ov[big2-1]==5;ok&=arena_used()>=big2)
  arena_reset();ok&=arena_used()==0;}
 ok&=arena_peak()>=(N)(1<<20);//peak survives the rewind
 x(al((L)ok)))
// diagnostic self-test builtin (`dgn): render the reference report, verify its structure.
A1(dgnT,
 // 1.9.4 self-test for the diagnostic renderer AND the error catalogue.
 // Renders with color=0 so the assertions below are about layout, not SGR.
 CO C*src="x:1 2 3\n  prices + sizes\n";
 U pp=(U)(strstr(src,"prices")-src),sp2=(U)(strstr(src,"sizes")-src);
 Span pr=span_at(src,pp,pp+6),se=span_at(src,sp2,sp2+5);C buf[2048];
 report_diagnostic_ex(buf,SZ buf,"E0103","Vector length mismatch","test.k",pr,
   "operands have different counts",&se,1,
   "Conforming operations require vectors of matching lengths.",
   "left operand has 3 elements, right has 2",0);
 B ok=1;
 ok&=!!strstr(buf,"error[E0103]: Vector length mismatch");
 ok&=!!strstr(buf,"--> test.k:2:3");
 ok&=!!strstr(buf,"prices + sizes");
 ok&=!!strstr(buf,"^^^^^^");                       // primary spans the token
 ok&=!!strstr(buf,"~~~~~");                        // secondary uses ~, not ^
 ok&=!!strstr(buf,"operands have different counts");// inline label
 ok&=!!strstr(buf,"= help: ");
 ok&=!!strstr(buf,"= note: ");
 ok&=!strchr(buf,27);                              // color=0 emits NO ANSI
 // every gutter row puts its bar in the same column: that alignment IS the
 // visual design, and it is the first thing to break when padding changes.
 {CO C*p=buf;I col=-1;W(p,CO C*q=strchr(p,'|');I(!q,break)CO C*ln=q;
   W(ln>buf&&ln[-1]!=10,ln--)I c=(I)(q-ln);I(col<0,col=c)E(ok&=c==col)
   p=strchr(q,10);I(p,p++))}
 // the whole category -> code/title/help matrix (Task 3)
 {Z CO C*want[][2]={{"value","E0101"},{"type","E0102"},{"length","E0103"},
   {"domain","E0104"},{"parse","E0105"},{"index","E0106"},{"rank","E0107"},
   {"limit","E0108"},{"io","E0109"},{"stack","E0110"},{"compile","E0111"},
   {"nyi","E0112"}};
  F(L(want),CO C*c=edinfo(want[i][0],0),*t=edinfo(want[i][0],1),*h=edinfo(want[i][0],3);
    ok&=c&&t&&h&&!strcmp(c,want[i][1])&&*t&&*h)}
 // a category with no catalogue row must report absence, not crash
 ok&=!edinfo("no-such-category",0);
 // and the legacy compact block must NOT be re-emitted once a rich report has
 // been rendered -- that duplication is exactly what 1.9.4 removed.
 ok&=amdiagshown==0||amdiagshown==1;
 x(al((L)ok)))
// SIMD self-test + benchmark builtin (`simd): verifies simd_{add,mul,sum}_{i64,f64}
// (src/simd.{h,c}) against a plain scalar C reference over a large vector, prints a
// one-line "backend / n / scalar-ms / simd-ms" report to stderr (like `arn`/`dgn`'s
// silent-unless-you-look convention), and returns 1 iff every value round-trips
// exactly (integers) or exactly (floats -- add/mul are elementwise, not reduced, so
// no reordering is involved and no epsilon is needed here). Never touches the `+`/`*`
// dyadic verb dispatch (v.c) or the bytecode VM (b.c) -- this exercises the kernels
// directly against Long/Float vectors built the same way ajc()/aL()/aF() do above.
A1(simdT,
 U n=400009;//large enough to be a meaningful bench and to exercise every remainder tail
 //NOTE: locals below intentionally avoid the bare accessor-macro namespace
 //(xl/yl/xf/yf/etc. are g.h macros meaning "element i of x's Long/Float data").
 A vXi=aL(n),vYi=aL(n),vOi=aL(n);L*dXi=_V(vXi),*dYi=_V(vYi),*dOi=_V(vOi);
 F(n,dXi[i]=(L)i*7-200000;dYi[i]=(L)(n-i)*3+1)
 B ok=1;struct timespec t0,t1,t2;clock_gettime(CLOCK_MONOTONIC,&t0);
 simd_add_i64((int64_t*)dXi,(int64_t*)dYi,(int64_t*)dOi,n);clock_gettime(CLOCK_MONOTONIC,&t1);
 F(n,L r=dXi[i]+dYi[i];ok&=dOi[i]==r)
 //NOTE: the arena_reset() below MUST come after the reference loop. It used
 //to sit immediately after the arena_alloc(), so every one of the n stores
 //into dRef[] landed in scratch the allocator had already rewound and was
 //free to hand out again -- a use-after-reset that only happened to be
 //harmless because nothing else allocated in between.
 L*dRef=(L*)arena_alloc((N)n*SZ(L));P(!dRef,mr(vXi);mr(vYi);mr(vOi);x(al(0)))//scratch just for timing symmetry
 F(n,dRef[i]=dXi[i]+dYi[i]);clock_gettime(CLOCK_MONOTONIC,&t2);arena_reset();
 simd_mul_i64((int64_t*)dXi,(int64_t*)dYi,(int64_t*)dOi,n);F(n,ok&=dOi[i]==dXi[i]*dYi[i])
 L want=0;F(n,want+=dXi[i])ok&=simd_sum_i64((int64_t*)dXi,n)==want;
 A vXf=aF(n),vYf=aF(n),vOf=aF(n);F*dXf=_V(vXf),*dYf=_V(vYf),*dOf=_V(vOf);
 F(n,dXf[i]=(F)i*0.5-3.0;dYf[i]=(F)(n-i)*0.25)
 simd_add_f64(dXf,dYf,dOf,n);F(n,ok&=dOf[i]==dXf[i]+dYf[i])
 simd_mul_f64(dXf,dYf,dOf,n);F(n,ok&=dOf[i]==dXf[i]*dYf[i])
 mr(vXi);mr(vYi);mr(vOi);mr(vXf);mr(vYf);mr(vOf);
 F ms=(F)((t1.tv_sec-t0.tv_sec)*1000000000ll+(t1.tv_nsec-t0.tv_nsec))/1e6,
   ss=(F)((t2.tv_sec-t1.tv_sec)*1000000000ll+(t2.tv_nsec-t1.tv_nsec))/1e6;
 fprintf(stderr,"simd: backend=%s n=%u simd_add=%.3fms scalar_add=%.3fms ok=%d\n",simd_backend(),n,ms,ss,ok);
 x(al((L)ok)))
// bytecode-disassembler self-test builtin (`vmd): compiles several representative
// expressions and re-checks that vm.c's mirrored opcode/operand-length table
// (kept in sync with b.c by hand, since b.c does not export it) still decodes
// every one of them byte-exact. See vm.c: vm_selftest().
A1(vmdT,x(al((L)vm_selftest())))
// CSV loader builtin (`csvr): x is a char vector (file path); returns a typed
// table via csv_read() (csv.{h,c}). x itself is a string, not the arena/file --
// csv_read() re-opens the path with a plain C FILE*, so x is only consumed here.
X1(csvrT,RC(C buf[4096];P(xn>=SZ buf,x(ez0()))U n=xn;MC(buf,xC,n);buf[n]=0;x(csv_read(buf)))R_(et(x)))//a longer path used to be truncated silently
// `csvx "path": the new reader against the 2.2.0 reference reader, bit for bit (csv.h).
X1(csvxT,RC(C buf[1024];U n=MIN(xn,SZ buf-1);MC(buf,xC,n);buf[n]=0;x(al((L)csv_check(buf))))R_(et(x)))
// CSV parser self-test builtin (`csv0): writes a small known CSV (mixed long/
// float/symbol columns, an embedded comma inside a quoted field, an escaped
// quote, and one empty cell) to a temp file, parses it with csv_read(), and
// asserts the resulting table's shape/types/values/null-handling. Cleans up
// the temp file whether the assertions pass or fail.
A1(csv0T,
 // A name of this process's own: two test runs at once shared one file (digest #80)
 C P_[64];
#if !defined(wasm)
 snprintf(P_,SZ P_,"/tmp/.amber_csv_selftest.%d.csv",(I)getpid());
#else
 snprintf(P_,SZ P_,"/tmp/.amber_csv_selftest.csv");
#endif
 CO C*body="sym,px,qty,note\nAAPL,187.5,100,\"a note, with a comma\"\nMSFT,410.2,50,plain\nGOOG,138.9,,\"a \"\"quoted\"\" word\"\n";
 FILE*fp=fopen(P_,"wb");B ok=!!fp;I(fp,fwrite(body,1,strlen(body),fp);fclose(fp))
 // Delegate the actual shape/value/null-handling assertions to the real,
 // already-proven K evaluator (#, [], ~, @, &) rather than hand-walking the
 // table's internal representation here -- csv_read()'s own header comment
 // documents that its output is verified against the same primitives.
 // NOTE: K has no operator precedence (flat right-to-left), so each `~`/`=`
 // comparison MUST be parenthesized -- an unparenthesized `a~b&c` groups as
 // `a~(b&c)`, not `(a~b)&c`.
 // NOTE: uses a local name `_ct` (not `t`) for the parsed table -- assigning
 // to the bare global `t` here would clobber test.k's own harness function
 // (also named `t`), breaking every t[...] assertion that runs after this
 // self-test in the same session. Hit and fixed via the full regression run.
 C chk[512];snprintf(chk,SZ chk,"_ct:`csvr \"%s\";"
   "((#_ct)=3)&(_ct[`sym]~`AAPL`MSFT`GOOG)&(_ct[`px]~187.5 410.2 138.9)&(_ct[`qty]~100 50 0N)&((@_ct[`note])=`S)",P_);
 // evs() returns 0 (not `au`) on a parse/compile/eval error -- check
 // truthiness of r itself, not identity against `au`, before touching it.
 A r=ok?evs(chk,0):0;ok=ok&&r&&tru(r);I(r,mr(r))
 remove(P_);ok=ok&&csv_selftest();x(al((L)ok)))
// AST visualizer self-test builtin (`astt): runs \ast (src/ast.{h,c}) over a
// set of representative expressions with stdout captured and checks each
// printed tree contains the expected labels -- guards against the historical
// "<v-atom>"/"<w-atom>"/"<o-atom>"/"<I-atom>"/"<S-atom>" placeholder bugs
// (unrecognized verb/adverb/lambda/vector/symbol-vector leaves) regressing.
// See ast_selftest() in ast.c for the full case list, and tests/test_ast.c
// for a standalone (non-builtin) harness covering the same ground.
A1(astT,x(al((L)ast_selftest())))
// multithreaded vector engine self-test + benchmark builtin (`par): verifies
// par_{add,mul,sum}_{i64,f64} (src/parallel.{h,c}) against a plain scalar C
// reference over a vector well above PAR_THRESHOLD, reports the thread count
// actually used and a serial-vs-parallel timing comparison to stderr (same
// silent-unless-you-look convention as `simd`), returns 1 iff every value
// matches. Never touches the bytecode VM or the `+`/`*` dyadic dispatch.
A1(parT,
 U n=600037;//comfortably above PAR_THRESHOLD (100000)
 A vXi=aL(n),vYi=aL(n),vOi=aL(n);L*dXi=_V(vXi),*dYi=_V(vYi),*dOi=_V(vOi);
 F(n,dXi[i]=(L)i*5-300000;dYi[i]=(L)(n-i)*2+3)
 B ok=1;struct timespec t0,t1,t2;
 clock_gettime(CLOCK_MONOTONIC,&t0);
 par_add_i64((int64_t*)dXi,(int64_t*)dYi,(int64_t*)dOi,n);
 clock_gettime(CLOCK_MONOTONIC,&t1);
 F(n,ok&=dOi[i]==dXi[i]+dYi[i])
 simd_add_i64((int64_t*)dXi,(int64_t*)dYi,(int64_t*)dOi,n);
 clock_gettime(CLOCK_MONOTONIC,&t2);
 par_mul_i64((int64_t*)dXi,(int64_t*)dYi,(int64_t*)dOi,n);F(n,ok&=dOi[i]==dXi[i]*dYi[i])
 L want=0;F(n,want+=dXi[i])ok&=par_sum_i64((int64_t*)dXi,n)==want;
 A vXf=aF(n),vYf=aF(n),vOf=aF(n);F*dXf=_V(vXf),*dYf=_V(vYf),*dOf=_V(vOf);
 F(n,dXf[i]=(F)i*0.25-1.0;dYf[i]=(F)(n-i)*0.1)
 par_add_f64(dXf,dYf,dOf,n);F(n,ok&=dOf[i]==dXf[i]+dYf[i])
 par_mul_f64(dXf,dYf,dOf,n);F(n,ok&=dOf[i]==dXf[i]*dYf[i])
 mr(vXi);mr(vYi);mr(vOi);mr(vXf);mr(vYf);mr(vOf);
 F pms=(F)((t1.tv_sec-t0.tv_sec)*1000000000ll+(t1.tv_nsec-t0.tv_nsec))/1e6,
   sms=(F)((t2.tv_sec-t1.tv_sec)*1000000000ll+(t2.tv_nsec-t1.tv_nsec))/1e6;
 fprintf(stderr,"par: threads=%d n=%u par_add=%.3fms serial_simd_add=%.3fms ok=%d\n",par_thread_count(n),n,pms,sms,ok);
 x(al((L)ok)))
Z A1(sam,x)V_;T_;U _K(A x/*0*/)_(X(R2(tu,tw,1)Rv(2)Rx(x>>48&15)Ropqr(xk)RA(rnk(x)))0)
X1(mkn,RmMA(e1f(mkn,x))Rt(x(_R(cn[xt])))R_(x(rsz(xN,_R(cn[xt])))))
A1(iei,/*0*/0x2332211004>>(xv*(xtv&&xv<10u)<<2)&15)
Y2(iex,/*01*/RmMA(r2f(iex,x,y))RT_A(rsz(yN,iex(x,fir(y))))Rs(as(0))Rc(ac(xtv&&xv-6<4u?-(xv==6||xv==9):"\0\1\x7f\x80 "[iei(x)]))Rf(y(af(A(0.,1.,WF,-WF,NF)[iei(x)])))R_(y(az(G(0ll,1,WL,-WL,NL)[iei(x)]))))
A2(ie,/*00*/x==CAT?emp(yt):iex(x,fir(yR)))
Z __attribute__((always_inline)) inline A prj_(A x,CO A*a,U n)_(XmMA(x8(a,n))U k=MAX(n,xK);F(n,k-=a[i]!=GAP)x=(xtp?val:aA1)(xR);I i=0,j=1;W(i<n&&j<xn,I(xA[j]==GAP,xA[j]=a[i++])j++)W(i<n,PSH(x,a[i++]))P(xn>9,ez(x))AT(tp,AK(k,x)))
AX(prj,prj_(x,a,n))   //prj_: inlined into d3s, d4s (an amend at more than one level), as prj(DOT,..) was
A2(com,/*01*/AK(yK,AT(tq,aA2(xR,y))))
Z A iM(A x,L i)_(Q(xtM);A y=xy,z=aA(yn);Q(ytA);Fj(zn|!zn,zA[j]=io(yA[j],i))am(_R(xx),sqz(z)))
A ii(A x/*0*/,U i)_(X(RA(_R(xa))RC(ac(xc))RG(ai(xg))RH(ai(xh))RI(ai(xi))RL(al(xl))RF(af(xf))RS(as(xi))Rm(ii(xy,i))RM(iM(x,i))RE(az(*xL+i))RB(ai(xG[i>>3]>>(i&7)&1))R_(xR))0)
A io(A x/*0*/,L i)_(X(RE(i<(W)(xL[1]-*xL)?az(*xL+i):_R(cn[tl]))RT_E(i<(W)xn?ii(x,i):xn?mkn(ii(x,0)):xtA?_R(xx):_R(cn[xt]))Rt(xR)Rm(io(xy,i))RM(iM(x,i)))0)
A1(fir,x(io(x,0)))A1(las,x(io(x,xN-1)))
ZN U maxfU(CO U*a,U n)_(U v=0;F(n,v=MAX(v,a[i]))v)
#define ambcn CO V*RES a,U m,CO U*RES b,V*RES c,U n
ZN V iG(ambcn){CO G*p=a;G*r=c;F(n,*r++=p[*b++])}
ZN V iH(ambcn){CO H*p=a;H*r=c;F(n,*r++=p[*b++])}
ZN V iI(ambcn){CO I*p=a;I*r=c;F(n,*r++=p[*b++])}
ZN V iC(ambcn){CO C*p=a;C*r=c;F(n+31&-32,*r++=b[i]<m?p[b[i]]:32)}
ZN V iS(ambcn){CO I*p=a;I*r=c;F(n+7&-8,*r++=b[i]<m?p[b[i]]: 0)}
ZN V oG(ambcn){CO G*p=a;L*r=c;F(n+3&-4,*r++=b[i]<m?p[b[i]]:NL)}
ZN V oH(ambcn){CO H*p=a;L*r=c;F(n+3&-4,*r++=b[i]<m?p[b[i]]:NL)}
ZN V oI(ambcn){CO I*p=a;L*r=c;F(n+3&-4,*r++=b[i]<m?p[b[i]]:NL)}
ZN V o8(ambcn,L v){CO L*p=a;L*r=c;F(n+3&-4,*r++=b[i]<m?p[b[i]]:v)}
ZN V oL(ambcn){o8(a,m,b,c,n,NL);}
ZN V oF(ambcn){o8(a,m,b,c,n,NFL);}
// Amber 2.5 (exp): a big gather split across threads (see patch header). es: bytes per output item.
#define PGAT_MIN (1u<<16)
TD struct{V(*f)(ambcn);CO V*a;U m;CO U*b;C*c;U n,nt,es;}GJ;
Z V gat_w(V*c_,int t){GJ*c=c_;U s=(U)(((W)c->n*t/c->nt)&~31ull),e=(U)t+1==c->nt?c->n:(U)(((W)c->n*(t+1)/c->nt)&~31ull);
 if(e>s)c->f(c->a,c->m,c->b+s,c->c+(N)s*c->es,e-s);}
Z V gat(V(*f)(ambcn),U es,CO V*a,U m,CO U*b,V*c,U n){int nt=n<PGAT_MIN?1:par_thread_count(n);
 if(nt<2){f(a,m,b,c,n);return;}GJ j={f,a,m,b,(C*)c,n,(U)nt,es};par_run(nt,gat_w,&j);}
// amber 2.7: a dict made `s (q's `s#d) looks up by steps: the value at the last key not above y, null below the
// first key. bin does numbers; other keys (symbols, by name) count the keys not above y.
Z A dstep(A k,A y)_(UC t=_t(k);P(LH(tB,t,tL)||t==tF,bin(k,y))K2("{[k;v]{[k;v]-1++/~v<k}[k]'v}",_R(k),y))
A2(i1,/*01*/P(y==GAP||y==au,xR)
 X(Rt(y(xR))
   RE(x=gZ(xR);x(i1(x,y)))
   Rm(i1(xy,N(_at(x)==1&&_t(xx)!=tM?dstep(xx,y):fnd(xx,y))))
   RM(Y(RsS(x=flp(xR);x(i1(x,y)))RA(r2(AP1,x,y))RmM(C t=yt;A z=kv(&y);A u=am(y,Ny(i1(x,z)));t==tM?flp(u):u)R_(B d=ytmt;y=N(l2f(i1,xy,y));(d?am:aM)(_R(xx),y)))0)
   R_(Y(Rilc(io(x,gl(y)))
        RmM(C t=yt;A z=kv(&y);A u=am(y,Ny(i1(x,z)));P(t==tM,flp(u))u)
        RA(r2(AP1,x,y))
        RE(L i=*yL,j=yL[1];P(0<=i&&i<j&&j<xN,y(0);slc(x,i,j))i1(x,gZ(y)))
        R2(tB,tC,i1(x,cI(y)))
        // amber 2.2: a short byte/short index vector (every literal index list
        // is stored that narrow) is widened on the stack, not into a fresh
        // 32-bit K vector that is allocated, filled and freed on every gather.
        R2(tG,tH,C t=xt;P(yn>256||t-tG>=7u,i1(x,cI(y)))U n=yn,b[288];I(yt==tG,F(n,b[i]=(U)(I)yg))E(F(n,b[i]=(U)(I)yh))
         B k=t-tG<3u&&maxfU(b,n)>=xn;A z=an(n,k?tL:t);G(&iG,iH,iI,oL,oF,iC,iS,oG,oH,oI)[7*k+t-tG](xV,xn,b,zV,n);y(z))
        R_(et(y))
        RL(A z=aI(yn);My(F(yn+3&-4,L v=yl;zi=v|-(v!=(I)v)))i1(x,z))
        RI(U n=yn;
         X(RA(A z=aA(n);F(n,za=io(x,yi))y(0);I(!n,zx=mkn(io(x,0)))sqz(z))
           RB(x=cG(xR);x(i1(x,y)))
           R_(C t=xt;B k=t-tG<3u&&maxfU(yV,yn)>=xn;A z=an(n,k?tL:t);My(U q_=7*k+t-tG;gat(G(&iG,iH,iI,oL,oF,iC,iS,oG,oH,oI)[q_],(U)"\1\2\4\10\10\1\4\10\10\10"[q_],xV,xn,yV,zV,n))z))0))0))0)
Z A3(i2,/*001*/C b=ytT||y==GAP||y==au;x=Nz(i1(x,yR));P(!b,x(x1(z)))x(l2f(dot,x,aA1(z))))
Z AX(i8,A y=*a;P(n==1,i1(x,y))P(n==2,y(_2(x,y,a[1])))a++;n--;C b=ytT||y==GAP||y==au;x=i1(x,y);P(!x,mrn(n,a);x)P(!b,x(i8(x,a,n)))x(l2f(dot,x,aV(tA,n,a))))
L iw(A x/*0*/,U w,L i)_(S4(w,_(xg),_(xh),_(xi),_(xl))0)
// ---- `xs : native multi-column grade for xasc / xdesc -----------------------
// x = (columns; descending?)  ->  the permutation, or () to tell the caller
// (amber.k) to use the portable K path.
//
// xasc was BY FAR the most expensive primitive left in the engine: on a 2M-row
// two-column table it measured ~4.6 s, more than the as-of join it exists to
// feed. The K definition is
//     xasc:{[c;t]c:c,();o:<+(+t)c; ...}
// and `+(+t)c` MATERIALISES ONE K LIST PER ROW -- 2M heap objects -- purely so
// that `<` has something row-shaped to compare. `<` on a list of lists is
// ascA() in src/o.c: a copying mergesort whose comparator (qA) walks two boxed
// rows element by element, allocating a boxed scalar per comparison. So the
// cost was O(n log n) *boxed* comparisons on top of 2M short-lived objects.
//
// Here nothing row-shaped is ever built. The columns are sorted LEAST
// SIGNIFICANT FIRST (last key column first), each with one stable LSD radix
// pass over an unsigned key extracted straight from the column's raw C array.
// That is O(n * total key bytes) with no comparisons and no allocation, and
// because LSD radix is stable, running the columns right-to-left reproduces
// lexicographic multi-column order exactly.
//
// SYMBOL COLUMNS collate BY NAME, which is what `<` on a symbol vector does and
// what the rest of the engine (ajsorted's collation note above) assumes. A
// packed symbol id has no relation to its spelling, so a symbol column is first
// reduced to a DENSE RANK: the distinct ids are collected with one open-
// addressed pass, ranked by handing them to asc() as a symbol vector (the same
// name collation, on the distinct set rather than on n rows), and each row then
// carries its rank as the radix key. A book with 5,000 symbols and 2M rows pays
// 5,000 string comparisons, not 2M boxed row comparisons.
//
// DESCENDING is the same machinery with complemented keys. K's `>` is
// "descending by value, ties by ASCENDING original index" (that is what
// dsc's rev/asc/rev identity in src/o.c computes), and complementing the key of
// a STABLE sort gives precisely that -- so xdesc goes down the same path.
#define XS_MAXCOL 32u
// One column -> n unsigned radix keys in `k`, gathered through the running
// permutation `ix`, plus the number of significant key BYTES. Returns 0 for a
// column type this kernel does not handle, which aborts the whole grade and
// sends the caller back to the K path.
Z B xssym(A c,CO I*RES ix,W*RES k,N n,U*nbo){
 CO U*RES p=(CO U*)_V(c);                       // tS: packed 32-bit ids
 // amber 2.1: interned ids are dense small integers, so a DIRECT table over
 // their range replaces the two hash passes whenever the range is modest
 // (up to 4M ids): one pass to collect the distinct ids, one to emit ranks.
 {U mn=p[ix[0]],mx=mn;for(N i=1;i<n;i++){U v=p[ix[i]];if(v<mn)mn=v;if(v>mx)mx=v;}
  W rg=(W)mx-(W)mn+1;
  if(rg<=((W)1<<22)){
   I*RES tb=(I*)arena_alloc((N)rg*SZ(I));U*RES dv=(U*)arena_alloc(n*SZ(U));U*RES rk=(U*)arena_alloc(n*SZ(U));
   if(tb&&dv&&rk){
    for(W s=0;s<rg;s++)tb[s]=-1;
    N nd=0;
    for(N i=0;i<n;i++){U v=p[ix[i]];I*t=tb+(v-mn);if(*t<0){*t=(I)nd;dv[nd++]=v;}}
    A g=asc(aV(tS,(U)nd,dv));                   // by NAME: asc(tS) -> asc(str x)
    P(!g,0)
    U gw=(U)(_w(g)-3);
    for(N r=0;r<nd;r++)rk[iw(g,gw,(L)r)]=(U)r;
    mr(g);
    for(N i=0;i<n;i++)k[i]=(W)rk[tb[p[ix[i]]-mn]];
    *nbo=nd<=256u?1u:nd<=65536u?2u:4u;
    return 1;}}}
 U cap=8;while((W)cap<(W)n*2)cap<<=1;
 I*RES ht=(I*)arena_alloc((N)cap*SZ(I));
 U*RES dv=(U*)arena_alloc(n*SZ(U));
 U*RES rk=(U*)arena_alloc(n*SZ(U));
 P(!ht||!dv||!rk,0)
 for(U s=0;s<cap;s++)ht[s]=-1;
 N nd=0;
 for(N i=0;i<n;i++){U v=p[ix[i]];
   U s=(U)(((W)v*0x9E3779B97F4A7C15ull)>>40)&(cap-1);
   while(ht[s]>=0&&dv[ht[s]]!=v)s=(s+1)&(cap-1);
   if(ht[s]<0){ht[s]=(I)nd;dv[nd++]=v;}}
 A g=asc(aV(tS,(U)nd,dv));                      // by NAME: asc(tS) -> asc(str x)
 P(!g,0)
 U gw=(U)(_w(g)-3);
 for(N r=0;r<nd;r++)rk[iw(g,gw,(L)r)]=(U)r;     // distinct slot -> dense rank
 mr(g);
 for(N i=0;i<n;i++){U v=p[ix[i]];
   U s=(U)(((W)v*0x9E3779B97F4A7C15ull)>>40)&(cap-1);
   while(dv[ht[s]]!=v)s=(s+1)&(cap-1);
   k[i]=(W)rk[ht[s]];}
 *nbo=nd<=256u?1u:nd<=65536u?2u:4u;
 return 1;}
Z B xskey(A c,CO I*RES ix,W*RES k,N n,U*nbo,int desc){
 CO V*q=_V(c);UC t=_t(c);
 switch(t){
  case tG:{CO G*RES p=q;for(N i=0;i<n;i++)k[i]=(W)AMKG(p[ix[i]]);*nbo=1;}break;
  case tC:{CO UC*RES p=q;for(N i=0;i<n;i++)k[i]=(W)p[ix[i]];*nbo=1;}break;   //chars order as unsigned bytes (issue #17)
  case tH:{CO H*RES p=q;for(N i=0;i<n;i++)k[i]=(W)AMKH(p[ix[i]]);*nbo=2;}break;
  case tI:{CO I*RES p=q;for(N i=0;i<n;i++)k[i]=(W)AMKI(p[ix[i]]);*nbo=4;}break;
  case tL:{CO L*RES p=q;for(N i=0;i<n;i++)k[i]=AMKL(p[ix[i]]);*nbo=8;}break;
  case tF:{CO F*RES p=q;F mn,mx;int so=0,spf=0;
   // amber 2.1: a float column holding integral values (prices, sizes, ids in
   // a tick table) is keyed by its INTEGER value, which orders identically and
   // needs 1-4 key bytes instead of 8 -- what lets a (sym;px) sort pack into
   // one radix below. NaN, -0.0 and non-integral columns keep the IEEE fold.
   // amber 2.2: the range scan's two halves are called in this order so the
   // < 2^32 span test -- which this path needs anyway and which rejects on the
   // range alone -- runs BEFORE the integrality pass instead of after it. A
   // column that is going to take the IEEE fold now costs one pass, not two.
   simd_frange0_f64(p,n,&mn,&mx,&so,&spf);
   if(!spf&&(mx-mn)<4294967296.0&&simd_fintegral_f64(p,n)){L lo=(L)mn;W sp=(W)((L)mx-lo);
     for(N i=0;i<n;i++)k[i]=(W)((L)p[ix[i]]-lo);*nbo=sp<256u?1:sp<65536u?2:sp<16777216u?3:4;}
   else{for(N i=0;i<n;i++)k[i]=amkFc(p[ix[i]]);*nbo=8;}}break;
  case tS: P(!xssym(c,ix,k,n,nbo),0) break;
  default: return 0;}
 // Complementing every key inverts the value order while the sort stays
 // stable. The key WIDTH is unaffected: the bytes above *nbo were identical
 // across the vector before the complement, so they still are.
 if(desc)for(N i=0;i<n;i++)k[i]=~k[i];
 return 1;}
A kys(A,A*,U*);
A xsC(A x){
 P(_t(x)-tA||_n(x)-2,x(emp(tA)))
 A*e=(A*)_V(x);A CS=e[0];
 P(_t(CS)-tA,x(emp(tA)))
 U nc=_n(CS);
 P(!nc||nc>XS_MAXCOL,x(emp(tA)))
 A*cv=(A*)_V(CS);
 N n=_n(cv[0]);
 P(!n||n!=(N)(U)(I)n,x(emp(tA)))                // grade indices are 32-bit
 F(nc,P(_tP(cv[i])||_n(cv[i])-(U)n,x(emp(tA))))
 // A generic column of dates, times or timestamps (ints or floats among them too) sorts as grade orders it:
 // by its keys (o.c kys), after its items' kinds when it holds more than one. Such columns are swapped for
 // their keys and the sort is asked again; a generic column of anything else is left to the K path. Each
 // column orders as < on it does: a leading "row class" key used to copy the K path's row grade, where a
 // row of one int squeezed to an int list and sorted after every generic row (digest #58)
 {B g=0;F(nc,g|=_t(cv[i])==tA)I(g,A b[2*XS_MAXCOL+1],kd[XS_MAXCOL];U m=1,w=0;
  F(nc,A q=cv[i],d=0;U s=0;kd[i]=0;I(_t(q)-tA,b[m++]=_R(q);continue)A k=kys(q,&d,&s);I(!k,mrn(m-1,b+1);return x(emp(tA));)w|=s;I(d,b[m++]=kd[i]=d)b[m++]=k)
  U o=1;(V)w;
  P(m-o>XS_MAXCOL,mrn(m-o,b+o);x(emp(tA)))return x(xsC(aV(tA,2,A(aV(tA,m-o,b+o),_R(e[1]))))));}
 int desc=tru(_R(e[1]));
 ArenaMark mk=arena_mark();
 I*cur=(I*)arena_alloc(n*SZ(I)),*alt=(I*)arena_alloc(n*SZ(I));
 W*kA=(W*)arena_alloc(n*SZ(W)),*kB=(W*)arena_alloc(n*SZ(W));
 P(!cur||!alt||!kA||!kB,arena_release(mk);x(emp(tA)))
 for(N i=0;i<n;i++)cur[i]=(I)i;
 // amber 2.1: when the columns' significant key bytes fit in 64 bits together
 // (a 100-symbol rank + an integral price is 3 bytes), pack them -- most
 // significant column highest -- and run ONE radix over the packed key from
 // the identity permutation, instead of one full key+index radix per column
 // gathering through the running permutation. The C reference sorts its
 // (sym,px) table this way; measured 2M rows: 143 ms -> ~75 ms.
 B packed=0;
 I(nc>=2&&(W)n*nc*8<=((W)1<<28),{
  ArenaMark cm=arena_mark();W*kc=(W*)arena_alloc(n*SZ(W)*nc);U nbc[XS_MAXCOL];U tot=0;
  I(kc,{
   for(U ci=0;ci<nc;ci++){P(!xskey(cv[ci],cur,kc+ci*n,n,&nbc[ci],desc),arena_release(mk);x(emp(tA)))nbc[ci]=amnorm(kc+ci*n,n,nbc[ci]);tot+=nbc[ci];}
   I(tot<=8,{
    // a column's key may keep CONSTANT high bytes (amnorm skips the translation
    // when it gains nothing; a descending complement sets every high bit), so
    // each key is masked to its significant bytes before it is shifted in.
    U sh[XS_MAXCOL];W mk_[XS_MAXCOL];U s=0;for(L ci=(L)nc-1;ci>=0;ci--){sh[ci]=s;s+=8*nbc[ci];mk_[ci]=nbc[ci]>=8?~0ull:((1ull<<(8*nbc[ci]))-1);}
    for(N i=0;i<n;i++){W v=0;for(U ci=0;ci<nc;ci++)if(nbc[ci])v|=(kc[ci*n+i]&mk_[ci])<<sh[ci];kA[i]=v;}
    I(tot,I*r=amrdx8(kA,cur,kB,alt,n,tot);if(r!=cur){alt=cur;cur=r;})
    packed=1;})})
  arena_release(cm);})
 I(!packed,for(L ci=(L)nc-1;ci>=0;ci--){
   U nb=0;ArenaMark cm=arena_mark();
   P(!xskey(cv[ci],cur,kA,n,&nb,desc),arena_release(mk);x(emp(tA)))
   nb=amnorm(kA,n,nb);                        // clustered column: fewer passes
   if(nb){I*r=amrdx8(kA,cur,kB,alt,n,nb);
          if(r!=cur){alt=cur;cur=r;}}         // nb==0: column is constant
   arena_release(cm);})                         // per-column symbol scratch
 A y=aI((U)n);MC(_V(y),cur,n*SZ(I));
 arena_release(mk);
 x(0);
 return ct(tZ((L)n-1),y);}

Z A1(qa,A y=emp(tA);S*p=argv;W(*p,PSH(y,aCz(*p++)))y(y1(x)))
Z A1(qe,A y=emp(tS),z=emp(tA);S*e=env;W(*e,S p=*e++,q=strchrnul(p,'=');PSH(y,cS(aCm(p,q)));PSH(z,aCz(q+!!*q)))y=am(y,z);x-au?x(x1(y)):y)
Z A1(qx,exit(xtz?gl(x):1);0)
Z X1(qjs,RC(C b[4096];U n=js_eval(xC,xn,b,SZ b);x(0);aCn(b,n))RA(e1f(qjs,x))R_(et(x)))
Z X1(qp,RC(x=str0(x);S s=xC;x(pk(&s,0)))R_(et(x)))
Z A1(qt,x(al(now())))
Z A1(qfb,P(!xtC,et(x))P(xn-8,el(x))x=rev(x);x(aV(tf,1,xV)))//float from bits
// amber 2.3: an attribute is a promise the engine acts on -- find binary-searches
// an `s vector, aj trusts the runs of an `s/`p column -- so it is only set when
// the data keeps it, as q does: `sa 3 1 2 is 's-fail, not a vector whose `?`
// quietly misses. The checks use the engine's own comparisons (the fused
// neighbour counts: 308 = descents, 310 = equal neighbours), so the collation is
// exactly the one sort and find use. An `s vector the engine sorted is trusted.
Z L atcnt(A x,L code)_(A fa[3];fa[0]=az(code);fa[1]=x;fa[2]=au;A c=fredC(fa,3);P(!c,-1)gl(c))
Z A1(qsa,UC t=_t(x);P(_tP(x)||!LH(tG,t,tS),x)P(_at(x)==1,x)L d=atcnt(x,308);P(d<0,x(0))P(d,x(err0("s-fail")))x=mut(x);_at(x)=1;x)//amber: `s sorted
Z A1(qua,UC t=_t(x);P(_tP(x)||!LH(tG,t,tS),x)P(_at(x)==2,x)A u=unq(_R(x));P(!u,x(0))U k=_N(u);mr(u);P(k!=xn,x(err0("u-fail")))x=mut(x);_at(x)=2;x)//amber: `u unique
Z A1(qpa,UC t=_t(x);P(_tP(x)||!LH(tG,t,tS),x)P(_at(x)==3,x)L e=atcnt(x,310);P(e<0,x(0))A u=unq(_R(x));P(!u,x(0))U k=_N(u);mr(u);
 P(xn&&(L)xn-e!=(L)k,x(err0("p-fail")))x=mut(x);_at(x)=3;x)//amber: `p parted: #runs = #distinct
// whether x keeps the promise of attribute a (1 `s, 2 `u, 3 `p; 4 `g promises nothing), by the
// checks `sa `ua `pa make, without an error: -9! (ser.c) keeps an attribute byte it reads only then
UC atok(A x,UC a)_(P(a<1||a>3||_tP(x)||!LH(tG,_t(x),tS),a==4)P(a==1,atcnt(x,308)==0)
 A u=unq(_R(x));P(!u,0)U k=_N(u);mr(u);P(a==2,k==_n(x))L e=atcnt(x,310);e>=0&&(L)_n(x)-e==(L)k)
Z A1(qga,UC t=_t(x);P(_tP(x)||!LH(tG,t,tS),x)x=mut(x);_at(x)=4;x)//amber: `g grouped
// ---- amber 2.7: q's `s#x `u#x `p#x `g#x, and `#x to take the attribute off --------------------------------
// The same answers as kdb+ 4.1 (checked case by case against q.exe, see docs/AMBER.md): every list may carry
// one, an atom is 'type; `s needs the order sort uses, `u no repeats, `p equal items next to each other ('p-fail,
// where q says 'u-fail); a general list is ordered as < orders it and cannot be `p. `s#t needs the whole table
// in row order: the table is `s and its first column `s (a one-column table) or `p. `s# on a dict or a keyed
// table checks the keys and makes the lookup a step function (the last key not above the probe, as bin finds).
Z A atset(A x,UC a)_(x=mut(x);_at(x)=a;x)
Z A atchk(UC c,A y){   //y a list (not B, not general): the checks `sa `ua `pa make; 0 and an error when it fails
 S(c,C('s',y=qsa(y))C('u',y=qua(y))C('p',y=qpa(y))C('g',y=qga(y)))return y;}
Z A atlist(UC c,A y){
 UC t=_t(y),a=c=='s'?1:c=='u'?2:c=='p'?3:4;
 P(t==tE,atlist(c,gZ(y)))
 P(LH(tG,t,tS),atchk(c,y))
 I(t==tB,A g=cG(_R(y));P(!g,y(0))g=atchk(c,g);P(!g,y(0))mr(g);return atset(y,a);)
 I(t==tA,I k=tjk(y);
  I(k==tdt||k==ttm||k==tnp,A l=tjn(y);P(!l,y(0))l=atchk(c,l);P(!l,y(0))mr(l);return atset(y,a);)   //dates and times: by their values
  P(c=='p',y(et0()))
  I(c=='s',A r=K1("{(!#x)~<x}",_R(y));P(!r,y(0))B ok=gl(r);P(!ok,y(err0("s-fail"))))
  I(c=='u',A u=unq(_R(y));P(!u,y(0))U k=_N(u);mr(u);P(k!=_n(y),y(err0("u-fail"))))
  return atset(y,a);)
 return y(et0());}
Z A atkeys(A y,UC lone){   //the keys of a sorted dict or keyed table: a list `s, a table in row order
 P(_t(y)!=tM,atlist('s',y))
 A r=K1("{v:. +x;o:`xs(v;0b);o:$[#o;o;{[o;v]o@<v o}/[!#x;|v]];o~!#x}",_R(y));P(!r,y(0))B ok=gl(r);P(!ok,y(err0("s-fail")))
 A d=_A(y)[1];U n=_n(d);P(!n||(n==1&&_N(_A(d)[0])<2),atset(y,1))   //a one-column table of 0 or 1 rows: no column attribute, as q
 A c0=atlist(n==1&&lone?'s':'p',_R(_A(d)[0]));P(!c0,y(0))
 A v=aA(n);F(n,_A(v)[i]=i?_R(_A(d)[i]):c0)A z=aV(tM,2,A(_R(_A(y)[0]),v));mr(y);_at(z)=1;return z;}
// amber 2.7: x within (lo;hi) in one pass for a numeric vector and two number bounds (q's within took four);
// anything else (atoms, symbols, a NaN bound) gives back 0N and amber.k's within does it as before
A wtnT(A x){P(_t(x)!=tA||_n(x)!=2,x(al(NL)))A v=_A(x)[0],r=_A(x)[1];UC t=_t(v);
 I(!_tP(v)&&t==tE&&!_tP(r)&&_n(r)==2&&(_t(r)==tG||_t(r)==tH||_t(r)==tI||_t(r)==tL),   //a range i..j (til n, the virtual i): one fill
   L i0=*(CO L*)_V(v),j0=((CO L*)_V(v))[1],lo=gl_(ii(r,0)),hi=gl_(ii(r,1));U n=(U)(j0-i0);A z=an(n,tG);MS(_V(z),0,n);
   L a=MAX(lo,i0),b=MIN(hi,j0-1);I(a<=b,MS((G*)_V(z)+(a-i0),1,(N)(b-a+1)))return x(z);)
 P(_tP(v)||!(t==tG||t==tH||t==tI||t==tL||t==tF)||_tP(r)||_n(r)!=2||!(_t(r)==tG||_t(r)==tH||_t(r)==tI||_t(r)==tL||_t(r)==tF),x(al(NL)))
 U n=_n(v);A z=an(n,tG);G*RES o=(G*)_V(z);UC rt=_t(r);
 I(t==tF,F lo=rt==tF?((CO F*)_V(r))[0]:(F)gl_(ii(r,0)),hi=rt==tF?((CO F*)_V(r))[1]:(F)gl_(ii(r,1));
   P(lo!=lo||hi!=hi,mr(z);x(al(NL)))CO F*RES p=(CO F*)_V(v);F(n,o[i]=p[i]>=lo&&p[i]<=hi)return x(z);)
 P(rt==tF,mr(z);x(al(NL)))L lo=gl_(ii(r,0)),hi=gl_(ii(r,1));
 S(t,C(tG,CO G*RES p=(CO G*)_V(v);F(n,o[i]=p[i]>=lo&&p[i]<=hi))C(tH,CO H*RES p=(CO H*)_V(v);F(n,o[i]=p[i]>=lo&&p[i]<=hi))
     C(tI,CO I*RES p=(CO I*)_V(v);F(n,o[i]=p[i]>=lo&&p[i]<=hi))C(tL,CO L*RES p=(CO L*)_V(v);F(n,o[i]=p[i]>=lo&&p[i]<=hi)))
 return x(z);}
A qattrs(C c,A y){
 UC t=_t(y);
 P(_tP(y)||_tt(y),y(et0()))
 I(!c,P(t==tE,y)return atset(y,0);)   //`#x
 I(t==tM,P(c!='s',y(et0()))return atkeys(y,1);)
 I(t==tm,P(c!='s',y(et0()))A k=atkeys(_R(_A(y)[0]),_t(_A(y)[0])==tM);P(!k,y(0))A z=aV(tm,2,A(k,_R(_A(y)[1])));mr(y);_at(z)=1;return z;)
 return atlist((UC)c,y);}
// amber: `diag 0 / `diag 1 -- turn the Rust-style stderr diagnostic off/on at
// runtime, returning the PREVIOUS setting so a caller can restore it. Needed by
// anything that deliberately provokes errors it then catches (tests/harness.k,
// std.k's protect); see the note above amdiag in e.c.
Z A1(qdiag,I(amdiag<0,amdiag=({S dgev=getenv("AMBER_DIAG");!dgev||*dgev!='0';}))   //unset: from $AMBER_DIAG, as eD resolves it (it was 1, so AMBER_DIAG=0 lost: digest #77)
 I v=amdiag;I(_tz(x),amdiag=!!gl_(x))x(0);ai(v))
// amber: `srt x -- SORT (not grade). Takes the counting-sort path in src/v.c
// when the value range is small relative to n, and otherwise reproduces the
// previous K definition of asc verbatim, so semantics (collation, the `s
// attribute, every non-integer type) are unchanged.
Z A1(qsrt,srtC(x))
Z A1(qat,UC a=(_tP(x)||_tt(x)||_t(x)==tE)?0:_at(x);x(0);a?({C b[2]={"\0supg"[a],0};sym(b);}):as(0))//amber: get attribute
ZN AX(ext,P(n-xK,er8(a,n))V*f=(V*)(x&-1ull>>16);S(n,R(1,((A1*)f)(a[0]))R(2,((A2*)f)(a[0],a[1]))R(3,((A3*)f)(a[0],a[1],a[2]))R(4,((A4*)f)(a[0],a[1],a[2],a[3]))R_(en8(a,n)))0)
// `sumn x: +/x with the int null counted as 0, in one pass (amber.k's q-style sum). Only a 64-bit int
// vector can hold 0N; anything else is +/ as it is. Wraps like +/ (unsigned adds).
// `prn x: */x with the int null counted as 1 (amber.k's q-style prd), four lanes (int products wrap the same in any
// order). `hnl x: might x hold a null - a scan of a 64-bit int or float list, 0 at once for anything else (a narrower
// int list can't hold 0N). Both digest #43
Z A1(prnT,P(!_tP(x)&&_t(x)==tL,CO L*RES p=_V(x);U n=_n(x);W t0=1,t1=1,t2=1,t3=1;U i=0;
 for(;i+4<=n;i+=4){L v0=p[i],v1=p[i+1],v2_=p[i+2],v3=p[i+3];t0*=(W)(v0==NL?1:v0);t1*=(W)(v1==NL?1:v1);t2*=(W)(v2_==NL?1:v2_);t3*=(W)(v3==NL?1:v3);}
 for(;i<n;i++){L v0=p[i];t0*=(W)(v0==NL?1:v0);}x(az((L)(t0*t1*t2*t3))))K1("{*/x}",x))
Z A1(hnlT,I hz=0;I(!_tP(x)&&_t(x)==tL,CO L*RES p=_V(x);F(_n(x),hz|=p[i]==NL))J(!_tP(x)&&_t(x)==tF,CO F*RES p=_V(x);F(_n(x),hz|=p[i]!=p[i]))x(ai(hz)))
Z A1(sumnT,P(!_tP(x)&&_t(x)==tL,CO L*RES p=_V(x);U n=_n(x);W t=0;F(n,L v=p[i];t+=(W)(v==NL?0:v))x(az((L)t)))K1("{+/x}",x))
ZN A sym1(I v,A x)_(V*amxf=am_ext_verb_lookup(v);P(amxf,((A1*)amxf)(x))Z CO C s[][4] __attribute__((aligned(4)))={"k","j","p","t","x","hex","err","argv","env","exit","js","pri","prng","sin","cos","exp","ln","fb","sa","ua","pa","ga","at","pe","ema","wj","mkd","mkt","mkp","plt","cdl","aex","aim","bi","aj","arn","dgn","simd","vmd","para","csvr","csv0","csvx","astt","diag","ajs","wjb","mw","xs","srt","rdl","sbb","sbt","wsm","memb","gagg","sumn","ejx","cvm","prn","hnl","abs","wcol","rcol","fsz","ldir","wtn","senc","hh","mm","sec","tjn"};
 G(&kst,js1,qp,qt,frk,hex,err,qa,qe,qx,qjs,qpri,prng,ksin,kcos,kexp,klog,qfb,qsa,qua,qpa,qga,qat,peachC,emaC,wjc,mkdt,mktm,mknp,plotC,candleC,arrowExport,arrowImport,binfo,ajc,arnT,dgnT,simdT,vmdT,parT,csvrT,csv0T,csvxT,astT,qdiag,ajsC,wjbC,mwC,xsC,qsrt,rdlC,sbbC,sbtC,wsmC,membC,gaggT,sumnT,ejxC,cvmC,prnT,hnlT,kabs,wcolT,rcolT,fszT,ldirT,wtnT,sencT,hhC,mmC,secC,tjnC,ed)[fI((V*)s,L(s),v)](x))
/* ---- tacit trains: hook (f g) and fork (f g h) --------------------------
 * A general list of length 2 or 3 whose every element is a function becomes a
 * TRAIN when it is applied: (f g) is a hook, (f g h) a fork (APL/J/BQN rules).
 *   hook   monadic  (f g)   y  = y f (g y)      dyadic  x (f g)   y = x f (g y)
 *   fork   monadic  (f g h) y  = (f y) g (h y)  dyadic  x (f g h) y = (x f y) g (x h y)
 * A primitive element is applied straight through the index-aligned monad/dyad
 * tables (v1/v2) by its verb index, so it never projects; any other function
 * element (lambda, projection, composition, derived verb) goes through _1/_2.
 * Elements are borrowed from x (freed together with x by the caller); the
 * arguments are owned as by _1/_2 and ap1/ap2: trn1 consumes y, trn2 borrows
 * its left argument y and consumes z. An element that fails frees what is
 * still held and passes the error (0) on rather than applying to it. */
Z I istrain(A x){if(_t(x)!=tA)return 0;U m=_n(x);if(m!=2&&m!=3)return 0;A*e=_A(x);for(U i=0;i<m;i++)if(!TU(_t(e[i])))return 0;return(I)m;}
Z A ap1(A f,A y){UC t=_t0(f);if(t==tu||t==tv)return v1[_v(f)](y);return _1(f,y);}
Z A ap2(A f,A y,A z){UC t=_t0(f);if(t==tu||t==tv)return v2[_v(f)](y,z);return _2(f,y,z);}
Z A trn1(A x,A y,I m){A*e=_A(x);if(m==2){A g=Ny(ap1(e[1],_R(y)));return y(ap2(e[0],y,g));}A f=Ny(ap1(e[0],_R(y))),h=N(ap1(e[2],y),mr(f));A r=ap2(e[1],f,h);mr(f);return r;}
Z A trn2(A x,A y,A z,I m){A*e=_A(x);if(m==2){A g=N(ap1(e[1],z));return ap2(e[0],y,g);}A l=Nz(ap2(e[0],y,_R(z))),r=N(ap2(e[2],y,z),mr(l));A v=ap2(e[1],l,r);mr(l);return v;}
A2(_1,/*01*/{I tn=istrain(x);if(tn&&!(_tz(y)&&gl_(y)>=0&&gl_(y)<(I)_n(x)))return trn1(x,y,tn);}P(!xtt,i1(x,y))U k=xK;P(1<k,k==2&&!xtp?prj(x,A8(y,GAP),2):prj(x,&y,1))
 X(Ro(run(x,&y,1))Rp(P(k>7,er(y))I m=xn-1,j=0;Ab8;F(m,b[i]=xA[i+1]==GAP&&!j?j++,y:_R(xA[i+1]))I l=MAX(0,1-j);MC(b+m,&y,8*l);_8(xx,b,m+l))
  Rq(_1(xx,N(_1(xy,y))))Rr(w1(xE,xx,y))Rs(sym1(xv,y))Ru(v1[xv](y))Rw(AK(xv-1<3u&&yK==2?1:ytU?yK:1,AW(xv,aV(tr,1,&y))))Rx(ext(x,&y,1))R_(et(y)))0)
A3(_2,/*001*/{I tn=istrain(x);if(tn)return trn2(x,y,z,tn);}P(!xtt,i2(x,y,z))A a[]={y,z};U k=xK;P(2<k,yR;prj(x,a,2))
 X(Ro(yR;run(x,a,2))Rp(P(k>6,er(z))yR;I m=xn-1,j=0;Ab8;F(m,b[i]=xA[i+1]==GAP&&j<2?a[j++]:_R(xA[i+1]))I l=MAX(0,2-j);P(l+m>8,mrn(m,b);er8(a+j,2-j))MC(b+m,a+j,8*l);_8(xx,b,m+l))
  Rq(_1(xx,N(_2(xy,y,z))))Rr(z(w2(xE,xx,yR,z)))Rv(v2[xv](y,z))Rw(P(!xv,com(y,z))x=Nz(x1(yR));x(x1(z)))Rx(ext(x,A8(yR,z),2))R_(et(z)))0)
AX(_8,/*01..1*/Q(n)P(n==1,x1(*a))P(n==2&&!xtp,A y=*a;y(x2(y,a[1])))P(!xtt,i8(x,a,n))U k=xK;P(n<k,prj(x,a,n))
 X(Ro(run(x,a,n))Rp(I m=xn-1,j=0;Ab8;F(m,b[i]=xA[i+1]==GAP&&j<n?a[j++]:_R(xA[i+1]))I l=MAX(0,n-j);P(l+m>8,mrn(m,b);er8(a+j,n-j))MC(b+m,a+j,8*l);_8(xx,b,m+l))
   Rq(_1(xx,N(_8(xy,a,n))))Rr(w8(xE,xx,a,n))Rv(x=v8[xv](a,n);mrn(n-1,a+1);x)Rx(ext(x,a,n))R_(et8(a,n)))0)
A1(jS,cS(jc('.',str(x))))//join symbols with "."
X1(val,RA(P(!xn,x)P(xn==1,fir(x))P(xn>9,ez(x))x=mut(x);A y=_8(xx,&xy,xn-1);AN(1,x);x(y))RmM(x(_R(xy)))RE(gZ(x))RC(x=str0(x);x(evs(xV,0)))Rc(val(enl(x)))RsS(gg(x))
 Ropq(AT(tA,mut(x)))Rr(cat10(AT(tA,mut(x)),aw+xE))Ruvw(ai(xv))R_(x))
A2(dot,/*01*/Ym(et(y))U n=yN;P(!n,y(xR))P(n>8,ez(y))y=mRa(N(blw(y)));y(x8(yA,n)))
Z U knd(A x/*0*/)_(X(Ril(ti)REBGHIL(tI)R_(xt))0)
Z AM_TLS_IE I nsq;   //>0: inside amend's per-index fold, which squeezes once at the end (a squeeze per item made it quadratic)
#define USQ(e) ({I k_=nsq;nsq=0;A r_=(e);nsq=k_;r_;}) //run code the amend calls out to (verbs, lambdas) with squeezing back on
Z A set(A x,L i,A y/*1i1*/)_(Q(MINE(x));
 X(RA(A z=xa;xa=z(y);ytt&&!ytU&&!nsq?sqz(x):x)
   RM(A z=kv(&x);z=mut(z);Q(ztA);I(ytT&&yN-zn,x(y(el(z))))I j=i;F(zn,za=set(mut(za),j,ii(y,i));P(!za,za=au;x(y(z(0)))))y(aM(x,z)))
   RB(set(cG(x),i,y))
   R_(P(knd(x)-knd(y)-tC+tc,set(blw(x),i,y))I(xtZ,N(sup(&x,&y)))C w=xw-3;!w?xg=yv:w==1?xh=yv:w==2?xi=yv:(xl=gl(y));x))0)
// amber: the MC()s below used to copy a fixed 8-slot's worth of argument
// pointers (56/48/40/64 bytes) out of the caller's `a[]` no matter how many
// arguments were actually passed. `a` is not always an 8-element buffer --
// run() (b.c) hands over a pointer straight into its own dynamically sized
// stack frame -- so every amend with n<8 read past the end of live storage
// (ASan: dynamic-stack-buffer-overflow, reproducible on test-fin.k and four
// examples). The extra slots were never *used* (each recursive call is bounded
// by n), so this is a pure read overrun -- but it is still one, and on a
// stack-probing/tagged-memory target it faults. AC() clamps each copy to the
// number of arguments that actually exist.
#define AC(d,s,c) {I c_=(I)(c);I(c_>0,MC(d,s,8u*(U)c_))}
//amend a dict at a list of keys whose places z come from one find, not a find per key, which made it quadratic.
//With every key present that is one amend of the values. Otherwise the keys go in order, as the fold over them did:
//a key not yet in the dict is added at its first use, with the value ie gives from the values at that point.
//Deeper (n==5, .[d;(k;i);f;y]) the keys always go in order, squeezing as the fold did: after each atom, not at the end
Z A dam(A x,A y,A z,CO A*a,U n/*10100*/)_(x=mut(x);U m=0;F(zn,m+=zL[i]==NL)
 P(!m&&n<5,Ab8;*b=xy;b[1]=z;AC(b+2,a+2,n-2);xy=au;xy=Nx(z(a8(b,n)));x)
 A w=aL(m);m=0;F(zn,I(zL[i]==NL,_L(w)[m++]=i))A k=i1(y,_R(w)),p=k?fnd(k,_R(k)):0;I(k,mr(k))I(p,p=cL(p))P(!p,mr(w);z(x(0)))
 L c=_N(xx);z=mut(z);F(m,L q=_L(p)[i];_L(p)[i]=q==i?c++:_L(p)[q];zL[_L(w)[i]]=_L(p)[i])mr(p);mr(w);B t=n>3&&!_tt(a[n-1]),s=n<5;nsq+=s;
 F(zn,L j=zL[i];I(j==_N(xx),PSH(xx,ii(y,i));PSH(xy,ie(a[2],xy)))Ab8;*b=xy;b[1]=az(j);AC(b+2,a+2,n-2);I(t,b[n-1]=ii(a[n-1],i))
  xy=au;A v=a8(b,n);mr(b[1]);I(t,mr(b[n-1]))P(!v,nsq-=s;z(x(0)))xy=v)
 nsq-=s;I(s,xy=sqz(xy))z(x))
//t[c]:y and @[t;c;f] amend a table as the dict of its columns and flip it back (x, the dict amended). The flip gives an
//atom column the count of the columns that are lists, so with none left it made one row (+(,`a)!,,9 from three), or
//'length where the atoms were of two types: tfl gives each atom column the table's old row count, r (an empty table
//stays empty), as update and the amend while a list column remains do. Lists replacing every column still give their
//count (t[`a]:7 8 on a one-column table: two rows). Not in flip, which knows no row count (+`a`b!1 2 is one row)
ZN A tfa(A x,U r/*1.*/)_(A y=xy;P(!_n(y),flp(x))A v=aA(_n(y));F(_n(y),_A(v)[i]=rsz(r,ii(y,i)))x=mut(x);mr(xy);xy=v;flp(x))   //every column an atom: r of each (no column: as before)
Z A tfl(A x,U r/*1.*/)_(A y=xy;I(_tA(y),F(_n(y),P(!_tt(_A(y)[i]),flp(x))))tfa(x,r))   //a column a list (the first, nearly always): flip
AA(a8,/*10..0*/A x=*a,y=a[1];
 X(RE(Ab8;*b=gZ(x);AC(b+1,a+1,n-1);a8(b,n))
   RT_E(P(y==au,mRn(n-2,a+2);Ab8;*b=a[2];b[1]=x;AC(b+2,a+3,n-3);USQ(e8(AP1,b,n-1)))
    Yzc(L i=gl_(y);P(i>=(W)xn,ei(x))x=mut(x);Ab8;*b=ii(x,i);AC(b+1,a+3,n-3);mRn(n-3,b+1);A z=a[2];set(x,i,Nx(USQ(z8(b,n-2)))))
    I(ytZC&&n==4,A z=a[2],u=a[3];P(xtZ&&ztv&&utzZ&&(0xcf&1<<zv),ara(x,y,z,u))P(xtC&&z==av&&utcC,cC(N(ara(x,y,z,u)))))Yt(et(x))mRn(n-1,a+1);nsq++;A r_=f8(AP1,a,n);nsq--;r_?sqz(r_):0)
   Rm(P(y==au,A k=_R(xx);Ab8;*b=x;b[1]=k;AC(b+2,a+2,n-2);x=a8(b,n);mr(k);x)   //:: is every key, as it is when reading d[::;..]: the keys (!d), as d[!d;..]
      A z=Nx(fnd(xx,yR));ZT(P(LH(tG,zt,tL)&&(n==3||n==4&&(_tt(a[3])||_tT(a[3])&&_N(a[3])==zn)||n==5&&_tt(a[3])&&(_tt(a[4])||_tT(a[4])&&_N(a[4])==zn)),dam(x,y,cL(z),a,n))z(0);mRn(n-1,a+1);f8(AP1,a,n))x=mut(x);I(ztl,z=mut(z);F(zN,I(zl==NL,zl=xN;PSH(xx,ztt?yR:ii(y,i));PSH(xy,ie(a[2],xy)))))
    Ab8;*b=xy;b[1]=z;AC(b+2,a+2,n-2);xy=au;xy=Nx(z(a8(b,n)));x)
   RM(Ab8;AC(b,a,n);YsS(U r=xN;*b=flp(x);tfl(N(a8(b,n)),r))B e=!xN;*b=blw(e?_R(x):x);A p=e?_R(*_A(*b)):0;A u=a8(b,n);P(!u,e?(mr(p),x(0)):0)P(!e,sqz(u))   //p: the prototype of an empty table's rows, its null row
    B m=_tA(u)&&!_n(u)&&_tm(ux)&&mtc_(ux,p);mr(p);m?u(x):x(sqz(u)))   //no amend reached the null row: the table as it was (one that did comes back a table already)
   RU(mRn(n-1,a+1);x(USQ(x8(a+1,n-1))))
   R_(et(x)))0)
//ixst (with ixwk, b.c): .[x;y;f;z] at the places in y that ixwk found (kd), as a8 does it but without looking them up
//again (from a table's row on, and where the walk stopped short below the first level, d8 does it, as d4's projection
//does; from a key to add above the last level, a8; and from a list of keys above the last level, dam, at the places
//the check found). An item on the way is taken out of its list, so it is amended in place. z 0: .[x;y;f], f of one argument.
//f 0: z, taken, is the item's new value (a verb's result, b.c ixca), where every level is one place that is there (ixit).
//u: where f fails at the last place, and every level above is one that is there (0, 1, 2, 6: not a key to add), each
//level puts back the item it took out, and *u is the value as it was (or a copy of it), so the variable can be put back
Z A ixsl(A x,A y,CO UC*kd,CO L*ix,U k,U m,A f,A z,A*u);
Z A rbl(A),d3(A,A,A),a5(A,A,A,A,A),d3t(A,A,A),d4t(A,A,A,A),dt3(A,A,A),dt4(A,A,A,A);
//a key (1) at the last level, or a table's column (2) in the table flipped to a dict (tb): inlined in each, so that a
//dict's has nothing of the table's (its old row count, kept across the amend for tfl)
Z __attribute__((always_inline)) inline A ixsd(A x,A y,CO UC*kd,CO L*ix,U k,U m,A f,A z,A*u,B tb/*10....00..*/)_(x=mut(x);
 I(ix[k]==_N(xx),u=0;A s=_tt(y)?y:_tA(y)?_A(y)[k]:ii(y,k);PSH(xx,_R(s));PSH(xy,ie(f,xy)))   //a key not there (the last level): added, as a8 does
 A v=xy;xy=au;v=ixsl(v,y,kd,ix,k,m,f,z,u);P(!v,u&&*u?(xy=*u,*u=tb?flp(x):x,(A)0):x(0))xy=v;x)
Z NI A ixtc(A x,A y,CO UC*kd,CO L*ix,U k,U m,A f,A z,A*u/*10....00.*/)_(U r=xN;x=ixsd(flp(x),y,kd,ix,k,m,f,z,u,1);x?tfl(x,r):0)   //a column (tfl)
A ixst(A x,A y,CO UC*kd,CO L*ix,U k,U m,A f,A z,A*u/*10....00.*/)_(UC t=kd[k];P(!t,ixsl(x,y,kd,ix,k,m,f,z,u))
 P(t==3,A w=k?drp(k,yR):yR;B s=k&&!_tP(x)&&_t(x)==tS;x=USQ(s?z?d4t(x,w,f,z):d3t(x,w,f):z?d8(A8(x,w,f,z),4):d8(A8(x,w,f),3));mr(w);x)   //d8, not d4: a symbol below
  //the first level names a global; a symbol list there is data, and so are its items (d4t, d3t)
 P(t==9,A s=_A(y)[k],p=(A)ix[k],v=prj(z?ax(dt4,4):ax(dt3,3),(A[]){GAP,drp(k+1,yR)},2);B tb=_t(x)==tM;I(tb,x=flp(x))U n=z?5:4;A b[5]={x,s,v,f,z};   //keys above the last level:
  x=LH(tG,_t(p),tL)&&(n==4?_tt(f)||_tT(f)&&_N(f)==_n(p):_tt(f)&&(_tt(z)||_tT(z)&&_N(z)==_n(p)))?dam(x,s,cL(p),b,n):(mr(p),a8(b,n));mr(v);P(!x,0)tb?flp(x):x)   //as a8 does (Rm), the find from ixwk
 P(t==6,U j=(U)ix[k+1];x=mut(x);xy=mut(xy);A c=_A(xy)[j];_A(xy)[j]=au;c=ixsl(c,y,kd,ix,k,k+1,f,z,u);P(!c,u&&*u?(_A(xy)[j]=*u,*u=x,(A)0):x(0))_A(xy)[j]=c;   //a row then a column: that column
  F(_n(xy),A*p=_A(xy)+i;I(_tA(*p),*p=rbl(*p)))x)   //amended at the row, as d4 does it (tca)
 P(t>6,I(t==8,x=flp(x))x=mut(x);A s=_tA(y)?_A(y)[k]:ii(y,k);PSH(xx,_R(s));PSH(xy,ie(av,xy));A v=xy;xy=au;v=ixsl(v,y,kd,ix,k,m,f,z,0);P(!v,x(0))xy=v;t==8?flp(x):x)   //a key to add,
  //then an index into its value, the first value nulled (as for d4's projection, ie gives the nulled first value for :), which keeps its count
 P(t>3,I(t==5,x=flp(x))x=mut(x);A s=_tA(y)?_A(y)[k]:ii(y,k),v=prj(z?ax(dt4,4):ax(dt3,3),(A[]){GAP,drp(k+1,yR)},2);PSH(xx,_R(s));PSH(xy,ie(v,xy));   //a key to add above the last level:
  A w=xy;xy=au;w=z?a8(A8(w,az(ix[k]),v,f,z),5):a8(A8(w,az(ix[k]),v,f),4);mr(v);P(!w,x(0))xy=w;t==5?flp(x):x)   //the rest as a8 does it (d4's projection), without finding the key again
 P(t==2,ixtc(x,y,kd,ix,k,m,f,z,u))ixsd(x,y,kd,ix,k,m,f,z,u,0))   //a column: in the table flipped to a dict (ixtc)
Z A ixsl(A x,A y,CO UC*kd,CO L*ix,U k,U m,A f,A z,A*u)_(U i=(U)ix[k];I(_t(x)==tE,x=gZ(x))P(_t(x)==tM,x=ixsl(blw(x),y,kd,ix,k,m,f,z,u);I(!x&&u&&*u,*u=sqz(*u))x?sqz(x):0)x=mut(x);
 P(k+1==m,A r=!f?z:z&&f==av?_R(z):USQ(z?_8(f,A8(ii(x,i),_R(z)),2):_8(f,A8(ii(x,i)),1));   //the item at the last place: f applied to it (: needs not read it)
  I(__builtin_expect(kd[k]==2,0)&&r&&_n(x)>1&&!_tP(r)&&_tT(r)&&_N(r)!=_N(_A(x)[!i]),mr(r);r=el0())   //a table's column (2): a list of another count does not fit
  P(!r,u?(*u=x,(A)0):x(0))set(x,i,r))
 A w;I(_t(x)==tA,w=_A(x)[i];_A(x)[i]=au)E(w=ii(x,i))w=ixst(w,y,kd,ix,k+1,m,f,z,u);P(!w,u&&*u?(_tA(x)?(V)(_A(x)[i]=*u):mr(*u),*u=x,(A)0):x(0))set(x,i,w))
A ixv2(UC d,A q,A z/*00.*/)_(USQ(v2[d](q,_R(z))))   //b.c ixca: the verb on the item ixit read, which it goes first on
Z A3(a3,/*100*/a8(A8(x,y,z),3))
A4(a4,/*1000*/a8(A8(x,y,z,u),4))
Z A a5(A x,A y,A z,A u,A v/*10000*/)_(a8(A8(x,y,z,u,v),5))
//.[t;(i;c),p] of a table, at a row and one of its columns, amends that column at (i),p, by d3/d4 (. takes a symbol
//column for a name). It made a list of every row and a table of it again, which keeps each column's values but drops
//its attribute (so only without one; one inside a column that is a table is kept, as by t[c;i]:y) and builds each
//generic column again item by item, as psh does: rbl
U tcc(A x,A s/*00*/)_(P(!_tS(xx),0)F(_n(xy),A c=_A(xy)[i];P(_at(c)||!_tT(c)&&!_tM(c),0))U j=fI(_I(xx),_n(xx),_v(s));j<_n(xx)?j+1:0)   //column s of t, +1 (0: not this way)
Z U tci(A x,A y/*00*/)_(P(!xtM||!ytA||yn<2||!_tz(*yA)||!_ts(yA[1]),0)tcc(x,yA[1]))
Z A1(rbl,A y=enl(_R(*xA));F(xn-1,PSH(y,_R(xA[i+1])))x(y))
Z A d3(A,A,A);Z A tca(A x,A y,A z,A u,U n,U j/*10000.*/)_(A w=aA(yn-1);*_A(w)=_R(*yA);F(yn-2,_A(w)[i+1]=_R(yA[i+2]))
 x=mut(x);xy=mut(xy);A c=_A(xy)[j];_A(xy)[j]=au;c=n==4?d4(c,w,z,u):d3(c,w,z);mr(w);P(!c,x(0))_A(xy)[j]=c;F(_n(xy),A*p=_A(xy)+i;I(_tA(*p),*p=rbl(*p)))x)
//.[t;(::;c),p;f(;y)] amends each row of t: with none, the rows made nothing to squeeze back, and the result was (),
//losing the table (q keeps it, as update does). The table, as it is, where f and y are atoms or empty: a list or dict
//with items is 'length, as before
Z B tnr(A x,A y,A z,A u/*0000*/)_(xtM&&ytA&&*yA==au&&!xN&&(_tt(z)||!_N(z))&&(!u||_tt(u)||!_N(u)))
//an amend at more than one level amends each item at the first with the rest of the indices, by a projection of a
//function that does what . does there (dt3, dt4), but for a symbol list, which is data (. takes it as a name), and so
//are its items (s: x is such a list, and ds3, ds4 do its items)
Z A ds3(A,A,A),ds4(A,A,A,A);
Z __attribute__((always_inline)) inline A d3s(A x,A y,A z,B s/*100.*/)_(U m=yN;P(y==au||!m,z1(x))P(m==1,y=fir(yR);y(a3(x,y,z)))U j=tci(x,y);P(j,tca(x,y,z,0,3,j-1))P(tnr(x,y,z,0),x)A u=s?prj(ax(ds3,3),(A[]){GAP,drp(1,yR)},2):prj_(ax(dt3,3),(A[]){GAP,drp(1,yR)},2);y=fir(yR);y(u(a4(x,y,u,z))))
Z __attribute__((always_inline)) inline A d4s(A x,A y,A z,A u,B s/*1000.*/)_(U m=yN;P(y==au||!m,x(z2(x,uR)))P(m==1,y=fir(yR);y(a4(x,y,z,u)))U j=tci(x,y);P(j,tca(x,y,z,u,4,j-1))P(tnr(x,y,z,u),x)A v=s?prj(ax(ds4,4),(A[]){GAP,drp(1,yR)},2):prj_(ax(dt4,4),(A[]){GAP,drp(1,yR)},2);y=fir(yR);A r=y(a5(x,y,v,z,u));mr(v);r)
Z A3(d3,/*100*/d3s(x,y,z,0))
A4(d4,/*1000*/d4s(x,y,z,u,0))
Z __attribute__((cold)) A3(d3t,/*100*/d3s(x,y,z,1))   //s a constant in each, so d3, d4 inline prj_ as they did prj(DOT,..); a symbol list's
Z __attribute__((cold)) A4(d4t,/*1000*/d4s(x,y,z,u,1))   //own amend is rare (cold: kept out of the way of the hot code, at the binary's end)
Z AA(d8_,/*10..0*/A x=*a,y=a[1],z=a[2];P(n==4,d4(x,y,z,a[3]))P(n==3,d3(x,y,z))en(x))
I ixwk(A,A,A,B,B,UC*,L*),ixgn(A*),ixgh(A*);A ixit(A,A,UC*,CO L*,U,B*);
//d8, where the walk found nothing to assign (w_ 0): read the item first, as d8 does at a row, where it is a table's row with
//more below it, or, in an assignment by d4 (g_ found again here, so d8 need not keep it) of another global (gd), where the
//walk finds it: its new value is then assigned with : at the places found (ixst), not by d4, which would copy the global as
//it is held here; ixst there never amends by name. *h 0: not so, and d8 goes on as before (b as it was)
Z NI __attribute__((cold)) A d8q(A*p,A*b,A*a,I n,UC*kd,L*ix,B*h){I g_=ixgn(p);B gd=g_&&!ixgh(p),o;I w=gd?ixwk(*b,a[1],n==4?a[3]:au,0,0,kd,ix):0;
 P(w<0||!gd&&(g_||_t(*b)!=tM),*h=0,(A)0)A q=ixit(*b,a[1],kd,ix,(U)w,&o);P(!(*h=!!q),I(w>0&&kd[w-1]==9,mr((A)ix[w-1]))(A)0)I(!w,w=1)   //(the find for a list of keys, 9, unused)
 I(!o,_R(q))A r=USQ(n>3?_8(a[2],A8(q,_R(a[3])),2):_8(a[2],A8(q),1));P(!r,I(*p&&*p!=au,mr(*b))E(*p=*b);0)I(*p&&*p!=au,mr(*p);*p=au)   //as d8 does
 A u=0;q=ixst(*b,a[1],kd,ix,0,(U)w,av,r,&u);mr(r);P(!q,I(u,*p=u)(A)0)return *p=_R(q);}   //(u: a result that does not fit puts the value back)
AA(d8,/*10..0*/A x=*a;
I ixwk(A,A,A,B,B,UC*,L*),ixck(A,A,U,A,B),ixgn(A*),ixgi(A*);A ixit(A,A,UC*,CO L*,U,B*);   //b.c: is .[`v;i;f;y] sure to fail on its index, count or type? (then v is not touched) If not, where can ixst assign? (.[`v;i;f] is not checked)
 //while run assigns a global (ixgs): as before (ixck, d4) in an assignment by d4, and refused if this one is assigned in place (ixgi).
 //Where ixit reads the item, f goes first, on it (v :: meanwhile, as before), and ixst assigns its result with :, so if f
 //fails, v is put back (unless f set v, to other than ::). Five or more arguments, which amend does not take: 'nyi, v kept.
 //In an assignment by d4 (and v not the one assigned), where the walk finds the item, f goes first too, and then : as before
 X(RsS(P(ray_rc_sync,mr(*a);err0("noupdate"))A*p=gp(x);P(!p,0)I g_=ixgn(p);P(n>2&&g_&&ixgi(p),et0())P(n>4,en0())UC kd[8];L ix[8];
   I w_=n==3||n==4?g_?n<4?0:*p&&_t(*p)==tm&&!_tMT(_y(*p))?-3:-ixck(*p,a[1],0,a[3],a[2]==av):ixwk(*p,a[1],n==4?a[3]:au,n==4&&a[2]==av,0,kd,ix):0;P(n==4&&w_<0,w_==-1?ei0():w_==-2?el0():et0())I(w_<0,w_=0)I(!*p,*p=au)Ab8;*b=*p;MC(b+1,a+1,(n-1)*SZ(A));*p=au;
   B o;A q=w_?a[2]!=av&&kd[w_-1]==3?ixit(*b,a[1],kd,ix,(U)w_,&o):0:n>2&&a[2]!=av?({A r=d8q(p,b,a,n,kd,ix,&o);P(o,r);(A)0;}):0;   //a table's row, at the last
   //level: read first; and with more below, or in an assignment by d4 (d8q)
   I(q,I(!o,_R(q))A r=USQ(n>3?_8(a[2],A8(q,_R(a[3])),2):_8(a[2],A8(q),1));P(!r,I(*p&&*p!=au,mr(*b))E(*p=*b);0)I(*p&&*p!=au,mr(*p);*p=au)   //what f set, the assignment's result replaces
     q=ixst(*b,a[1],kd,ix,0,(U)w_,av,r,0);mr(r);*p=au;return *p=_R(N(q)))   //a row: by d8
   A u=0,r=w_?ixst(*b,a[1],kd,ix,0,(U)w_,a[2],n==4?a[3]:0,&u):n==4&&_tA(a[1])&&_n(a[1])==1?a4(*b,*_A(a[1]),a[2],a[3]):d8_(b,n);
   P(!r,I(u,I(*p&&*p!=au,mr(u))E(*p=u))0)I(*p&&*p!=au,mr(*p))*p=_R(r))// amend-by-name of a global: not from a peach worker (b.c bS)
   RU(n==3?try(x,a[1],a[2]):er(x))
   R_(d8_(a,n)))0)
//an item of a nested amend, amended at the indices below it: as . does it (d8), but a symbol list is data there, not the
//name of a global to amend (`a`b is a.b to .), so d[`b;1]:`q with d[`b]~`x`y`z amends that list; a symbol atom still
//names one. Its items are data too (ds3, ds4): symbol atoms, so an index below one is 'type, as into an atom
Z A3(dt3,/*111*/A r=_tP(x)||_tU(x)||_t(x)==ts?d8((A[]){x,y,z},3):_t(x)==tS?d3t(x,y,z):d3(x,y,z);mr(y);mr(z);r)   //d8 goes to d3 for the rest
Z A4(dt4,/*1111*/A r=_tP(x)||_tU(x)||_t(x)==ts?d8((A[]){x,y,z,u},4):_t(x)==tS?d4t(x,y,z,u):d4(x,y,z,u);mr(y);mr(z);mr(u);r)
Z __attribute__((cold)) A3(ds3,/*111*/A r=d3t(x,y,z);mr(y);mr(z);r)
Z __attribute__((cold)) A4(ds4,/*1111*/A r=d4t(x,y,z,u);mr(y);mr(z);mr(u);r)
ZN A ki(A*p,S s)_(*p=evs(s,0);P(!*p,0)PSH(cns,*p))   //a name that does not evaluate (a missing formatter): its error, not die
//k1 k2 k8: the lambda is compiled on first use under the parse lock in a peach scope, so two workers reaching
//it first do not both compile it and push it to cns at once (digest #10); outside peach plk does nothing
A k1(A*p,S s,A x)_(I(!*p,plk(1);I(!*p,ki(p,s))plk(0))P(!*p,x(0))_1(*p,x))
A k2(A*p,S s,A x,A y)_(I(!*p,plk(1);I(!*p,ki(p,s))plk(0))P(!*p,mr(y);0)_2(*p,x,y))   //x borrowed on every path, as _2 borrows it (digest #9)
A k8(A*p,S s,CO A*a,U n)_(I(!*p,plk(1);I(!*p,ki(p,s))plk(0))P(!*p,mrn(n,(A*)a);0)n?_8(*p,a,n):*p)
AA(no8,/*10..0*/en(*a))
A2(no2,/*01*/y(en0()))//amber 2.1: unused fused-verb dyad slots
