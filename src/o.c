#include"a.h"
#include <stdlib.h>   // amber 2.5 (exp): malloc/calloc/qsort/free for the parallel kernels
#include"parallel.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include"arena.h"

// amber: NaN-aware float compare for `~` (match).
// IEEE gives no single bit pattern for "not a number": the `0n` LITERAL is the
// positive quiet NaN 0x7ff8..., but every NaN the FPU *computes* (0%0, 0w-0w,
// avg of an empty vector, ...) is the default NaN, which on x86-64 has the
// sign bit set (0xfff8...). A plain memcmp therefore reported
//   (0%0)~0n  ->  0b
// even though both sides print as "0n" and both answer 1b to `^` (is-null),
// so there was no way to write a working equality test against a computed
// float null. K's `~` is "match", and a null matches a null, so compare
// float payloads value-wise with all NaNs treated as one value. Only reached
// when the byte-wise fast path has already failed, so equal data still costs
// exactly one memcmp.
Z B mtcF(CO F*a,CO F*b,U n){F(n,F u=a[i],v=b[i];I(u!=v&&!(u!=u&&v!=v),return 0))return 1;}

NI B mtc_(A x,A y/*00*/)_(
 P(x==y,1)
 P(xt==yt&&((1<<ti|1<<tc|1<<ts|1<<tu|1<<tv|1<<tw|1<<tx)&1<<xt),xv==yv)
 P(xtz&&ytz,gl_(x)==gl_(y))
 XE(x=gZ(xR);x(mtc_(x,y)))
 YE(mtc_(y,x))
 P(xtZ&&ytZ&&xt-yt&&xn==yn,C t=MAX(xt,yt);x=ct(t,xR);y=ct(t,yR);x(y(!memcmp(xV,yV,((W)xn<<xw)+7>>3))))
 P(xt-yt||xtP||(xtr&&xE-yE)||xn-yn,0)
 XB(U n=((W)xn<<Tw[xt])>>3;B r=!memcmp(xV,yV,n);P(xn&7,UC m=(UC)((1u<<(xn&7))-1);r&&(xG[n]&m)==(yG[n]&m))r)   //bits after the last item are not part of the value (upstream ngn/k): take and drop leave them as they were
 P(!xtR||(LH(tB,xt,tS)&&xt==yt&&xn==yn),
   B e=!memcmp(xV,yV,((W)xn<<Tw[xt])+7>>3);
   e||!(xtf||xtF)?e:mtcF(xV,yV,xtf?1u:xn))
 F(xn|!xn,P(!mtc_(xa,ya),0))1)

A2(mtc,/*01*/y(ai(mtc_(x,y))))
Z CO W o=(-1ull>>12)-1;Z L t(L v)_(v^(W)(v>>63)>>1);Z A of_(A,I);
Z L o0(L v)_(t(v-o))Z V of0LL(CO L*a,L*r,N n){F(n+3&~3,r[i]=o0(a[i]))}A1(of0,P(xti,of0(al(xv)))/*a key in int range comes back from ari packed*/Q(xtlL);of_(x,0))
// Issue #15 (option A): -0.0 and 0.0 are one value, and so are all NaNs, wherever order is seen, so
// the order key canonicalises first: either zero is 0.0, any NaN is 0n (which sorts first). Ties then
// keep their places (the sorts are stable), and min and max give 0.0 or 0n for them.
Z L fco(L v)_(W b=(W)v<<1;!b?0:b>0xffe0000000000000ull?NFL:v)
Z L o1(L v)_(t(fco(v))+o)Z V of1LL(CO L*a,L*r,N n){F(n+3&~3,r[i]=o1(a[i]))}A1(of1,Q(xtfF);of_(x,1))
Z A of_(A x,I f)_(N n=xn;C t=xt+(tf-tl)*(1-2*f);A y=MINE(x)?AT(t,xR):an(n,t);_at(y)=0;Mx((f?of1LL:of0LL)(xV,yV,n))y)
Z I ql(L i,L j)_(i<j?-1:i>j)
I qf(F u,F v)_(ql(o1(*(L*)&u),o1(*(L*)&v)))
I qA(A x,A y/*00*/)_(I v=TS[xt]-TS[yt];P(v,v)
 X(Ril(ql(gl_(x),gl_(y)))
   Rf(qf(*xF,*yF))
   Rs(S s=su(xv);C t[8];U n=SL(s);I(n<5,s=MC(t,s,n+1))strcmp(s,su(yv)))
   RT(F(MIN(xn,yn),A z=ii(x,i),u=ii(y,i);I d=qA(z,u);mr(z(u));P(d,d))ql(xn,yn))
   Ropqr(x=str(xR);y=str(yR);I r=x&&y?qA(x,y):!!x-!!y;I(x,mr(x))I(y,mr(y));r)   //a function whose text fails (a formatter's error) sorts first
   R(tdt,ql((I)x,(I)y))R(ttm,ql((I)x,(I)y))R(tnp,ql(*(L*)_V(x),*(L*)_V(y)))   //dates and times by their value, not their words; a timestamp by its nanoseconds, not its address
   R_(ql(x,y)))0)
Z I*ascZ(CO UC*v,UC*g,I*a,I*b,I n,I w)_(U c[257];tilV(a,0,n,2);Fj(w,MS(c,0,SZ c);F(n,g[i]=v[w*a[i]+j])F(n,c[g[i]+1]++)I(c[1+*g]-n,F(255,c[i+1]+=c[i])F(n,b[c[g[i]]++]=a[i])SW(b,a)))a)
Z A grdm(A x/*1*/,A1 f)_(A y=kv(&x);x(x1(Nx(f(y)))))

