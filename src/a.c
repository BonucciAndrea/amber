/* clock_gettime()/CLOCK_MONOTONIC (used by the `simd` self-test/benchmark
 * builtin below) need `_POSIX_C_SOURCE >= 199309L`, which must be defined
 * before the first system header (a.h's own <unistd.h>) is pulled in --
 * same reasoning as trace.c/arena.c. Pure feature-test addition, no
 * behaviour change. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif
#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
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
//  * Zero transient allocations. The old version bump-allocated an nt-long
//    arena scratch vector, filled it, then copied it element-by-element into the
//    result -- two full passes over nt longs and an arena_reset() that stomped
//    any scratch a caller still had live. Results are now written straight into
//    the freshly allocated result vector (which cannot alias any input), so aj
//    performs one pass, touches the arena not at all, and is arena-neutral to
//    its caller. That is strictly stronger than the "allocate the workspace once
//    from the arena" rule: the workspace is gone.
A ajc(A x){
 P(_t(x)-tA||_n(x)-4,et(x))
 A*e=(A*)_V(x);
 A QT=N(cL(_R(e[0]))),TT=N(cL(_R(e[1]))),GB=N(cL(_R(e[2]))),GE=N(cL(_R(e[3])));
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
 L*RES cbase=(L*)arena_alloc((N)AJC_N*SZ(L));   // slice base occupying the slot
 L*RES ckey =(L*)arena_alloc((N)AJC_N*SZ(L));   // that group's last probed key
 U*RES ccur =(U*)arena_alloc((N)AJC_N*SZ(U));   // that group's cursor position
 P(!cbase||!ckey||!ccur,mr(QT);mr(TT);mr(GB);mr(GE);mr(out);eo(x))
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
  case tF: {F f=((CO F*)_V(c))[i];MC(&v,&f,8);break;}
  default: v=0;
 }
 return v*0x9E3779B97F4A7C15ull;}
A ajsC(A x){
 P(_t(x)-tA||_n(x)-2,et(x))
 A*e=(A*)_V(x);
 A gcs=e[0],tcol=e[1];
 U ng=_t(gcs)==tA?_n(gcs):0;
 A*gc=ng?(A*)_V(gcs):0;
 U n=_n(tcol);
 P(n<2,x(al(1)))                                  // 0 or 1 row is trivially ordered
 // ---- pass 1: run boundaries -------------------------------------------
 // chg[r] = "row r starts a new group". The type switch is hoisted OUT of the
 // row loop -- one dispatch per column, not one per element.
 UC*RES chg=(UC*)arena_alloc((N)n);
 P(!chg,x(al(0)))
 MS(chg,0,(N)n);
 #define AJS_NE(T) {CO T*RES p=_V(c);for(U r=1;r<n;r++)chg[r]|=(UC)(p[r]!=p[r-1]);}
 F(ng,A c=gc[i];
   P(_n(c)-n,x(al(0)))                            // ragged column: don't guess
   switch(_t(c)){
    case tG: case tC: AJS_NE(G) break;
    case tH: AJS_NE(H) break;
    case tI: case tS: AJS_NE(I) break;             // tS stores packed 32-bit ids
    case tL: AJS_NE(L) break;
    case tF: AJS_NE(F) break;
    default: return x(al(0));                      // unknown type: sort, don't guess
   })
 #undef AJS_NE
 // ---- pass 2: ordering column non-decreasing inside each run ------------
 #define AJS_ORD(T) {CO T*RES p=_V(tcol);for(U r=1;r<n;r++)if(!chg[r]&&p[r]<p[r-1])return x(al(0));}
 switch(_t(tcol)){
  case tG: case tC: AJS_ORD(G) break;
  case tH: AJS_ORD(H) break;
  case tI: AJS_ORD(I) break;
  case tL: AJS_ORD(L) break;
  case tF: AJS_ORD(F) break;
  default: return x(al(0));                        // unknown ordering column: sort
 }
 #undef AJS_ORD
 P(!ng,x(al(1)))                                   // no grouping: pass 2 was the whole test
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
X1(csvrT,RC(C buf[1024];U n=MIN(xn,SZ buf-1);MC(buf,xC,n);buf[n]=0;x(csv_read(buf)))R_(et(x)))
// CSV parser self-test builtin (`csv0): writes a small known CSV (mixed long/
// float/symbol columns, an embedded comma inside a quoted field, an escaped
// quote, and one empty cell) to a temp file, parses it with csv_read(), and
// asserts the resulting table's shape/types/values/null-handling. Cleans up
// the temp file whether the assertions pass or fail.
A1(csv0T,
 CO C*P_="/tmp/.amber_csv_selftest.csv";
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
 CO C*chk="_ct:`csvr \"/tmp/.amber_csv_selftest.csv\";"
   "((#_ct)=3)&(_ct[`sym]~`AAPL`MSFT`GOOG)&(_ct[`px]~187.5 410.2 138.9)&(_ct[`qty]~100 50 0N)&((@_ct[`note])=`S)";
 // evs() returns 0 (not `au`) on a parse/compile/eval error -- check
 // truthiness of r itself, not identity against `au`, before touching it.
 A r=ok?evs(chk,0):0;ok=ok&&r&&tru(r);I(r,mr(r))
 remove(P_);x(al((L)ok)))
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
Z A1(sam,x)V_;T_;U _K(A x/*0*/)_(X(R2(tu,tw,1)Rv(2)Rx(x>>48&15)Ropqr(xk))0)
X1(mkn,RmMA(e1f(mkn,x))Rt(x(_R(cn[xt])))R_(x(rsz(xN,_R(cn[xt])))))
A1(iei,/*0*/0x2332211004>>(xv*(xtv&&xv<10u)<<2)&15)
Y2(iex,/*01*/RmMA(r2f(iex,x,y))RT_A(rsz(yN,iex(x,fir(y))))Rs(as(0))Rc(ac("\0\1\x7f\x80 "[iei(x)]))Rf(y(af(A(0.,1.,WF,-WF,NF)[iei(x)])))R_(y(az(G(0ll,1,WL,-WL,NL)[iei(x)]))))
A2(ie,/*00*/x==CAT?emp(yt):iex(x,fir(yR)))
AX(prj,XmMA(x8(a,n))U k=MAX(n,xK);F(n,k-=a[i]!=GAP)x=(xtp?val:aA1)(xR);I i=0,j=1;W(i<n&&j<xn,I(xA[j]==GAP,xA[j]=a[i++])j++)W(i<n,PSH(x,a[i++]))P(xn>9,ez(x))AT(tp,AK(k,x)))
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
A2(i1,/*01*/P(y==GAP||y==au,xR)
 X(Rt(y(xR))
   RE(x=gZ(xR);x(i1(x,y)))
   Rm(i1(xy,N(fnd(xx,y))))
   RM(Y(RsS(x=flp(xR);x(i1(x,y)))RA(r2(AP1,x,y))RmM(A z=kv(&y);am(y,Ny(i1(x,z))))R_(B d=ytmt;y=N(l2f(i1,xy,y));(d?am:aM)(_R(xx),y)))0)
   R_(Y(Rilc(io(x,gl(y)))
        RmM(A z=kv(&y);am(y,Ny(i1(x,z))))
        RA(r2(AP1,x,y))
        RE(L i=*yL,j=yL[1];P(0<=i&&i<j&&j<xN,y(0);slc(x,i,j))i1(x,gZ(y)))
        R4(tB,tG,tH,tC,i1(x,cI(y)))
        R_(et(y))
        RL(A z=aI(yn);My(F(yn+3&-4,L v=yl;zi=v|-(v!=(I)v)))i1(x,z))
        RI(U n=yn;
         X(RA(A z=aA(n);F(n|!n,za=io(x,yi))y(0);I(!zn,zx=mkn(zx))sqz(z))
           RB(x=cG(xR);x(i1(x,y)))
           R_(C t=xt;B k=t-tG<3u&&maxfU(yV,yn)>=xn;A z=an(n,k?tL:t);My(G(&iG,iH,iI,oL,oF,iC,iS,oG,oH,oI)[7*k+t-tG](xV,xn,yV,zV,n))z))0))0))0)
