#include<stdbool.h> // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include<string.h>
#include<unistd.h>
#include"g.h"
#define  DBG(a...)//a
#define    _(a...) {return({a;});}
#define  A(x,a...) (TY(x)[]){x,a}
#define   A8(a...) (CO A[8]){a}
#define  G(x,a...) ({Z CO TY(x)arr[]={x,a};arr;})
#define  W(x,a...) while(x){a;}
#define  B(x,a...) I(x,a;break)
#define  P(x,a...) I(x,_(a))
#define  I(x,a...) if(x){a;}
#define    J(a...) else I(a)
#define    E(a...) else{a;}
#define  S(x,a...) switch(x){a}
#define  C(x,a...) case x:{a;}break;
#define    D(a...) default:{a;}break;
#define    F(a...) F_(i,a)
#define   Fj(a...) F_(j,a)
#define    X(a...) S(xt,a)
#define    Y(a...) S(yt,a)
#define X1(f,a...) A1(f,X(a)0)
#define X2(f,a...) A2(f,X(a)0)
#define Y2(f,a...) A2(f,Y(a)0)
#define  R(x,a...) case x:_(a)
#define   R_(a...) default:_(a)
#define F_(i,n,a...) for(TY(n)n_=(n),i=0;i<n_;i++){a;}
#define L(x) (SZ(x)/SZ((x)[0]))
#define R1 R
#define CO const
#define Z static
#define SZ sizeof
// amber 1.9.2: HD is both the array header size AND the allocation granularity,
// so it is what every payload pointer is aligned to. At 32 it left every vector
// buffer exactly 32 bytes PAST a cache-line boundary (measured: ptr%64==32 for
// every size), splitting a 64-byte line on the first AVX access of every array.
// 64 costs 32 bytes of header per allocation and makes every payload cache-line
// aligned. The bucket-index constant in an() (m.c) is derived from log2(HD) and
// moves with it.
#define HD 64ll//header + payload alignment (one cache line)
#define NI __attribute__((noinline))
#define ZN Z NI
#define TD typedef
#define TY __typeof__
#define ST struct
#define RES restrict
#define SW(x,y) {TY(x)t_=x;x=y;y=t_;}
#define M1(x) #x
#define M2(x) M1(x)
#define EX extern
#define Q(x) DBG(I(!(x),die(__FILE__":"M2(__LINE__)": "#x)))//assert
#define MIN(x,y) ({TY(x) x_=(x),y_=(y);x_<y_?x_:y_;})
#define MAX(x,y) ({TY(x) x_=(x),y_=(y);x_>y_?x_:y_;})
#define LH(x,y,z) ((y)-(x)<=(U)((z)-(x)))//between(low,high)
#define C09(c) LH('0',c,'9')
#define CAz(c) LH('a',(c)|32,'z')
#define CA9(c) (CAz(c)||C09(c))
#define S4(i,a,b,c,d) S(i,C(0,a)C(1,b)C(2,c)D(d))
#define Ab8 A b[8];
#define Lij L i=*xL,j=xL[1];
#define PSH(x,y) ((x)=psh(x,y))
#ifdef AMBER_ALIGNCHECK
#include<stdio.h>
static inline const void*amb_alchk(const void*p_,const char*f_,int l_){
 if(((unsigned long long)p_)&63ull){fprintf(stderr,"ALIGN %s:%d %p\n",f_,l_,p_);}
 return p_;}