Z V mrg(A x/*0*/,I*p,I*q,I*b,I*d,I k){I*r=p-q+b;W(1,I(qA(xA[*p],xA[*b])<k,*r++=*p++;P(p==q))E(*r++=*b++;B(b==d)))MC(r,p,q-p<<2);}//merge(k=1),mergeR(k=0)
Z V cis(A x/*0*/,I*p,N n,I*r){F(n,I j=0,k=i,v=p[i];A y=xA[v];W(j<k,I m=j+k>>1;I(qA(y,xA[r[m]])<0,k=m)E(j=m+1))memmove(r+j+1,r+j,i-j<<2);r[j]=v)}//copying_insertionsort
Z V cms(A x/*0*/,I*p,N n,I*r){P(n<17,cis(x,p,n,r);)N m=n/2;cms(x,p+m,n-m,r+m);cms(x,p,m,p+m);mrg(x,p+m,p+2*m,r+m,r+n,1);}//copying_mergesort
// A generic list of dates, times or timestamps (with ints or floats among them, such as a 0N for a missing one)
// is graded by keys, in qA's order: by kind (d f i n t, as TS spells them), then by value, each kind's keys
// through the stable counting or radix grade, where ascA's merge sort would call qA (and read a timestamp's
// boxed nanoseconds) for every comparison. kK reads the items once: their keys (*k) and kinds (*c), and the
// kinds there as bits; 0 when an item is of another kind or none is temporal (with nothing allocated when
// that shows before the first temporal item).
Z I kT(UC t)_(t==tdt?0:t==tf?1:t==ti||t==tl?2:t==tnp?3:t==ttm?4:-1)
Z L kV(A y,UC t)_(t==tnp?*(L*)_V(y):t==tdt||t==ttm?(L)(I)y:t==tf?o1(*(L*)_V(y)):gl_(y))
Z A kG(A k)_(A g=cntgrd(k);I(!g,g=rdxg(k))mr(k);g)
Z U kK(A x,A*k,A*c){N n=xn;CO A*a=xA;U m=0;N p=0;I h=0;W(p<n&&(h=kT(_t(a[p])))>=0&&!(1u<<h&25u),p++)P(p==n||h<0,0)A K=aL((U)n),C_=aG((U)n);L*RES v=_L(K);UC*RES g=_V(C_);
 F(n,A y=a[i];UC t=_t(y);I j=kT(t);I(j<0,mr(K);mr(C_);return 0;)g[i]=(UC)j;m|=1u<<j;v[i]=kV(y,t))
 *k=K;*c=C_;return m;}
Z A kGr(A k,A c,U m){P(!(m&m-1),kG(_R(k)))N n=_n(k);CO L*RES v=_L(k);CO UC*RES g=_V(c);U s[5]={0},o[5],t=0;F(n,s[g[i]]++)F(5,o[i]=t;t+=s[i])
 A K=aL((U)n),p=aI((U)n),z=aI((U)n);L*RES w=_L(K);I*RES q=_I(p);
 F(n,U j=o[g[i]]++;w[j]=v[i];q[j]=(I)i)
 F(5,U b=o[i]-s[i];I(s[i],A h=kG(aV(tL,s[i],w+b));P(!h,mr(K);mr(p);mr(z);0)Fj(s[i],zI[b+j]=q[b+_I(h)[j]])mr(h)))
 mr(K);mr(p);return z;}
// and for xasc (a.c xsC): such a list's keys, *c its kinds when it holds more than one (else 0), and *s the
// kinds there as bits (d f i n t); 0 for any other list
A kys(A x,A*c,U*s){A k,g;U m=_t(x)==tA&&xn?kK(x,&k,&g):0;P(!m,0)*s=m;I(m&m-1,*c=g)E(*c=0;mr(g))return k;}
Z A ascT(A x){A k,c;U m=kK(x,&k,&c);P(!m,0)A z=kGr(k,c,m);mr(k);mr(c);return z;}
// For = and ?: in grade order, the items equal to one another (~) are a run of one kind and one key, which is
// found from the keys without reading the items again; each item is named by its group's first index (the
// run's first, as the grade is stable): grpI groups those names (in order of first appearance, indices
// ascending) and the first indices are the distinct items, as the K paths for generic lists find them.
Z A frT(A x){A k,c;U m=kK(x,&k,&c);P(!m,0)A g=kGr(k,c,m);P(!g,mr(k);mr(c);0)N n=xn;CO I*RES p=_I(g);CO L*RES v=_L(k);CO UC*RES t=_V(c);
 A u=aI((U)n);I*RES r=_I(u);I f=p[0];r[f]=f;for(N j=1;j<n;j++){I i=p[j],h=p[j-1];I(v[i]!=v[h]||t[i]!=t[h],f=i)r[i]=f;}
 mr(g);mr(k);mr(c);return u;}