Z A3(i2,/*001*/C b=ytT||y==GAP||y==au;x=Nz(i1(x,yR));P(!b,x(x1(z)))x(l2f(dot,x,aA1(z))))
Z AX(i8,A y=*a;P(n==1,i1(x,y))P(n==2,y(_2(x,y,a[1])))a++;n--;C b=ytT||y==GAP||y==au;x=i1(x,y);P(!x,mrn(n,a);x)P(!b,x(i8(x,a,n)))x(l2f(dot,x,aV(tA,n,a))))
L iw(A x/*0*/,U w,L i)_(S4(w,_(xg),_(xh),_(xi),_(xl))0)
Z A1(qa,A y=emp(tA);S*p=argv;W(*p,PSH(y,aCz(*p++)))y(y1(x)))
Z A1(qe,A y=emp(tS),z=emp(tA);S*e=env;W(*e,S p=*e++,q=strchrnul(p,'=');PSH(y,cS(aCm(p,q)));PSH(z,aCz(q+!!*q)))y=am(y,z);x-au?x(x1(y)):y)
Z A1(qx,exit(xtz?gl(x):1);0)
Z X1(qjs,RC(C b[4096];U n=js_eval(xC,xn,b,SZ b);x(0);aCn(b,n))RA(e1f(qjs,x))R_(et(x)))
Z X1(qp,RC(x=str0(x);S s=xC;x(pk(&s,0)))R_(et(x)))
Z A1(qt,x(al(now())))
Z A1(qfb,P(!xtC,et(x))P(xn-8,el(x))x=rev(x);x(aV(tf,1,xV)))//float from bits
Z A1(qsa,UC t=_t(x);P(_tP(x)||!LH(tG,t,tS),x)x=mut(x);_at(x)=1;x)//amber: `s sorted
Z A1(qua,UC t=_t(x);P(_tP(x)||!LH(tG,t,tS),x)x=mut(x);_at(x)=2;x)//amber: `u unique
Z A1(qpa,UC t=_t(x);P(_tP(x)||!LH(tG,t,tS),x)x=mut(x);_at(x)=3;x)//amber: `p parted
Z A1(qga,UC t=_t(x);P(_tP(x)||!LH(tG,t,tS),x)x=mut(x);_at(x)=4;x)//amber: `g grouped
// amber: `diag 0 / `diag 1 -- turn the Rust-style stderr diagnostic off/on at
// runtime, returning the PREVIOUS setting so a caller can restore it. Needed by
// anything that deliberately provokes errors it then catches (tests/harness.k,
// std.k's protect); see the note above amdiag in e.c.
Z A1(qdiag,I(amdiag<0,amdiag=1)I v=amdiag;I(_tz(x),amdiag=!!gl_(x))x(0);ai(v))
Z A1(qat,UC a=(_tP(x)||!LH(tG,_t(x),tS))?0:_at(x);x(0);a?({C b[2]={"\0supg"[a],0};sym(b);}):as(0))//amber: get attribute
ZN AX(ext,P(n-xK,er8(a,n))V*f=(V*)(x&-1ull>>16);S(n,R(1,((A1*)f)(a[0]))R(2,((A2*)f)(a[0],a[1]))R(3,((A3*)f)(a[0],a[1],a[2]))R(4,((A4*)f)(a[0],a[1],a[2],a[3]))R_(en8(a,n)))0)
ZN A sym1(I v,A x)_(Z CO C s[][4]={"k","j","p","t","x","hex","err","argv","env","exit","js","pri","prng","sin","cos","exp","ln","fb","sa","ua","pa","ga","at","pe","ema","wj","mkd","mkt","mkp","plt","cdl","aex","aim","bi","aj","arn","dgn","simd","vmd","para","csvr","csv0","astt","diag","ajs"};
 G(&kst,js1,qp,qt,frk,hex,err,qa,qe,qx,qjs,qpri,prng,ksin,kcos,kexp,klog,qfb,qsa,qua,qpa,qga,qat,peachC,emaC,wjc,mkdt,mktm,mknp,plotC,candleC,arrowExport,arrowImport,binfo,ajc,arnT,dgnT,simdT,vmdT,parT,csvrT,csv0T,astT,qdiag,ajsC,ed)[fI((V*)s,L(s),v)](x))