#define AL(x) ((void*)amb_alchk((const void*)(x),__FILE__,__LINE__))
#else
// amber item 8. The payloads ARE 64-byte aligned -- HD is both the header size
// and the allocation granularity (see above) and mm() hands out mmap'd,
// page-aligned blocks, so every payload from an()/mb() starts on a cache line.
// That was VERIFIED, not assumed: build with -DAMBER_ALIGNCHECK (the check
// above) and run every suite -- it reports zero AL() arguments that are not
// 64-byte aligned.
//
// The assumption is nevertheless left at 32. Widening it to 64 was tried and
// measured, interleaved against the baseline: it made the byte-wise kernels
// SLOWER (bit-mask scan at 1M elements went from 0.99x to 0.85x of baseline),
// because the wider alignment assumption pushes GCC into a different peeling
// and vectorisation strategy for the 1-byte element loops. The alignment
// guarantee is real and worth keeping documented -- a future kernel that wants
// aligned 64-byte loads can rely on it -- but asserting it globally here is
// not free, and on this codebase it does not pay.
#define AL(x) __builtin_assume_aligned(x,32)
#endif
#if defined(__clang__)
#define SIMD _Pragma("clang loop vectorize(enable) interleave(enable)")
#elif defined(__GNUC__)
#define SIMD _Pragma("GCC ivdep")
#else
#define SIMD
#endif
#define CLZ   __builtin_clzll
#define CTZ   __builtin_ctzll
#define MC    __builtin_memcpy
#define MS    __builtin_memset
#define PC    __builtin_popcountll
#define SL    __builtin_strlen
#define SQ    __builtin_sqrt
// ---- release identity ----------------------------------------------------
// THE canonical version of the Amber interpreter. Everything that reports a
// version -- the REPL banner (repl.k, via `bi`), `amber --version`, and the
// WASM bindings -- reads it from here, so a release bump is a one-line change
// and the components can no longer drift apart (they did: the banner said
// v1.7 while README.md already advertised 1.9).
#define AMBER_VERSION_MAJOR 1
#define AMBER_VERSION_MINOR 9
#define AMBER_VERSION_PATCH 6
#define AMBER_VERSION M2(AMBER_VERSION_MAJOR) "." M2(AMBER_VERSION_MINOR) "." M2(AMBER_VERSION_PATCH)
#define REFB  1
#define MINE(x) (_r(x)==REFB)

// ---- scoped atomic refcounting (ray_rc_sync) -----------------------------
// A single thread-local flag flips retain/release between the fast serial path
// (plain xr++/xr--) and C11-relaxed/acq-rel atomics. peachC (src/i.c) sets it
// true only for the duration of a parallel dispatch, on every worker AND the
// parent, so ordinary single-threaded evaluation pays nothing but one
// perfectly-predicted branch -- no lock, no atomic, no memory barrier.
#if defined(__STDC_VERSION__) && __STDC_VERSION__>=201112L && !defined(__STDC_NO_THREADS__)
#define AM_TLS _Thread_local
#else
#define AM_TLS __thread
#endif
// AM_TLS_IE: thread-local with the INITIAL-EXEC model -- a single %fs-relative
// load, no __tls_get_addr() call. General-dynamic (the default in a PIE binary)
// turns every access into a function call, which is fatal on the allocator's
// hot path (bkt[] is hit by every aF/aL/aC/an). Initial-exec is valid for TLS
// defined in the main executable (which never gets dlopen'd), exactly our case.
// Fall back to plain AM_TLS on compilers without the attribute or on wasm.
// `shared` (./build.sh --shared -> libamber.so) is excluded deliberately, and
// this is a correctness requirement, not a tuning preference. Initial-exec TLS
// is only valid for storage in a module that is present at program start: it is
// resolved out of the static TLS block the loader sizes ONCE, before main().
// libamber.so is dlopen'd -- transitively, every time -- by every satellite that
// consumes it: CPython imports python-amber's extension module, which pulls
// libamber.so in behind it; the Jupyter kernel and the LSP daemon do the same
// one level further out. glibc keeps a small "surplus" static TLS reserve that
// such a library may borrow from, and Amber's own TLS (bkt[24], ray_rc_sync,
// the error buffer, the PRNG state) would usually fit inside it -- but "usually"
// here means "until some other dlopen'd library in the host process got there
// first", at which point the import fails outright with
//   dlopen: cannot allocate memory in static TLS block
// and does so nondeterministically, depending on what else the host imported.
// Global-dynamic costs one __tls_get_addr() call per access on the allocator's
// hot path, which is exactly why the native ./amber build keeps initial-exec;
// the shared build pays that call and in exchange is always loadable.
#if !defined(wasm) && !defined(shared) && (defined(__GNUC__) || defined(__clang__))
#define AM_TLS_IE AM_TLS __attribute__((tls_model("initial-exec")))
#else
#define AM_TLS_IE AM_TLS
#endif
EX AM_TLS_IE bool ray_rc_sync;
// retain one reference to a heap object's refcount word (_r(x) is a U).
#define RC_INC(x) do{if(ray_rc_sync)__atomic_fetch_add(&_r(x),1u,__ATOMIC_RELAXED);else _r(x)++;}while(0)
// atomically drop one reference, yielding the PREVIOUS count (post-decrement
// semantics, matching _r(x)-- ). A result of REFB means "I was the last owner".
#define RC_DECV(x) (ray_rc_sync?__atomic_fetch_sub(&_r(x),1u,__ATOMIC_ACQ_REL):_r(x)--)