A1(ascA,N n=xn;A z=aI(n);I*p=zI;tilV(p,0,n,2);P(n<17,cis(x,p,n,p);x(z))N m=n/2;A y=aI(n-m);I*t=yI;cms(x,p+m,n-m,t);cms(x,p,m,p+n-m);mrg(x,t,t+n-m,p+n-m,p+n,0);x(y(z)))
// The pre-batch-2 grade, kept verbatim as the fallback for every case rdxg()
// declines (an exotic width, or an arena that could not supply scratch): the
// subtract-the-minimum normalisation plus ascZ()'s byte-wise radix, which
// re-gathers the value array through the permutation on every one of its
// passes. Floats reach it only via of1(), i.e. through a fully materialised
// order-preserving copy of the vector -- exactly the two costs rdxg() removes.
Z A1(ascB,P(xtF,asc(of1(x)))
 x=N(K1("{x-&/x}",x));N n=xn;A y=aC(n),z=aI(n),u=aI(n);Mx(My(u=ascZ(xV,yV,zV,uV,n,(1ll<<xw)+7>>3)==zV?u(z):z(u)))u)
X1(asc,Rt(opn(x))Rm(grdm(x,asc))RM(K1("{(!#x){x@<y x}/|.+x}",x))RS(asc(str(x)))RA(P(xn-(I)xn,ez(x))A g=xn>1?ascT(x):0;g?x(g):ascA(x))RE(Lij x(0);aE(0,j-i))
 RGC(P(xn-(I)xn,ez(x))N n=xn;I c[257]={};B u=xtC;I*b=c+(u?1:129),*d=c+(u?0:128);F(n,b[u?(UC)xg:xg]++)F(256,c[i+1]+=c[i])A y=aI(n);Mx(F(n,yI[d[u?(UC)xg:xg]++]=i))ct(tZ(n-1),y))//chars sort as unsigned bytes
 // amber batch 2: 16/32/64-bit integers and IEEE-754 doubles go through the
 // key-carrying LSD radix in src/v.c -- one sequential pass per SIGNIFICANT key
 // byte, constant byte columns skipped, already-ordered input recognised in the
 // single extraction pass and answered with the identity permutation.
 // amber 2.3: an `s vector grades to the identity (the grade is stable), so it
 // is written directly instead of being rediscovered by the key pass.
 R4(tH,tI,tL,tF,P(xn-(I)xn,ez(x))N n=xn;
  I(_at(x)==1,A y=aI((U)n);I*RES o=yI;for(N i=0;i<n;i++)o[i]=(I)i;return x(ct(tZ(n-1),y));)
  A y=cntgrd(x);I(!y,y=rdxg(x))P(!y,ascB(x))x(ct(tZ(n-1),y)))
 R_(P(xn-(I)xn,ez(x))ascB(x)))