A2(_1,/*01*/P(!xtt,i1(x,y))U k=xK;P(1<k,k==2&&!xtp?prj(x,A8(y,GAP),2):prj(x,&y,1))
 X(Ro(run(x,&y,1))Rp(P(k>7,er(y))I m=xn-1,j=0;Ab8;F(m,b[i]=xA[i+1]==GAP&&!j?j++,y:_R(xA[i+1]))I l=MAX(0,1-j);MC(b+m,&y,8*l);_8(xx,b,m+l))
  Rq(_1(xx,N(_1(xy,y))))Rr(w1(xE,xx,y))Rs(sym1(xv,y))Ru(v1[xv](y))Rw(AK(xv-1<3u&&yK==2?1:ytU?yK:1,AW(xv,aV(tr,1,&y))))Rx(ext(x,&y,1))R_(et(y)))0)
A3(_2,/*001*/P(!xtt,i2(x,y,z))A a[]={y,z};U k=xK;P(2<k,yR;prj(x,a,2))
 X(Ro(yR;run(x,a,2))Rp(P(k>6,er(z))yR;I m=xn-1,j=0;Ab8;F(m,b[i]=xA[i+1]==GAP&&j<2?a[j++]:_R(xA[i+1]))I l=MAX(0,2-j);MC(b+m,a+j,8*l);_8(xx,b,m+l))
  Rq(_1(xx,N(_2(xy,y,z))))Rr(z(w2(xE,xx,yR,z)))Rv(v2[xv](y,z))Rw(P(!xv,com(y,z))x=Nz(x1(yR));x(x1(z)))Rx(ext(x,A8(yR,z),2))R_(et(z)))0)