TD void V;TD bool B;TD char G,C;TD char unsigned UC;TD CO C*S;TD short H;TD unsigned short UH;TD int I;TD unsigned int U;TD long long L;TD double F;TD size_t N;
TD unsigned long long W,A,A0(),A1(A),A2(A,A),A3(A,A,A),A4(A,A,A,A),AA(CO A*,U),AX(A,CO A*,U);

#define A0(f,b...) A f(                )_(b)
#define A1(f,b...) A f(A x             )_(b)/*1*/
#define A2(f,b...) A f(A x,A y         )_(b)/*01*/
#define A3(f,b...) A f(A x,A y,A z     )_(b)
#define A4(f,b...) A f(A x,A y,A z,A u )_(b)
#define AX(f,b...) A f(A x,CO A*a,U n  )_(DBG(Q(n<=8));b)/*0,1..1,n*/
#define AA(f,b...) A f(    CO A*a,U n  )_(DBG(Q(n<=8));b)
A1 _R,aA1,asc,AZ,blw,cB,cG,cC,cF,cH,cI,cL,cS,dsc,emaC,enl,epr,err,fir,flp,flr,frk,gZ,gg,grp,hex,imx,imn,inv,jS,js0,js1,kcos,kexp,klog,ksin,kst,las,len,m0,m1,mkn,mRa,mr,mut,
 neg,not,nul,of0,of1,opn,out,peachC,prng,qkmp,qpri,qte,raz,rev,rs0,spl,sqr,sqz,sqzZ,str,str0,til,typ,u0c,u1c,u2c,unh,unq,val,whr,wjc,mkdt,mktm,mknp,plotC,candleC,arrowExport,arrowImport,binfo,ajc,arnT,dgnT,mwC,xsC,rdlC;
A2 _1,aA2,aM,add,am,psh,ari,bin,ct,cat,cat10,cat11,dlr,dex,dot,dvd,eql,exc,crt,fil,fnd,gtn,hsh,ie,i1,ltn,mod,mnm,mtc,mul,mxm,que,sub,und,v0c,v1c,v2c;
A3 _2,aA3,arf,arp,ars,cpl,e2,r2,try;
A4 ara,a4,d4;
AX _8,e8,f8,prj,run;
AA a8,d8,ins,no8;
TD A TAU(U);TAU aA0,aA,aB,aG,aC,aF,aI,aL,aS,gns,emp;
TD A TAL(L);TAL al,az,cls,rndF;
TD A TALA(L,A);TALA drp,rnd,rsz;
TD A TAQ(S);TAQ aCz,bsl,bsm,die,sym;
A aa(U,A),ii(A,U),io(A,L),aE(L,L),af(F),aCm(S,S),aCn(S,U),apc(A,C),an(U,C),aV(C,U,CO V*),cts(A,S,U),e1f(A1,A),e2f(A2,A,A),err0(S),evs(S,B),
 k1(A*,S,A),k2(A*,S,A,A),k8(A*,S,CO A*,U),jc(C,A),jC(S,U,A),kv(A*),r2f(A2,A,A),l2f(A2,A,A),mf(U,U,U),pk(S*,C),pen(A,A1*),slc(A,U,U),unhC(S,U),wdn(A,U,U,U),
 AT(W,A),AW(C,A),AK(C,A),AO(UC,A),AN(U,A),w1(U,A,A),w2(U,A,A,A),w8(U,A,CO A*,U),*gp(A);