X1(dsc,RMT(x=rev(asc(rev(x)));sub(ai(xN-1),x))Rm(grdm(x,dsc))Ril(cls(gl(x)))R_(et(x)))
// amber: O(n) direct-indexed group for a 32-bit int vector.  This is the hot
// case: SYMBOLS reach it through cSI (tS is stored as interned 4-byte ids), so
// every `select ... by sym` lands here, as do the group_* benchmarks whose keys
// are `k!H` -- dense small non-negative integers.
//
// The path this replaces graded the vector (`<x`) and cut it at the boundaries:
// a full O(n log n) sort to answer a question that only needs equal elements
// collected.  tG and tH already did the counting version (see RGC/RH below);
// this is the same idea with the table sized at run time instead of 256/65536.
//
// Two passes, both sequential: the first counts occurrences and records each
// key the FIRST time it is seen, which is exactly the first-appearance key
// order the old result had -- the partition and the key order are byte-identical,
// which is the invariant the 2.0.0 id-based change was careful to preserve.
// The second scatters row indices into the per-group vectors.
//
// Returns 0 (not an error) when the value range is too wide to index, and the
// caller falls back to the sort path: a sparse sweep over a huge table is worse
// than sorting.  Workspace comes from the HFT scratch arena, as in src/v.c.
Z A grpI(A x){
 N n=xn;CO I*RES v=(CO I*)_V(x);
 I mn=v[0],mx=v[0];
 F(n,I t=v[i];I(t<mn,mn=t)I(t>mx,mx=t))
 L rng=(L)mx-(L)mn+1;
 // cap the table both absolutely and relative to the data: 4M slots, and no
 // more than 8 slots per row, so a sparse key space cannot blow up the sweep.
 P(rng>((L)1<<22)||rng>8*(L)n+1024,0)
 ArenaMark mk=arena_mark();
 U*RES cnt=(U*)arena_alloc((size_t)rng*SZ(U));P(!cnt,arena_release(mk);0)
 U*RES gid=(U*)arena_alloc((size_t)rng*SZ(U));P(!gid,arena_release(mk);0)
 L md=(L)n<rng?(L)n:rng;
 I*RES fst=(I*)arena_alloc((size_t)md*SZ(I));P(!fst,arena_release(mk);0)
 MS(cnt,0,(size_t)rng*SZ(U));
 U nb=0;
 F(n,L k=(L)v[i]-mn;I(!cnt[k]++,gid[k]=nb;fst[nb]=v[i];nb++))
 A z=aA(nb);P(!z,arena_release(mk);0)
 F(nb,_A(z)[i]=aI(cnt[(L)fst[i]-mn]))
 U*RES fil=(U*)arena_alloc((size_t)nb*SZ(U));P(!fil,arena_release(mk);mr(z);0)
 MS(fil,0,(size_t)nb*SZ(U));
 F(n,L k=(L)v[i]-mn;U s=gid[k];_I(_A(z)[s])[fil[s]++]=i)
 A ky=aV(tI,nb,fst);          // copies out of the arena before it is released
 arena_release(mk);
 return am(ky,z);
}
// amber 2.3: float KEYS for = and ?: -0.0 folded onto 0.0 and every NaN onto one key
// (one value each, as find and ~ take them); every other double kept bit for bit.
// A copy; the caller still emits the original doubles.
Z A fcanon(A x)_(U n=xn;A y=aF(n);CO W*RES p=(CO W*)xV;W*RES q=(W*)yV;F(n,W v=p[i];q[i]=v==0x8000000000000000ull?0:v<<1>0xffe0000000000000ull?0x7ff8000000000000ull:v)y)   //one key for both zeros, and one for every NaN, as find matches them
// = and ? of a generic list of dates, times or timestamps, through frT; 0 for any other list
Z A grpT(A x){A u=frT(x);P(!u,0)A d=grpI(u);mr(u);P(!d,0)A v=kv(&d);return am(i1(x,d),v);}
Z A unqT(A x){A u=frT(x);P(!u,0)N m=0;I*RES r=_I(u);F(xn,I(r[i]==(I)i,r[m++]=(I)i))A j=aV(tI,(U)m,r);mr(u);return i1(x,j);}
Z A cSI(A);// amber 2.0.0: symbol<->int-id reinterpret (defined just below), used by grp's tS fast path
X1(grp,Ril(K1("=/:/2#,!:",x))Rm(A y=kv(&x);y=Nx(grp(y));yy=x(i1(x,yy));y)R_(et(x))
 // amber 2.0.0: group a SYMBOL vector by its interned 4-byte id (tS is stored as
 // tI-width ids; equal symbols -> equal ids) instead of the general path, whose
 // `<x` grade lexically string-sorts symbols (o.c asc's RS(asc(str(x)))) at
 // O(n log n) with per-char compares. Grouping only needs equal elements
 // adjacent, so the id order is fine, and the partition + first-appearance key
 // order are byte-identical to the old result.  Measured on 1M rows / 100 groups:
 // ~670 ms -> ~15 ms.  cSI flips tS<->tI on the same payload; we group the ids,
 // then flip the dict's int keys back to symbols.
 RS(P(!xn,K1("{x!0#,!0}",x))A r=grp(cSI(x));A v=kv(&r);am(cSI(r),v))
 // amber item 7, REVERTED after measurement. The "optimisation" here was to
 // hoist the group payload pointers into an rp[256] array before the scatter,
 // on the theory that `_I(r[v])` was a dependent load. It is not: A is an
 // integer HANDLE and _I() is pointer arithmetic on it, so the original form
 // has no load to hoist, while rp[] adds a real one. Measured at 10M rows,
 // interleaved base/new x3: 87-90 ms before, 106-109 ms after -- a 21%
 // REGRESSION. Restored verbatim; the lesson is recorded rather than the code.
 RGC(A r[  256]={};UC b[  256];U nb=0;U c[  256]={};F(xn,UC v=xg;I(!c[v]++,b[nb++]=v))A z=aA(nb);F(nb,za=r[b[i]]=aI(c[b[i]]))I(!nb,*zA=emp(tG))MS(c,0,SZ c);F(xn,UC v=xg;_I(r[v])[c[v]++]=i)x(am(aV(xt,nb,b),z)))
 RH( A r[65536]={};UH b[65536];U nb=0;U c[65536]={};F(xn,UH v=xh;I(!c[v]++,b[nb++]=v))A z=aA(nb);F(nb,za=r[b[i]]=aI(c[b[i]]))I(!nb,*zA=emp(tG))MS(c,0,SZ c);F(xn,UH v=xh;_I(r[v])[c[v]++]=i)x(am(aV(xt,nb,b),z)))
 RI(P(!xn,K1("{x!0#,!0}",x))
  {A g_=grpI(x);P(g_,x(g_))}   /* O(n) counting group; 0 = range too wide, sort instead */
  K1("{$[x;x[*'g]!g@:<g:(&~(~*s)=':s:x i)_i:<x;x!0#,!0]}",x))
 RF(P(!xn,K1("{x!0#,!0}",x))K2("{x[*'g]!g@:<g:(&1,~(1_s)=(-1)_s:y i)_i:<y}",x,fcanon(x)))   //floats: 2.3.0's canonical keys (first spelling, indices in order)
 R(tA,I(xn>1,A z=grpT(x);P(z,x(z)))K1("{$[#x;{b:~x~':x i:<x;i:i@<i+(#x)*-1++\\b;g:(&b)_i;g:g@<g;x[*'g]!g}x;x!0#,!0]}",x))   //generic lists (which may hold floats): ~ joins -0.0 with 0.0 (and NaNs), which grade keeps apart, so the indices are sorted within each run of matching items (one grade, by run then index)
 R3(tE,tL,tM,K1("{$[#x;x[*'g]!g@:<g:(&~x~':x i)_i:<x;x!0#,!0]}",x)))