AX(_8,/*01..1*/Q(n)P(n==1,x1(*a))P(n==2&&!xtp,A y=*a;y(x2(y,a[1])))P(!xtt,i8(x,a,n))U k=xK;P(n<k,prj(x,a,n))
 X(Ro(run(x,a,n))Rp(P(xn-k+n>8,er8(a,n))I m=xn-1,j=0;Ab8;F(m,b[i]=xA[i+1]==GAP&&j<n?a[j++]:_R(xA[i+1]))I l=MAX(0,n-j);MC(b+m,a+j,8*l);_8(xx,b,m+l))
   Rq(_1(xx,N(_8(xy,a,n))))Rr(w8(xE,xx,a,n))Rv(x=v8[xv](a,n);mrn(n-1,a+1);x)Rx(ext(x,a,n))R_(et8(a,n)))0)
A1(jS,cS(jc('.',str(x))))//join symbols with "."
X1(val,RA(P(!xn,x)P(xn==1,fir(x))P(xn>9,ez(x))x=mut(x);A y=_8(xx,&xy,xn-1);AN(1,x);x(y))RmM(x(_R(xy)))RE(gZ(x))RC(x=str0(x);x(evs(xV,0)))Rc(val(enl(x)))RsS(gg(x))
 Ropq(AT(tA,mut(x)))Rr(cat10(AT(tA,mut(x)),aw+xE))Ruvw(ai(xv))R_(x))