V cyc(V*,U,U),eS(A,U),eQ(S,U,U),exit(I),hexC(S,U,C*),kargs(I,S*),kinit(),*memmem(CO V*,N,CO V*,N),mrn(U,CO A*),mRn(U,CO A*),repl(),tilV(V*,L,L,U);
B id0(UC),mtc_(A,A),tru(A);
V par_prng_perturb(W);//decorrelate a peach worker's thread-local prng stream (r.c)
A peach_pool(A,A,U,I);//persistent thread-pool morsel-driven peach (src/peachpool.c)
C*sf(C*,L),*sl(C*,L),sup(A*,A*),tZ(L),*strchrnul(S,I);
U gi(A);
A unqL(A);//amber: O(n) integer distinct (f.c), 0 = not handled
A cntgrd(A),cntsrt(A);//amber: counting/bucket grade + counting sort (v.c), 0 = not handled
U amlb(CO L*RES,U,U,L);//branch-free lower_bound over a sorted long slice (a.c)
U amub(CO L*RES,U,U,L);//branch-free upper_bound (first >key) -- no key+1 overflow (a.c)
// AMGALLOP: how far a time-series join's merge cursor walks forward linearly
// before it gives up and binary-searches the remainder of the group slice.
// Inside a monotone run the TOTAL forward walk is bounded by the slice width
// regardless of this cap, so the cap costs nothing in the common (already
// time-ordered) case; it exists purely so one row that jumps across a huge
// slice degrades to O(log w) instead of O(w). Shared by aj (a.c) and wj (i.c).
// ---- radix-sort collation (src/v.c; src/a.c's `xs uses the same keys) -------
// Order-preserving unsigned keys: radix compares bytes as unsigned magnitudes,
// so every signed domain is folded onto the unsigned line by flipping its sign
// bit first. amkF() does the IEEE-754 double fold (see src/v.c).
#define AMKG(v) ((U)(UC)(v)^0x80u)
#define AMKH(v) ((U)(UH)(v)^0x8000u)
#define AMKI(v) ((U)(v)^0x80000000u)
#define AMKL(v) ((W)(v)^0x8000000000000000ull)
W amkF(F);
// Stable LSD radix over `nb` key bytes. Keys and row indices travel together;
// returns whichever index buffer holds the result.
I*amrdx8(W*RES,I*RES,W*RES,I*RES,N,U);
// Translate keys to a zero minimum; returns the number of key bytes that stay
// significant (0 = every key identical, i.e. nothing to sort).
U amnorm(W*RES,N,U);
A rdxg(A);   //ascending grade of a flat numeric vector, or 0 -> caller falls back
#define AMGALLOP 64u
I qA(A,A),qf(F,F),rnk(A);
U _K(A),si(S,C),_N(A),js_eval(C*,U,C*,U),fG(CO G*,U,G),fI(CO I*,U,I),fL(CO L*,U,L),us(S);
L cfm(CO A*,I),gl_(A),gl(A),iw(A,U,L),now(),pl(S*),minfZ(L,A),addfB(CO V*,U),addfZ(L,A),pf(S*);
S su(U),pID(S);
W pu(S*);
F gf(A);
EX I amdiag;//stderr-diagnostic switch (e.c); see `diag
CO C*edinfo(CO C*,I);//error-catalogue accessor for the `dgn self-test (e.c)
V eD(CO C*,U,U);//render a rich diagnostic from raw source bytes (e.c)
EX I amdiagshown;//set when a rich diagnostic was already rendered for the current error (e.c)
EX A1*v1[];EX A2*v2[];EX AA*v8[];EX A gv[4096],cns,cn[],ci[2][5];EX I pg;EX TY(CO C[])vc,TS,Tw,TR,TT,TX,Tk;EX S*argv,*env;