Z A1(cSI,Q(xtS||xtI)C t=tS^tI^xt;MINE(x)?(_at(x)=0,AT(t,x)):x(aV(t,xn,xV)))
X1(unq,RM(K1("{$[#x;x@i@<i:&/'.=+.+x;x]}",x))   /*a table: its distinct rows, first seen first (issue #19); as q*/Rm(unq(val(x)))RE(x)RS(cSI(unq(cSI(x))))Ril(rndF(gl(x)))R_(et(x))RB(unq(cG(x)))
 RGC(C a[256]={},r[256],t=xt;U n=0;Mx(F(xn,UC v=xg;I(!a[v],a[v]=1;r[n++]=v)))aV(t,n,r))
 R5(tA,tH,tI,tL,tF,P(xn<2,x)
  {A u_=xtA?unqT(x):unqL(x);P(u_,x(u_))}     /*amber: C hash/LUT distinct (or, for a generic list, unqT), 0 = not handled*/
  P(xn<<xw-3<pg&&!xtA,K1("{x@&(x?x)=!#x}",x))
  P(xtF,K2("{x@i@<i@:&@[;0;:;1]@~=':y@i:<y}",x,fcanon(x)))   //long float vectors: 2.3.0's canonical keys, which keep the first spelling
  K1("{b:@[;0;:;1]@~~':x@i:<x;o:(#x)*-1++\\b;j:(|&\\|i+o)[w]-o w:&b;x@j@<j}",x)))   //each run of matching items keeps its least index (not the first in grade order: -0.0 and 0.0 match); a min-scan from the right, each run offset by n times its number

// ---- amber 2.1: `gagg (op;k;v[;m]) -- fused group aggregate -----------------
// One pass: acc[group(k[i])] op= v[i]. op is a symbol (`sum `count `min `max
// `avg `first `last) or the matching small int. Groups are numbered in FIRST
// APPEARANCE order, exactly the key order `=` produces, so a caller that
// already has `=k` sees the same rows. Keys tG/tH/tI/tL/tS (ids); values any
// numeric vector (count ignores v; first/last accept any vector). The optional
// m is a 0/1 byte mask: masked-out rows are skipped, so `where` never has to
// build an index vector or gather the columns. Returns
//     (keys ; aggregates ; first row index per group)
// or () when the shape is not handled (the K wrapper then takes the generic
// path). Small key ranges (<= 4M and <= 8 slots per row) use a direct table;
// wider ranges an open-addressed hash that grows with the distinct count.
V free(V*);V*malloc(N);V*realloc(V*,N);
#define GOLD 0x9E3779B97F4A7C15ull
enum{GA_SUM,GA_CNT,GA_MIN,GA_MAX,GA_AVG,GA_FST,GA_LST};
#define GARD(w,p,i) ((w)==0?(L)((CO G*)(p))[i]:(w)==1?(L)((CO H*)(p))[i]:(w)==2?(L)((CO I*)(p))[i]:((CO L*)(p))[i])
#define GA_GROW() I(ng==cap,U nc=cap*2;gk=realloc(gk,(N)nc*SZ(L));gf=realloc(gf,(N)nc*SZ(I));af=realloc(af,(N)nc*SZ(F));al_=realloc(al_,(N)nc*SZ(L));gc=realloc(gc,(N)nc*SZ(L));cap=nc;)
#define GA_ADD(g,i) {af[g]=0;al_[g]=0;gc[g]=0;gf[g]=(I)(i);I(code==GA_MIN,af[g]=WF;al_[g]=vf?gmn:WL)I(code==GA_MAX,af[g]=-WF;al_[g]=vf?gmx:NL+1)}
// amber 2.3: float min/max compare o1() keys, the order &/ and |/ use (NaN lowest, -0.0
// below 0.0), and give back the winner's own bits. The plain < and > skipped NaNs, so a
// group min ignored the NaN &/ returns, and an all-NaN group came out as 0w/-0w.
#define GA_OF(i) o1(((CO L*)vp)[i])
// ---- amber 2.5 (exp): parallel gagg for the exactly-combinable ops, see patch notes / gaggC
#define PGAG_MIN (1u<<16)
#define PGAG_CELLS (1u<<22)        //threads x key range: the private tables' total size cap
TD struct{CO V*kp,*vp;CO UC*mp;U wk,wv,nt;N n;I code;B vf;L lo;W rg;L*cnt,*acc;I*fst,*lst;}GQ;
Z V gq1(V*c_,int t){GQ*c=c_;N s=c->n*t/c->nt,e=c->n*(t+1)/c->nt;W rg=c->rg;L*cnt=c->cnt+(N)t*rg,*acc=c->acc+(N)t*rg;
 I*fst=c->fst+(N)t*rg,*lst=c->lst+(N)t*rg;CO V*vp=c->vp;U wv=c->wv;B vf=c->vf;I code=c->code;L lo=c->lo;
 for(N i=s;i<e;i++){if(c->mp&&!c->mp[i])continue;W q=(W)GARD(c->wk,c->kp,i)-(W)lo;
  if(!cnt[q])fst[q]=(I)i;lst[q]=(I)i;cnt[q]++;
  S(code,
   C(GA_SUM,L d=GARD(wv,vp,i);I(d!=NL,acc[q]=(L)((W)acc[q]+(W)d)))
   C(GA_MIN,I(vf,F d=((CO F*)vp)[i];I(d==d,L t_=GA_OF(i);I(t_<acc[q],acc[q]=t_)))E(L t_=GARD(wv,vp,i);I(t_!=NL&&t_<acc[q],acc[q]=t_)))
   C(GA_MAX,I(vf,F d=((CO F*)vp)[i];I(d==d,L t_=GA_OF(i);I(t_>acc[q],acc[q]=t_)))E(L t_=GARD(wv,vp,i);I(t_!=NL&&t_>acc[q],acc[q]=t_)))
   D())}}