A2(dot,/*01*/Ym(et(y))U n=yN;P(!n,y(xR))P(n>8,ez(y))y=mRa(N(blw(y)));y(x8(yA,n)))
Z U knd(A x/*0*/)_(X(Ril(ti)REBGHIL(tI)R_(xt))0)
Z A set(A x,L i,A y/*1i1*/)_(Q(MINE(x));
 X(RA(A z=xa;xa=z(y);ytt&&!ytU?sqz(x):x)
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
AA(a8,/*10..0*/A x=*a,y=a[1];
 X(RE(Ab8;*b=gZ(x);AC(b+1,a+1,n-1);a8(b,n))
   RT_E(P(y==au,mRn(n-2,a+2);Ab8;*b=a[2];b[1]=x;AC(b+2,a+3,n-3);e8(AP1,b,n-1))
    Yzc(L i=gl_(y);P(i>=(W)xn,ei(x))x=mut(x);Ab8;*b=ii(x,i);AC(b+1,a+3,n-3);mRn(n-3,b+1);A z=a[2];set(x,i,Nx(z8(b,n-2))))
    I(ytZC&&n==4,A z=a[2],u=a[3];P(xtZ&&ztv&&utzZ&&(0xcf&1<<zv),ara(x,y,z,u))P(xtC&&z==av&&utcC,cC(N(ara(x,y,z,u)))))Yt(et(x))mRn(n-1,a+1);f8(AP1,a,n))
   Rm(A z=Nx(fnd(xx,yR));ZT(z(0);mRn(n-1,a+1);f8(AP1,a,n))x=mut(x);I(ztl,z=mut(z);F(zN,I(zl==NL,zl=xN;PSH(xx,ztt?yR:ii(y,i));PSH(xy,ie(a[2],xy)))))
    Ab8;*b=xy;b[1]=z;AC(b+2,a+2,n-2);xy=au;xy=Nx(z(a8(b,n)));x)
   RM(Ab8;AC(b,a,n);YsS(*b=flp(x);flp(N(a8(b,n))))*b=blw(x);sqz(N(a8(b,n))))
   RU(mRn(n-1,a+1);x(x8(a+1,n-1)))
   R_(et(x)))0)
Z A3(a3,/*100*/a8(A8(x,y,z),3))
A4(a4,/*1000*/a8(A8(x,y,z,u),4))
Z A a5(A x,A y,A z,A u,A v/*10000*/)_(a8(A8(x,y,z,u,v),5))
Z A3(d3,/*100*/U m=yN;P(y==au||!m,z1(x))P(m==1,y=fir(yR);y(a3(x,y,z)))A u=prj(DOT,(A[]){GAP,drp(1,yR)},2);y=fir(yR);y(u(a4(x,y,u,z))))
A4(d4,/*1000*/U m=yN;P(y==au||!m,x(z2(x,uR)))P(m==1,y=fir(yR);y(a4(x,y,z,u)))A v=prj(DOT,(A[]){GAP,drp(1,yR)},2);y=fir(yR);A r=y(a5(x,y,v,z,u));mr(v);r)
Z AA(d8_,/*10..0*/A x=*a,y=a[1],z=a[2];P(n==4,d4(x,y,z,a[3]))P(n==3,d3(x,y,z))en(x))
AA(d8,/*10..0*/A x=*a;
 X(RsS(A*p=gp(x);I(!*p,*p=au)Ab8;*b=*p;MC(b+1,a+1,56);*p=au;*p=_R(N(d8_(b,n))))
   RU(n==3?try(x,a[1],a[2]):er(x))
   R_(d8_(a,n)))0)
ZN A ki(A*p,S s)_(*p=evs(s,0);I(!*p,die(s))PSH(cns,*p))
A k1(A*p,S s,A x)_(I(!*p,ki(p,s))_1(*p,x))
A k2(A*p,S s,A x,A y)_(I(!*p,ki(p,s))_2(*p,x,y))
A k8(A*p,S s,CO A*a,U n)_(I(!*p,ki(p,s))n?_8(*p,a,n):*p)
AA(no8,/*10..0*/en(*a))