//                    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25
//                      () !i ,1 ,i ,i ,i ,i ,f "" ,` +m X!  5  6 .6 "c" ` {} 1+ ++ +/ +:  +  / 2:
enum                 {tA=1,tE,tB,tG,tH,tI,tL,tF,tC,tS,tM,tm,ti,tl,tf,tc,ts,to,tp,tq,tr,tu,tv,tw,tx,tdt,ttm,tnp,tn};
// tdt=date atom (days since 2000.01.01, packed int32); ttm=time atom (ms of day, packed int32);
// tnp=timestamp atom (ns since 2000.01.01, heap int64).  Native scalar temporal types.
#define T_ CO C TS[]="0""A""I""I""I""I""I""I""F""C""S""M""m""i""i""f""c""s""o""p""q""r""u""v""w""x""d""t""n",/*type symbols     */\
                Tw[]={0, 6, 6, 0, 3, 4, 5, 6, 6, 3, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5, 5, 6},/*log2(size)       */\
                TT[]={0,tA,tL,tB,tG,tH,tI,tL,tF,tC,tS,tM,tM,tI,tL,tF,tC,tS,tA,tA,tA,tA,tA,tA,tA,tA,tI,tI,tL},/*list type        */\
                TX[]={0,tG,tG,tG,tG,tH,tI,tL,tF,tG,tI,tG,tG, 0, 0,tF,tG,tI,tG,tG,tG,tG,tG,tG,tG,tG, 0, 0, 0},/*arith conformance*/\
                Tk[]="0""L""I""I""I""I""I""I""F""C""S""T""D""i""i""f""c""s""?""?""?""?""?""?""?""?""d""t""p";/*for the api (k.h)*/
#define TR(t) ((1<<tA|1<<tM|1<<tm|1<<to|1<<tp|1<<tq|1<<tr)>>(t)&1)//reftypes
#define TP(t) ((1<<ti|1<<tc|1<<ts|1<<tu|1<<tv|1<<tw|1<<tx|1<<tdt|1<<ttm)>>(t)&1)//packed types (+ date/time atoms)
#define TU(t) LH(to,t,tx)                                         //function types (to..tx exactly; temporal tags sit above)

//header bytes: b....... XXXXXXXX ....OEkt rrrrnnnn
#define _V(x) ((V*)(x))       //pointer to data
#define _n(x) (*(U *)((x)- 4))//length
#define _r(x) (*(U *)((x)- 8))//refcount
#define _T(x) (*(UC*)((x)- 9))//type(hdr)
#define _k(x) (*(UC*)((x)-10))//arity(for funcs)
#define _E(x) (*(UC*)((x)-11))//adverb(for tr)
#define _O(x) (*(UC*)((x)-12))//scroffset(for symbol lists)
#define _at(x) (*(UC*)((x)-13))//amber attribute: 0=none 1=sorted(`s)
#define _X(x) (*(A *)((x)-24))//ptr to next chunk in bucket
#define _b(x) (*(UC*)((x)-32))//bucket index

//tagged value bits (t=type,v=value,o=srcoffset,k=arity,x=ptr):
// tttttttt........................vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv tc,ti,tu,tv,tw
// tttttttt................oooooooovvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv ts
// tttttttt....kkkkxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx tx
// ................xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx00000 other(pointer)
#define _v(x) (I)(x)          //value
#define _t0(x) ((x)>>56)      //type(tag)
#define _t(x) ({A x_=(x);UC t=_t0(x_);t?t:_T(x_);})//type
#define _tU(x) TU(_t(x))      // func?
#define _tP(x) _t0(x)         // packed?
#define _tR(x) TR(_t(x))      // ref?
#define _tT(x) (_t(x)<tM)     // list?
#define _tt(x) (_t(x)>tm)     // atom?
#define _tZ(x) LH(tE,_t(x),tL)// intlist?
#define _tz(x) LH(ti,_t(x),tl)// intatom?
#define _o(x) (_ts(x)?(UC)((x)>>32):_tP(x)?0u:_O(x))//srcoffset(for symbols and symbol lists)
#define _w(x) Tw[_t(x)]       //log2(type width in bits)
#define _W(x) (1<<Tw[_t(x)]>>3)//type width in bytes
#define M_(x,a...) {DBG(A t_=)m0(x);a;DBG(x=0;m1(t_));}//two-phase free()