Z I gq_cmp(CO V*a,CO V*b){I x_=*(CO I*)a,y_=*(CO I*)b;return(x_>y_)-(x_<y_);}
// fills the group tables as the serial loop would; -1: not taken (wrong op, range too wide, or no memory)
NI Z I gagg_par(I code,CO V*kp,U wk,CO V*vp,U wv,B vf,CO UC*mp,N n,L lo,W rg,B direct,L gmn,L gmx,int nt,
 L**pgk,I**pgf,F**paf,L**pal,L**pgc,U*pcap,U*png){
 P(!(code==GA_CNT||code==GA_SUM&&!vf||code==GA_MIN||code==GA_MAX||code==GA_FST||code==GA_LST),-1)
 P(!direct||(W)nt*rg>PGAG_CELLS,-1)
 N cells=(N)nt*(N)rg;GQ c={.kp=kp,.vp=vp,.mp=mp,.wk=wk,.wv=wv,.nt=(U)nt,.n=n,.code=code,.vf=vf,.lo=lo,.rg=rg};
 c.cnt=calloc(cells,SZ(L));c.acc=malloc(cells*SZ(L));c.fst=malloc(cells*SZ(I));c.lst=malloc(cells*SZ(I));
 I r=-1;if(!c.cnt||!c.acc||!c.fst||!c.lst)goto out;
 {L a0=code==GA_MIN?(vf?gmn:WL):code==GA_MAX?(vf?gmx:NL+1):0;F(cells,c.acc[i]=a0)}
 par_run(nt,gq1,&c);
 // combine per slot in thread order, then order the groups by first row
 {U ng=0;I*ord=malloc((N)rg*SZ(I)*2);if(!ord)goto out;I*first=ord+rg;   //ord: (first row, slot) pairs packed as two Is
  TD struct{I f,s;}FS;FS*fs=(FS*)ord;
  for(W q=0;q<rg;q++){I fr=-1;for(int w=0;w<nt;w++)if(c.cnt[(N)w*rg+q]){fr=c.fst[(N)w*rg+q];break;}if(fr>=0){fs[ng].f=fr;fs[ng].s=(I)q;ng++;}}
  qsort(fs,ng,SZ(FS),gq_cmp);   //by first row (unique per slot), so the serial first-appearance order
  U cap=*pcap;if(ng>cap){L*gk=realloc(*pgk,ng*SZ(L));I*gf=realloc(*pgf,ng*SZ(I));F*af=realloc(*paf,ng*SZ(F));L*al_=realloc(*pal,ng*SZ(L));L*gc=realloc(*pgc,ng*SZ(L));
   if(gk)*pgk=gk;if(gf)*pgf=gf;if(af)*paf=af;if(al_)*pal=al_;if(gc)*pgc=gc;if(!gk||!gf||!af||!al_||!gc){free(ord);goto out;}*pcap=ng;}
  L*gk=*pgk;I*gf=*pgf;F*af=*paf;L*al_=*pal;L*gc=*pgc;
  for(U g=0;g<ng;g++){W q=(W)fs[g].s;gk[g]=lo+(L)q;af[g]=0;L cn=0,ac=code==GA_MIN?(vf?gmn:WL):code==GA_MAX?(vf?gmx:NL+1):0;I fr=-1,ls=-1;
   for(int w=0;w<nt;w++){N j=(N)w*rg+q;if(!c.cnt[j])continue;cn+=c.cnt[j];I(fr<0,fr=c.fst[j])ls=c.lst[j];
    S(code,C(GA_SUM,ac=(L)((W)ac+(W)c.acc[j]))C(GA_MIN,I(c.acc[j]<ac,ac=c.acc[j]))C(GA_MAX,I(c.acc[j]>ac,ac=c.acc[j]))D())}
   gc[g]=cn;al_[g]=ac;gf[g]=code==GA_LST?ls:fr;I(code==GA_MIN,af[g]=WF)I(code==GA_MAX,af[g]=-WF)}
  *png=ng;free(ord);r=0;}
 out:
 free(c.cnt);free(c.acc);free(c.fst);free(c.lst);
 return r;}
A1(gaggC,P(_t(x)-tA||(_n(x)-3&&_n(x)-4),et(x))A*e=_A(x);A op=e[0],k=e[1],v=e[2],m=_n(x)==4?e[3]:0;
 I code=-1;
 I(_ts(op),S nm=su(_v(op));code=!strcmp(nm,"sum")?GA_SUM:!strcmp(nm,"count")?GA_CNT:!strcmp(nm,"min")?GA_MIN:!strcmp(nm,"max")?GA_MAX:!strcmp(nm,"avg")?GA_AVG:!strcmp(nm,"first")?GA_FST:!strcmp(nm,"last")?GA_LST:-1)
 J(_tz(op),code=(I)gl_(op))
 P(code<0||code>GA_LST,x(emp(tA)))
 UC kt=_t(k);P(_tP(k)||!(kt==tG||kt==tH||kt==tI||kt==tL||kt==tS),x(emp(tA)))
 N n=_n(k);U wk=kt==tG?0:kt==tH?1:(kt==tI||kt==tS)?2:3;
 UC vt=code==GA_CNT?tL:_t(v);B vf=vt==tF;L gmn,gmx;{F w=WF;L b;MC(&b,&w,SZ b);gmn=o1(b);w=-WF;MC(&b,&w,SZ b);gmx=o1(b);}/*min and max start from the keys of 0w and -0w (or 0W and -0W): an all-null group gives them, as q*/U wv=vt==tG?0:vt==tH?1:vt==tI?2:3;
 I(code!=GA_CNT&&code!=GA_FST&&code!=GA_LST,P(_tP(v)||!(vt==tG||vt==tH||vt==tI||vt==tL||vt==tF)||_n(v)!=n,x(emp(tA))))
 I(code==GA_FST||code==GA_LST,P(_tP(v)||_N(v)!=n,x(emp(tA))))
 A mb_=0;I(m&&!_tP(m)&&_t(m)==tB,mb_=m=cG(_R(m)))   // a bit mask is widened to bytes
 I(m,P(_tP(m)||_t(m)!=tG||_n(m)!=n,I(mb_,mr(mb_))x(emp(tA))))
 CO V*kp=_V(k),*vp=code==GA_CNT?0:_V(v);CO UC*mp=m?_V(m):0;
 I(!n,A ky=an(0,kt),vl=an(0,vf||code==GA_AVG?tF:tL);I(mb_,mr(mb_))return x(aV(tA,3,A(ky,vl,aI(0))));)
 // ---- key range
 L lo=GARD(wk,kp,0),hi=lo;F(n,L t=GARD(wk,kp,i);I(t<lo,lo=t)I(t>hi,hi=t))
 W rg=(W)hi-(W)lo+1;B direct=rg&&rg<=((W)1<<22)&&rg<=8*(W)n+1024;
 // ---- group tables (grow on demand): key, first row, accumulators
 U cap=direct?(U)MIN((W)n,rg):1024,ng=0;
 L*RES gk=malloc((N)cap*SZ(L));I*RES gf=malloc((N)cap*SZ(I));F*RES af=malloc((N)cap*SZ(F));L*RES al_=malloc((N)cap*SZ(L));L*RES gc=malloc((N)cap*SZ(L));B fail=0;
 I*RES slot=0;W*RES ht=0;W hcap=0;U hlg=0;
 I(direct,slot=malloc((N)rg*SZ(I));I(slot,MS(slot,0xff,(N)rg*SZ(I))))
 E(hcap=2048;hlg=11;ht=malloc((N)hcap*SZ(W));I(ht,MS(ht,0,(N)hcap*SZ(W))))
 P(!gk||!gf||!af||!al_||!gc||(direct?!slot:!ht),free(gk);free(gf);free(af);free(al_);free(gc);free(slot);free(ht);I(mb_,mr(mb_))x(emp(tA)))
 for(N i=0;i<n;i++){
  I(mp&&!mp[i],continue)
  L key=GARD(wk,kp,i);I g;
  I(direct,W sidx=(W)key-(W)lo;g=slot[sidx];I(g<0,GA_GROW();g=(I)ng++;slot[sidx]=g;gk[g]=key;GA_ADD(g,i)))
  E(I((W)ng*2>=hcap,W nc=hcap<<1;U nl=hlg+1;W*nt=malloc((N)nc*SZ(W));I(!nt,fail=1;break)MS(nt,0,(N)nc*SZ(W));
      F(hcap,I(ht[i],W gi=ht[i]-1;W j=((W)gk[gi]*GOLD)>>(64-nl);W(nt[j],j=(j+1)&(nc-1))nt[j]=ht[i]))free(ht);ht=nt;hcap=nc;hlg=nl;)
    W j=((W)key*GOLD)>>(64-hlg),msk=hcap-1;
    W(ht[j]&&gk[ht[j]-1]!=key,j=(j+1)&msk)
    I(ht[j],g=(I)(ht[j]-1))E(GA_GROW();g=(I)ng++;ht[j]=(W)g+1;gk[g]=key;GA_ADD(g,i)))
  gc[g]++;
  S(code,
   // issue #14: sum avg min max are q-named, so they skip nulls, as q's do (avg counts the rest in al_)
   C(GA_SUM,I(vf,F d=((CO F*)vp)[i];I(d==d,af[g]+=d))E(L d=GARD(wv,vp,i);I(d!=NL,al_[g]+=d)))
   C(GA_AVG,I(vf,F d=((CO F*)vp)[i];I(d==d,af[g]+=d;al_[g]++))E(L d=GARD(wv,vp,i);I(d!=NL,af[g]+=(F)d;al_[g]++)))
   C(GA_MIN,I(vf,F d=((CO F*)vp)[i];I(d==d,L t=GA_OF(i);I(t<al_[g],al_[g]=t)))E(L t=GARD(wv,vp,i);I(t!=NL&&t<al_[g],al_[g]=t)))
   C(GA_MAX,I(vf,F d=((CO F*)vp)[i];I(d==d,L t=GA_OF(i);I(t>al_[g],al_[g]=t)))E(L t=GARD(wv,vp,i);I(t!=NL&&t>al_[g],al_[g]=t)))
   C(GA_LST,gf[g]=(I)i)
   D())}
 P(fail,free(gk);free(gf);free(af);free(al_);free(gc);free(slot);free(ht);I(mb_,mr(mb_))x(emp(tA)))
 // ---- results
 A ky=an(ng,kt);S4(wk,F(ng,_G(ky)[i]=(G)gk[i]),F(ng,_H(ky)[i]=(H)gk[i]),F(ng,_I(ky)[i]=(I)gk[i]),F(ng,_L(ky)[i]=gk[i]))
 A fr=aV(tI,ng,gf);A vl;
 S(code,
  C(GA_SUM,I(vf,vl=aV(tF,ng,af))E(vl=aV(tL,ng,al_)))
  C(GA_AVG,vl=an(ng,tF);F(ng,_F(vl)[i]=af[i]/(F)al_[i]))   //over the non-null items; none: 0n
  C(GA_MIN,I(vf,F(ng,al_[i]=o0(al_[i]))vl=aV(tF,ng,al_))E(vl=aV(tL,ng,al_)))
  C(GA_MAX,I(vf,F(ng,al_[i]=o0(al_[i]))vl=aV(tF,ng,al_))E(vl=aV(tL,ng,al_)))
  C(GA_CNT,vl=aV(tL,ng,gc))
  D(vl=i1(v,_R(fr))))
 free(gk);free(gf);free(af);free(al_);free(gc);free(slot);free(ht);I(mb_,mr(mb_))
 P(!vl,mr(ky);mr(fr);x(0))
 x(aV(tA,3,A(ky,vl,fr))))