#define Lt(t) (W)t<<56
#define ac(v) (Lt(tc)|(U)(C)(v))
#define ai(v) (Lt(ti)|(U)(v))
#define as(v) (Lt(ts)|(U)(v))
#define adt(v) (Lt(tdt)|(U)(I)(v))//date atom (packed): days since 2000.01.01
#define atm(v) (Lt(ttm)|(U)(I)(v))//time atom (packed): ms of day
#define antp(v) AT(tnp,al(v))     //timestamp atom (heap int64): ns since 2000.01.01
#define ax(v,k) (Lt(tx)|(W)(k)<<48|(W)(v)<<16>>16)
#define V_ A1*v1[]={sam,flp,neg,fir,sqr,til,whr,rev,asc,dsc,grp,not,enl,nul,len,flr,str,unq,typ,val,u0c,u1c,u2c,las,imn,imx,out};\
           A2*v2[]={dex,add,sub,mul,dvd,exc,mnm,mxm,ltn,gtn,eql,mtc,cat,crt,hsh,und,dlr,que, _1,dot,v0c,v1c,v2c,dex,dex,dex,dex};\
           AA*v8[]={no8,no8,no8,no8,no8,no8,no8,no8,no8,no8,no8,no8,no8,no8,no8,no8,no8,ins, a8, d8,no8,no8,no8,no8,no8,no8,no8};\
         CO C vc[]={':','+','-','*','%','!','&','|','<','>','=','~',',','^','#','_','$','?','@','.','0','1','2','3','4','5','6',0};
enum         {au=Lt(tu),FLP,NEG,FIR,SQR,TIL,WHR,REV,ASC,DSC,GRP,NOT,ENL,NUL,LEN,FLR,STR,UNQ,TYP,VAL,U0C,U1C,U2C,LAS,IMN,IMX,OUT,
              av=Lt(tv),ADD,SUB,MUL,DVD,EXC,MNM,MXM,LTN,GTN,EQL,MTC,CAT,CRT,RSH,UND,DLR,QUE,AP1,DOT,V0C,V1C,V2C,V3C,V4C,MKL,GAP,
              aw=Lt(tw)};
#define NFL 0x7ff8000000000000ll
#define WFL 0x7ff0000000000000ll
#define NF (*(F*)A(NFL))
#define WF (*(F*)A(WFL))
#define NL ((L)(1ull<<63))
#define WL (~NL)
#define K(s,a...) ({Z A f;k8(&f,s,A(a),L(A(a)));})
#define K1(s,x)   ({Z A f;k1(&f,s,x);})
#define K2(s,x,y) ({Z A f;k2(&f,s,x,y);})

#define ERR M(c,compile)M(d,domain)M(i,index)M(l,length)M(n,nyi)M(o,io)M(p,parse)M(r,rank)M(s,stack)M(t,type)M(v,value)M(z,limit)
#define M(t,m) A0 e##t##0;A1 e##t;AA e##t##8;
 ERR
#undef M
// ---- binary serializer (src/ser.c): -8!x encode, -9!y decode ----
A1 ser8,des9;
#define N(x,a...) ({A r_=(x);P(!r_,a;0)r_;})//error pass-through

// ---- Apache Arrow C Data Interface (ABI-stable structs; no libarrow linkage) ----
// github.com/apache/arrow/blob/main/cpp/src/arrow/c/abi.h
struct ArrowSchema{CO C*format,*name,*metadata;L flags,n_children;struct ArrowSchema**children,*dictionary;V(*release)(struct ArrowSchema*);V*private_data;};
struct ArrowArray{L length,null_count,offset,n_buffers,n_children;CO V**buffers;struct ArrowArray**children,*dictionary;V(*release)(struct ArrowArray*);V*private_data;};

#define ov(x) ov_(#x":",(L)(x))
#define oo os("["__FILE__":"M2(__LINE__)"]");
#define nop {asm volatile("fnop");}
U os(S);W ov_(S,W);