#undef GA_GROW
#undef GA_ADD
#undef GA_OF
// ---- amber 2.5 (exp): `gagg comes here first. The parallel aggregation (gagg_par) has its own entry point rather
// than a branch in gaggC: inside gaggC it made GCC compile the serial loop 3-10% slower even where it is never taken.
// gaggT takes what gagg_par does exactly -- count, int sum, min, max, first, last over a direct key range, n at least
// PGAG_MIN, more than one thread, no bit mask -- checked as gaggC checks it, and builds the result as gaggC does;
// everything else, and anything gagg_par declines, goes to gaggC as it always went.
A gaggT(A x){
 P(_t(x)-tA||(_n(x)-3&&_n(x)-4),gaggC(x))
 A*e=_A(x);A op=e[0],k=e[1],v=e[2],m=_n(x)==4?e[3]:0;
 P(_tP(k)||_n(k)<PGAG_MIN,gaggC(x))
 I code=-1;
 I(_ts(op),S nm=su(_v(op));code=!strcmp(nm,"sum")?GA_SUM:!strcmp(nm,"count")?GA_CNT:!strcmp(nm,"min")?GA_MIN:!strcmp(nm,"max")?GA_MAX:!strcmp(nm,"avg")?GA_AVG:!strcmp(nm,"first")?GA_FST:!strcmp(nm,"last")?GA_LST:-1)
 J(_tz(op),code=(I)gl_(op))
 P(!(code==GA_SUM||code==GA_CNT||code==GA_MIN||code==GA_MAX||code==GA_FST||code==GA_LST),gaggC(x))
 UC kt=_t(k);P(!(kt==tG||kt==tH||kt==tI||kt==tL||kt==tS),gaggC(x))
 N n=_n(k);int nt=par_thread_count(n);P(nt<2,gaggC(x))
 U wk=kt==tG?0:kt==tH?1:(kt==tI||kt==tS)?2:3;
 UC vt=code==GA_CNT?tL:_t(v);B vf=vt==tF;P(code==GA_SUM&&vf,gaggC(x))
 I(code!=GA_CNT&&code!=GA_FST&&code!=GA_LST,P(_tP(v)||!(vt==tG||vt==tH||vt==tI||vt==tL||vt==tF)||_n(v)!=n,gaggC(x)))
 I(code==GA_FST||code==GA_LST,P(_tP(v)||_N(v)!=n,gaggC(x)))
 P(m&&(_tP(m)||_t(m)!=tG||_n(m)!=n),gaggC(x))
 U wv=vt==tG?0:vt==tH?1:vt==tI?2:3;
 L gmn,gmx;{F w=WF;L b;MC(&b,&w,SZ b);gmn=o1(b);w=-WF;MC(&b,&w,SZ b);gmx=o1(b);}
 CO V*kp=_V(k),*vp=code==GA_CNT?0:_V(v);CO UC*mp=m?_V(m):0;
 L lo=GARD(wk,kp,0),hi=lo;F(n,L t=GARD(wk,kp,i);I(t<lo,lo=t)I(t>hi,hi=t))
 W rg=(W)hi-(W)lo+1;B direct=rg&&rg<=((W)1<<22)&&rg<=8*(W)n+1024;
 P(!direct||(W)nt*rg>PGAG_CELLS,gaggC(x))
 U cap=(U)MIN((W)n,rg),ng=0;
 L*gk=malloc((N)cap*SZ(L));I*gf=malloc((N)cap*SZ(I));F*af=malloc((N)cap*SZ(F));L*al_=malloc((N)cap*SZ(L));L*gc=malloc((N)cap*SZ(L));
 I r=gk&&gf&&af&&al_&&gc?gagg_par(code,kp,wk,vp,wv,vf,mp,n,lo,rg,direct,gmn,gmx,nt,&gk,&gf,&af,&al_,&gc,&cap,&ng):-1;
 P(r,free(gk);free(gf);free(af);free(al_);free(gc);gaggC(x))
 A ky=an(ng,kt);S4(wk,F(ng,_G(ky)[i]=(G)gk[i]),F(ng,_H(ky)[i]=(H)gk[i]),F(ng,_I(ky)[i]=(I)gk[i]),F(ng,_L(ky)[i]=gk[i]))
 A fr=aV(tI,ng,gf);A vl;
 S(code,
  C(GA_SUM,vl=aV(tL,ng,al_))
  C(GA_MIN,I(vf,F(ng,al_[i]=o0(al_[i]))vl=aV(tF,ng,al_))E(vl=aV(tL,ng,al_)))
  C(GA_MAX,I(vf,F(ng,al_[i]=o0(al_[i]))vl=aV(tF,ng,al_))E(vl=aV(tL,ng,al_)))
  C(GA_CNT,vl=aV(tL,ng,gc))
  D(vl=i1(v,_R(fr))))
 free(gk);free(gf);free(af);free(al_);free(gc);
 P(!vl,mr(ky);mr(fr);x(0))
 return x(aV(tA,3,A(ky,vl,fr)));}
