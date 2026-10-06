#include"a.h"
#include <stdlib.h>   // Amber 2.5 (exp): malloc/calloc/qsort/free for the parallel kernels
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
   RT(F(MIN(xn,yn),A z=ii(x,i),u=ii(y,i);I d=qA(z,u);mr(z(u));P(d,d))P(!xn&&!yn&&xtA,qA(xx,yx))ql(xn,yn))//empty lists: by prototype, as ~
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
Z X1(grq,Ril(K1("=/:/2#,!:",x))Rm(A y=kv(&x);y=Nx(grp(y));yy=x(i1(x,yy));y)R_(et(x))
 // amber 2.0.0: group a SYMBOL vector by its interned 4-byte id (tS is stored as
 // tI-width ids; equal symbols -> equal ids) instead of the general path, whose
 // `<x` grade lexically string-sorts symbols (o.c asc's RS(asc(str(x)))) at
 // O(n log n) with per-char compares. Grouping only needs equal elements
 // adjacent, so the id order is fine, and the partition + first-appearance key
 // order are byte-identical to the old result.  Measured on 1M rows / 100 groups:
 // ~670 ms -> ~15 ms.  cSI flips tS<->tI on the same payload; we group the ids,
 // then flip the dict's int keys back to symbols.
 // amber 2.7: short symbols are packed ids spread over a huge range, so grouping the ids hashed every row
 // (246 ms on 10M rows of 10 symbols). Dense codes first: the distinct symbols (first-seen order), each row's
 // index in them, and the codes grouped by their small range (~70 ms). The keys come out in the same order.
 RS(P(!xn,K1("{x!0#,!0}",x))A u=unq(_R(x));P(!u,x(0))A k=fnd(u,x);P(!k,mr(u);0)A d=grq(k);P(!d,mr(u);0)A v=kv(&d);mr(d);am(u,v))
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
//the keys of a group are distinct x, so they take the attribute distinct gives, as q: `s stays, `u and `g give `u,
//`p none (a dict's values likewise); a list without one pays this test only
A1(grp,UC a=_tP(x)||!_tT(x)||xtE||!xn?0:_at(x);A z=grq(x);P(!z||!a||a==3,z)zx=mut(zx);_at(zx)=a==1?1:2;z)
Z A1(cSI,Q(xtS||xtI)C t=tS^tI^xt;MINE(x)?(_at(x)=0,AT(t,x)):x(aV(t,xn,xV)))
Z I penc(A,int,A*,A*);
X1(unq,RM(K1("{$[#x;x@i@<i:&/'.=+.+x;x]}",x))   /*a table: its distinct rows, first seen first (issue #19); as q*/Rm(unq(val(x)))RE(x)RS(I(xn>=(1u<<16),A u_=0;I(!penc(x,par_thread_count(xn),&u_,0),return x(u_)))cSI(unq(cSI(x))))Ril(rndF(gl(x)))R_(et(x))RB(unq(cG(x)))
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
// amber 2.3: float min/max compare o1() keys, the order &/ and |/ use, and give back the key's
// value: either zero is 0.0 (issue #15). NaNs are skipped, as q's min/max skip nulls, so an
// all-NaN group is 0w/-0w.
#define GA_OF(i) o1(((CO L*)vp)[i])
// ---- amber 2.5 (exp): parallel gagg for the exactly-combinable ops, see patch notes / gaggC
#define PGAG_MIN (1u<<16)
#define PGAG_CELLS (1u<<22)        //Threads x key range: the private tables' total size cap
TD struct{CO V*kp,*vp;CO UC*mp;U wk,wv,nt;N n;I code;B vf;L lo;W rg;L*cnt,*acc;I*fst,*lst;}GQ;
// amber 2.7: each thread counts in its own buffers and copies them out once. With a few groups the threads' slots
// were bytes apart, every row wrote into a cache line the others were writing too, and 2 threads ran slower than 1.
Z V gq1(V*c_,int t){GQ*c=c_;N s=c->n*t/c->nt,e=c->n*(t+1)/c->nt;W rg=c->rg;L*cnt0=c->cnt+(N)t*rg,*acc0=c->acc+(N)t*rg;
 I*fst0=c->fst+(N)t*rg,*lst0=c->lst+(N)t*rg;
 L*cnt=malloc((N)rg*(2*SZ(L)+2*SZ(I)));B own=!!cnt;
 L*acc;I*fst,*lst;
 if(own){acc=cnt+rg;fst=(I*)(acc+rg);lst=fst+rg;MC(cnt,cnt0,(N)rg*SZ(L));MC(acc,acc0,(N)rg*SZ(L));}
 else{cnt=cnt0;acc=acc0;fst=fst0;lst=lst0;}CO V*vp=c->vp;U wv=c->wv;B vf=c->vf;I code=c->code;L lo=c->lo;
 // amber 2.7.2: float min and max as doubles (a NaN never compares: skipped, as below), the order keys made once per
 // slot at the end, -0.0 there as 0.0 since they are one value. The same keys as the row-by-row o1() compares
 if(own&&vf&&(code==GA_MIN||code==GA_MAX)){F*RES fa=(F*)acc;CO F*RES v=vp;F a0=code==GA_MIN?WF:-WF;for(W q=0;q<rg;q++)fa[q]=a0;
  if(code==GA_MIN){for(N i=s;i<e;i++){if(c->mp&&!c->mp[i])continue;W q=(W)GARD(c->wk,c->kp,i)-(W)lo;
    if(!cnt[q])fst[q]=(I)i;lst[q]=(I)i;cnt[q]++;F d=v[i],a=fa[q];fa[q]=d<a?d:a;}}
  else{for(N i=s;i<e;i++){if(c->mp&&!c->mp[i])continue;W q=(W)GARD(c->wk,c->kp,i)-(W)lo;
    if(!cnt[q])fst[q]=(I)i;lst[q]=(I)i;cnt[q]++;F d=v[i],a=fa[q];fa[q]=d>a?d:a;}}
  for(W q=0;q<rg;q++){F d=fa[q];if(d==0)d=0;L b;MC(&b,&d,SZ b);acc[q]=o1(b);}}
 else for(N i=s;i<e;i++){if(c->mp&&!c->mp[i])continue;W q=(W)GARD(c->wk,c->kp,i)-(W)lo;
  if(!cnt[q])fst[q]=(I)i;lst[q]=(I)i;cnt[q]++;
  S(code,
   C(GA_SUM,L d=GARD(wv,vp,i);I(d!=NL,acc[q]=(L)((W)acc[q]+(W)d)))
   C(GA_MIN,I(vf,F d=((CO F*)vp)[i];I(d==d,L t_=GA_OF(i);I(t_<acc[q],acc[q]=t_)))E(L t_=GARD(wv,vp,i);I(t_!=NL&&t_<acc[q],acc[q]=t_)))
   C(GA_MAX,I(vf,F d=((CO F*)vp)[i];I(d==d,L t_=GA_OF(i);I(t_>acc[q],acc[q]=t_)))E(L t_=GARD(wv,vp,i);I(t_!=NL&&t_>acc[q],acc[q]=t_)))
   D())}
 if(own){MC(cnt0,cnt,(N)rg*SZ(L));MC(acc0,acc,(N)rg*SZ(L));for(W q=0;q<rg;q++)if(cnt[q]){fst0[q]=fst[q];lst0[q]=lst[q];}free(cnt);}}
Z I gq_cmp(CO V*a,CO V*b){I x_=*(CO I*)a,y_=*(CO I*)b;return(x_>y_)-(x_<y_);}
// Fills the group tables as the serial loop would; -1: not taken (wrong op, range too wide, or no memory)
NI Z I gagg_par(I code,CO V*kp,U wk,CO V*vp,U wv,B vf,CO UC*mp,N n,L lo,W rg,B direct,L gmn,L gmx,int nt,
 L**pgk,I**pgf,F**paf,L**pal,L**pgc,U*pcap,U*png){
 P(!(code==GA_CNT||code==GA_SUM&&!vf||code==GA_MIN||code==GA_MAX||code==GA_FST||code==GA_LST),-1)
 P(!direct||(W)nt*rg>PGAG_CELLS,-1)
 N cells=(N)nt*(N)rg;GQ c={.kp=kp,.vp=vp,.mp=mp,.wk=wk,.wv=wv,.nt=(U)nt,.n=n,.code=code,.vf=vf,.lo=lo,.rg=rg};
 c.cnt=calloc(cells,SZ(L));c.acc=malloc(cells*SZ(L));c.fst=malloc(cells*SZ(I));c.lst=malloc(cells*SZ(I));
 I r=-1;if(!c.cnt||!c.acc||!c.fst||!c.lst)goto out;
 {L a0=code==GA_MIN?(vf?gmn:WL):code==GA_MAX?(vf?gmx:NL+1):0;F(cells,c.acc[i]=a0)}
 par_run(nt,gq1,&c);
 // Combine per slot in thread order, then order the groups by first row
 {U ng=0;I*ord=malloc((N)rg*SZ(I)*2);if(!ord)goto out;I*first=ord+rg;   //ord: (first row, slot) pairs packed as two Is
  TD struct{I f,s;}FS;FS*fs=(FS*)ord;
  for(W q=0;q<rg;q++){I fr=-1;for(int w=0;w<nt;w++)if(c.cnt[(N)w*rg+q]){fr=c.fst[(N)w*rg+q];break;}if(fr>=0){fs[ng].f=fr;fs[ng].s=(I)q;ng++;}}
  qsort(fs,ng,SZ(FS),gq_cmp);   //By first row (unique per slot), so the serial first-appearance order
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
// ---- amber 2.7.2: the serial gagg, rows in chunks of GS_CH. A chunk's keys become group
// numbers first (a direct table over a small key range, else a hash holding each key next to its group), then one
// tight loop per op runs over the numbers. Same groups, same order, same adds in the same order as the row loop in
// gaggC, so the same bits; it only drops the per-row switches and the second load of the old hash. Symbol keys are
// what gain most: 4-letter names are ids a billion apart, so they always hashed.
#define GS_CH 4096
#define GS_MIN 4096
#define GS_GROW() I(ng==cap,U nc=cap*2;L*a1=realloc(gk,(N)nc*SZ(L));I(a1,gk=a1)I*a2=realloc(gf,(N)nc*SZ(I));I(a2,gf=a2)\
 F*a3=realloc(af,(N)nc*SZ(F));I(a3,af=a3)L*a4=realloc(al_,(N)nc*SZ(L));I(a4,al_=a4)L*a5=realloc(gc,(N)nc*SZ(L));I(a5,gc=a5)\
 I(!a1||!a2||!a3||!a4||!a5,fail=1;goto done)cap=nc;)
#define GS_NEW(key,r) {GS_GROW();g=(I)ng++;gk[g]=key;gf[g]=(I)(r);gc[g]=0;af[g]=a0f;al_[g]=a0l;}
#define GS_DIR(key,r) {W q_=(W)key-(W)lo;g=slot[q_]-1;if(g<0){GS_NEW(key,r)slot[q_]=g+1;}}
#define GS_HSH(key,r) {W j_=((W)key*GOLD)>>(64-hlg);while(hg[j_]&&hk[j_]!=key)j_=(j_+1)&(hcap-1);\
 if(hg[j_])g=hg[j_]-1;else{GS_NEW(key,r)hk[j_]=key;hg[j_]=g+1;if((W)ng*2>=hcap&&gsrh(&hk,&hg,&hcap,&hlg)){fail=1;goto done;}}}
// masked: the chunk's chosen rows are listed first, without a branch (a random mask mispredicts every other row)
#define GS_A(KT,MAP,MSK) {CO KT*RES kk=(CO KT*)kp; if(MSK){U m_=0;for(N i=s;i<e;i++){ix[m_]=(I)i;m_+=mp[i]!=0;}for(U j=0;j<m_;j++){N i=(N)ix[j];L key=(L)kk[i];I g;MAP(key,i)cd[j]=g;}nc=m_;} else{for(N i=s;i<e;i++){L key=(L)kk[i];I g;MAP(key,i)cd[nc++]=g;}}}
#define GS_B(ROW) {if(mp){for(U j=0;j<nc;j++){I g=cd[j];N r=(N)ix[j];gc[g]++;ROW}}else{for(U j=0;j<nc;j++){I g=cd[j];N r=s+j;gc[g]++;ROW}}}
#define GS_AK(KT) {if(direct){if(mp)GS_A(KT,GS_DIR,1)else GS_A(KT,GS_DIR,0)}else{if(mp)GS_A(KT,GS_HSH,1)else GS_A(KT,GS_HSH,0)}}
#define GS_VT(T,ROW) {CO T*RES vT=vp;GS_B(L d=(L)vT[r];ROW)}
#define GS_IW(ROW) switch(wv){case 0:GS_VT(G,ROW)break;case 1:GS_VT(H,ROW)break;case 2:GS_VT(I,ROW)break;default:GS_VT(L,ROW)}
// grows the hash 2x, the keys rehashed into their new slots
Z I gsrh(L**phk,I**phg,W*phc,U*phl){W nc=*phc<<1;U nl=*phl+1;L*nk=malloc((N)nc*SZ(L));I*ng_=calloc((N)nc,SZ(I));
 I(!nk||!ng_,free(nk);free(ng_);return 1;)L*hk=*phk;I*hg=*phg;
 for(W i=0;i<*phc;i++)if(hg[i]){W j=((W)hk[i]*GOLD)>>(64-nl);while(ng_[j])j=(j+1)&(nc-1);nk[j]=hk[i];ng_[j]=hg[i];}
 free(hk);free(hg);*phk=nk;*phg=ng_;*phc=nc;*phl=nl;return 0;}
// 0: the tables are filled (the caller frees them); -1: not taken, or out of memory
NI Z I gaggS(I code,CO V*kp,U wk,CO V*vp,U wv,B vf,CO UC*mp,N n,L gmn,L gmx,L**pgk,I**pgf,F**paf,L**pal,L**pgc,U*png){
 // the key range: 4096 sampled keys first, the full scan only when they span a direct table's worth
 L lo,hi;B direct=0;
 {N sd=n/4096;sd+=!sd;
  lo=hi=GARD(wk,kp,0);for(N i=0;i<n;i+=sd){L v=GARD(wk,kp,i);if(v<lo)lo=v;if(v>hi)hi=v;}
  if((W)hi-(W)lo<((W)1<<22)){
   if(wk<2){for(N i=0;i<n;i++){L v=GARD(wk,kp,i);if(v<lo)lo=v;if(v>hi)hi=v;}}
   else if(wk==2){CO I*RES p=kp;I a=(I)lo;I b=(I)hi;for(N i=0;i<n;i++){I v=p[i];a=v<a?v:a;b=v>b?v:b;}lo=a;hi=b;}
   else{CO L*RES p=kp;L a=lo;L b=hi;for(N i=0;i<n;i++){L v=p[i];a=v<a?v:a;b=v>b?v:b;}lo=a;hi=b;}
   W rg=(W)hi-(W)lo+1;direct=rg&&rg<=((W)1<<22)&&rg<=8*(W)n+1024;}}
 L a0l=code==GA_MIN?(vf?gmn:WL):code==GA_MAX?(vf?gmx:NL+1):0;F a0f=code==GA_MIN?WF:code==GA_MAX?-WF:0;
 U cap=1024,ng=0;B fail=0;
 L*gk=malloc(cap*SZ(L));I*gf=malloc(cap*SZ(I));F*af=malloc(cap*SZ(F));L*al_=malloc(cap*SZ(L));L*gc=malloc(cap*SZ(L));
 I*slot=direct?calloc((N)((W)hi-(W)lo+1),SZ(I)):0;W hcap=direct?0:2048;U hlg=11;L*hk=direct?0:malloc(2048*SZ(L));I*hg=direct?0:calloc(2048,SZ(I));
 I*cd=malloc(GS_CH*SZ(I));I*ix=mp?malloc(GS_CH*SZ(I)):0;
 I(!gk||!gf||!af||!al_||!gc||(direct?!slot:!hk||!hg)||!cd||mp&&!ix,fail=1;goto done)
 {CO F*RES vF=vp;CO L*RES vL=vp;
 for(N s=0;s<n;s+=GS_CH){N e=s+GS_CH<n?s+GS_CH:n;U nc=0;
  switch(wk){case 0:GS_AK(G)break;case 1:GS_AK(H)break;case 2:GS_AK(I)break;default:GS_AK(L)}
  switch(code){
   case GA_CNT:case GA_FST:GS_B();break;
   case GA_LST:GS_B(gf[g]=(I)r;);break;
   // issue #14: sum avg min max skip nulls, as gaggC's row loop
   case GA_SUM:if(vf)GS_B(F d=vF[r];if(d==d)af[g]+=d;)else GS_IW(if(d!=NL)al_[g]+=d;)break;
   case GA_AVG:if(vf)GS_B(F d=vF[r];if(d==d){af[g]+=d;al_[g]++;})else GS_IW(if(d!=NL){af[g]+=(F)d;al_[g]++;})break;
   case GA_MIN:if(vf)GS_B(F d=vF[r];F a=af[g];af[g]=d<a?d:a;)else GS_IW(if(d!=NL&&d<al_[g])al_[g]=d;)break;
   case GA_MAX:if(vf)GS_B(F d=vF[r];F a=af[g];af[g]=d>a?d:a;)else GS_IW(if(d!=NL&&d>al_[g])al_[g]=d;)break;}}}
 // float min and max ran as doubles (a NaN never compares, so it is skipped, as the row loop skips it); the order
 // keys only differ there for -0.0 against 0.0, which are one value: either comes out 0.0, as o0(o1(.)) gives
 if(vf&&(code==GA_MIN||code==GA_MAX))for(U q=0;q<ng;q++){F d=af[q];if(d==0)d=0;L b;MC(&b,&d,SZ b);al_[q]=o1(b);}
 done:
 free(slot);free(hk);free(hg);free(cd);free(ix);
 I(fail,free(gk);free(gf);free(af);free(al_);free(gc);return -1;)
 *pgk=gk;*pgf=gf;*paf=af;*pal=al_;*pgc=gc;*png=ng;return 0;}
#undef GS_GROW
#undef GS_NEW
#undef GS_DIR
#undef GS_HSH
#undef GS_A
#undef GS_B
#undef GS_AK
#undef GS_VT
#undef GS_IW
// gagg's answer from the group tables: (keys; aggregates; first rows), 0 on an error
Z A gares(I code,A v,UC kt,U wk,B vf,U ng,L*gk,I*gf,F*af,L*al_,L*gc){
 A ky=an(ng,kt);S4(wk,F(ng,_G(ky)[i]=(G)gk[i]),F(ng,_H(ky)[i]=(H)gk[i]),F(ng,_I(ky)[i]=(I)gk[i]),F(ng,_L(ky)[i]=gk[i]))
 A fr=aV(tI,ng,gf);A vl;
 S(code,
  C(GA_SUM,I(vf,vl=aV(tF,ng,af))E(vl=aV(tL,ng,al_)))
  C(GA_AVG,vl=an(ng,tF);F(ng,_F(vl)[i]=af[i]/(F)al_[i]))   //over the non-null items; none: 0n
  C(GA_MIN,I(vf,F(ng,al_[i]=o0(al_[i]))vl=aV(tF,ng,al_))E(vl=aV(tL,ng,al_)))
  C(GA_MAX,I(vf,F(ng,al_[i]=o0(al_[i]))vl=aV(tF,ng,al_))E(vl=aV(tL,ng,al_)))
  C(GA_CNT,vl=aV(tL,ng,gc))
  D(vl=i1(v,_R(fr))))
 P(!vl,mr(ky);mr(fr);0)
 return aV(tA,3,A(ky,vl,fr));}
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
 I(n>=GS_MIN,   //the value types are checked above
   L*sk=0,*sl=0,*sc=0;I*sf=0;F*sa=0;U sn=0;
   I(!gaggS(code,kp,wk,vp,wv,vf,mp,n,gmn,gmx,&sk,&sf,&sa,&sl,&sc,&sn),
     A z_=gares(code,v,kt,wk,vf,sn,sk,sf,sa,sl,sc);free(sk);free(sf);free(sa);free(sl);free(sc);I(mb_,mr(mb_))return x(z_);))
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
 A z_=gares(code,v,kt,wk,vf,ng,gk,gf,af,al_,gc);
 free(gk);free(gf);free(af);free(al_);free(gc);free(slot);free(ht);I(mb_,mr(mb_))
 x(z_))
#undef GA_GROW
#undef GA_ADD
#undef GA_OF
// ---- 2.5 (exp): float sum and avg by group in parallel, from PGAF_MIN (256K) rows on a direct key range. The rows are cut
// into nb fixed blocks -- nb set by n and the key range only, never by the thread count -- each block keeps its own
// per-group count, first row and float sum (skipping NaN, as gaggC), and the blocks are added in order: one answer for
// any thread count from 2 up (last-bit different from gaggC's single running sum). One thread, or more than 32K groups,
// stays on gaggC: there the blocks cost more than they save (up to 10% on one thread, nothing gained at 100K groups).
// Group order: first row, as gaggC.
#define PGAF_MIN (1u<<18)
#define PGAF_CELLS (1u<<20)
TD struct{I f,s;}GFS;   //(first row, slot), sorted by first row, which is unique per slot
TD struct{CO V*kp;CO F*vp;CO UC*mp;U wk;N n,bs,nb;L lo;W rg;L*cnt;F*sum;I*fst;N next;}GF;
// amber 2.7.2: a block is counted in the thread's own buffer and copied out when done. With a few groups the blocks'
// tables were bytes apart, so threads on neighbouring blocks wrote the same cache lines on every row.
#define GF_ROW(KT) {W q=(W)((CO KT*)c->kp)[i]-(W)lo;if(!cnt[q])fst[q]=(I)i;F d=c->vp[i];if(d==d){sm[q]+=d;cnt[q]++;}else cnt[q]+=(L)1<<40;}   //A NaN row still makes its group: high count bits
#define GF_LOOP(KT) {if(mp){for(N i=s;i<e;i++){if(!mp[i])continue;GF_ROW(KT)}}else{for(N i=s;i<e;i++)GF_ROW(KT)}}
Z V gf1(V*c_,int t){GF*c=c_;(V)t;W rg=c->rg;L lo=c->lo;CO UC*mp=c->mp;
 L*own=malloc((N)rg*(SZ(L)+SZ(F)+SZ(I)));
 for(;;){N b=__atomic_fetch_add(&c->next,1,__ATOMIC_RELAXED);if(b>=c->nb)break;
  N s=b*c->bs;N e=s+c->bs<c->n?s+c->bs:c->n;L*cnt0=c->cnt+b*rg;F*sm0=c->sum+b*rg;I*fst0=c->fst+b*rg;
  L*cnt=cnt0;F*sm=sm0;I*fst=fst0;
  if(own){cnt=own;sm=(F*)(own+rg);fst=(I*)(sm+rg);MS(cnt,0,(N)rg*SZ(L));MS(sm,0,(N)rg*SZ(F));}
  switch(c->wk){case 0:GF_LOOP(G)break;case 1:GF_LOOP(H)break;case 2:GF_LOOP(I)break;default:GF_LOOP(L)}
  if(own){MC(cnt0,cnt,(N)rg*SZ(L));MC(sm0,sm,(N)rg*SZ(F));for(W q=0;q<rg;q++)if(cnt[q])fst0[q]=fst[q];}}
 free(own);}
#undef GF_LOOP
#undef GF_ROW
// (keys ; sums or averages ; first rows), or 0 when not taken
// the key range on nt threads (each a slice, then the min of the mins and max of the maxes)
TD struct{CO V*kp;U wk;N n;int nt;L mn[64],mx[64];}GMM;
Z V gmm1(V*c_,int t){GMM*c=c_;N s=c->n*t/c->nt,e=c->n*(t+1)/c->nt;L a=GARD(c->wk,c->kp,s),b=a;
 switch(c->wk){case 0:{CO G*p=c->kp;for(N i=s;i<e;i++){L v=p[i];if(v<a)a=v;if(v>b)b=v;}}break;
  case 1:{CO H*p=c->kp;for(N i=s;i<e;i++){L v=p[i];if(v<a)a=v;if(v>b)b=v;}}break;
  case 2:{CO I*p=c->kp;for(N i=s;i<e;i++){L v=p[i];if(v<a)a=v;if(v>b)b=v;}}break;
  default:{CO L*p=c->kp;for(N i=s;i<e;i++){L v=p[i];if(v<a)a=v;if(v>b)b=v;}}}
 c->mn[t]=a;c->mx[t]=b;}
Z V gmmP(CO V*kp,U wk,N n,int nt,L*lo,L*hi){GMM c={.kp=kp,.wk=wk,.n=n,.nt=nt>64?64:nt};par_run(c.nt,gmm1,&c);
 *lo=c.mn[0];*hi=c.mx[0];for(int t=1;t<c.nt;t++){if(c.mn[t]<*lo)*lo=c.mn[t];if(c.mx[t]>*hi)*hi=c.mx[t];}}
Z A gaggF(I code,A k,UC kt,U wk,CO F*vp,CO UC*mp,N n,L lo,W rg,int nt){
 N nb=(n+(((N)1<<18)-1))>>18;N cap=PGAF_CELLS/(N)rg;   //256K-row blocks: enough rows per group that a block's table pays for itself
if(cap<2)return 0;if(nb>cap)nb=cap;N bs=(n+nb-1)/nb;nb=(n+bs-1)/bs;
 N cells=nb*(N)rg;GF c={.kp=_V(k),.vp=vp,.mp=mp,.wk=wk,.n=n,.bs=bs,.nb=nb,.lo=lo,.rg=rg,.next=0};
 c.cnt=calloc(cells,SZ(L));c.sum=calloc(cells,SZ(F));c.fst=malloc(cells*SZ(I));A r=0;
 if(c.cnt&&c.sum&&c.fst){int t=nt>(int)nb?(int)nb:nt;par_run(t,gf1,&c);
  // Per slot: total count, sum and first row (blocks are in row order: the first block holding the slot has its
  // first row); then the groups in first-row order from a bitmap over the rows -- a qsort of 100k groups cost more
  // than the aggregation
  L*tc=calloc((N)rg,SZ(L));F*ts=malloc((N)rg*SZ(F));I*tf=malloc((N)rg*SZ(I));W*bm=calloc((n+63)>>6,SZ(W));
  if(tc&&ts&&tf&&bm){U ng=0;
   for(W q=0;q<rg;q++){F sm=0;L cn=0;I f=-1;for(N b=0;b<nb;b++){N j=b*rg+q;L cj=c.cnt[j];if(cj){if(f<0)f=c.fst[j];sm+=c.sum[j];cn+=cj;}}
    ts[q]=sm;tc[q]=cn;tf[q]=f;if(f>=0){bm[(W)f>>6]|=1ull<<((W)f&63);ng++;}}
   A ky=an(ng,kt);A vl=an(ng,tF);A fr=an(ng,tI);U g=0;
   for(W w=0;w<(n+63)>>6;w++){W bb=bm[w];while(bb){W p=(w<<6)|(W)__builtin_ctzll(bb);bb&=bb-1;W q=(W)GARD(wk,c.kp,p)-(W)lo;L key=lo+(L)q;
     if(wk==0)_G(ky)[g]=(G)key;else if(wk==1)_H(ky)[g]=(H)key;else if(wk==2)_I(ky)[g]=(I)key;else _L(ky)[g]=key;
     _F(vl)[g]=code==GA_AVG?ts[q]/(F)(tc[q]&(((L)1<<40)-1)):ts[q];_I(fr)[g]=(I)p;g++;}}
   r=aV(tA,3,A(ky,vl,fr));}
  free(tc);free(ts);free(tf);free(bm);}
 free(c.cnt);free(c.sum);free(c.fst);return r;}
// ---- amber 2.5 (exp): `gagg comes here first. The parallel aggregation (gagg_par) has its own entry point rather
// than a branch in gaggC: inside gaggC it made GCC compile the serial loop 3-10% slower even where it is never taken.
// gaggT takes what gagg_par does exactly -- count, int sum, min, max, first, last over a direct key range, n at least
// PGAG_MIN, more than one thread, no bit mask -- checked as gaggC checks it, and builds the result as gaggC does;
// everything else, and anything gagg_par declines, goes to gaggC as it always went.
// ---- amber 2.7.2: 32/64-bit keys to codes on the threads, one pass. Each thread hashes its own rows to local codes
// and keeps its new keys in the order it met them; the lists are merged thread by thread, which is first-appearance
// order over the whole vector (a thread's new keys all come after every row of the threads before it); then each
// thread swaps its local codes for the merged ones. Same codes and keys at any thread count.
TD struct{CO V*kp;U wk;N n;int nt;I*cd;L*lk[PAR_MAX_THREADS];U ln[PAR_MAX_THREADS];I*lm[PAR_MAX_THREADS];int bad;}PE;
#define PE_LOOP(KT,ST) {CO KT*RES kk=(CO KT*)c->kp;for(N i=s;i<e;i++){L key=(L)kk[i];W j=((W)key*GOLD)>>(64-hl);\
 while(hg[j]&&hk[j]!=key)j=(j+1)&(hc-1);I g;\
 if(hg[j])g=hg[j]-1;else{if(ng==cap){U nc_=cap*2;L*a=realloc(lk,(N)nc_*SZ(L));if(!a){c->bad=1;break;}lk=a;cap=nc_;}\
  g=(I)ng++;lk[g]=key;hk[j]=key;hg[j]=g+1;if((W)ng*2>=hc&&gsrh(&hk,&hg,&hc,&hl)){c->bad=1;break;}}\
 ST}}
Z V pe1(V*c_,int t){PE*c=c_;N s=c->n*t/c->nt,e=c->n*(t+1)/c->nt;I*RES cd=c->cd;
 W hc=1024;U hl=10,cap=256,ng=0;L*hk=malloc(1024*SZ(L));I*hg=calloc(1024,SZ(I));L*lk=malloc(256*SZ(L));
 if(hk&&hg&&lk){if(cd){if(c->wk==2)PE_LOOP(I,cd[i]=g;)else PE_LOOP(L,cd[i]=g;)}else{if(c->wk==2)PE_LOOP(I,)else PE_LOOP(L,)}}else c->bad=1;
 free(hk);free(hg);c->lk[t]=lk;c->ln[t]=ng;}
Z V pe3(V*c_,int t){PE*c=c_;N s=c->n*t/c->nt,e=c->n*(t+1)/c->nt;I*RES cd=c->cd;CO I*RES m=c->lm[t];if(!m)return;for(N i=s;i<e;i++)cd[i]=m[cd[i]];}
#undef PE_LOOP
// k (tI tL or tS, n of them) -> *pu its distinct items, first seen first, and *pcd each row's index in *pu (tI);
// pcd 0: the distinct items only. 1: declined
Z I penc(A k,int nt,A*pu,A*pcd){N n=_n(k);UC kt=_t(k);U wk=kt==tL?3:2;
 A cdA=pcd?an((U)n,tI):0;P(pcd&&!cdA,1)
 PE c={.kp=_V(k),.wk=wk,.n=n,.nt=nt,.cd=cdA?_I(cdA):0};
 par_run(nt,pe1,&c);
 I r=1;A u=0;W gc=16;U glg=4,tot=0;F(nt,tot+=c.ln[i])while(gc<2*(W)tot+2){gc<<=1;glg++;}
 L*gk=malloc((N)gc*SZ(L));I*gv=calloc((N)gc,SZ(I));L*uk_=malloc(((N)tot+1)*SZ(L));U m=0;
 if(c.bad||!gk||!gv||!uk_)goto out;
 for(int t=0;t<nt;t++){I*lm=malloc(((N)c.ln[t]+1)*SZ(I));c.lm[t]=lm;if(!lm)goto out;
  for(U q=0;q<c.ln[t];q++){L key=c.lk[t][q];W j=((W)key*GOLD)>>(64-glg);while(gv[j]&&gk[j]!=key)j=(j+1)&(gc-1);
   if(!gv[j]){gk[j]=key;gv[j]=(I)m+1;uk_[m++]=key;}lm[q]=gv[j]-1;}}
 if(cdA&&nt>1)par_run(nt,pe3,&c);   //one thread: its codes are the merged ones already
 u=an(m,kt);if(!u)goto out;S4(wk,,,F(m,_I(u)[i]=(I)uk_[i]),F(m,_L(u)[i]=uk_[i]))r=0;
 out:
 free(gk);free(gv);free(uk_);F(nt,free(c.lk[i]);free(c.lm[i]))
 I(r,I(cdA,mr(cdA))return 1;)
 *pu=u;I(pcd,*pcd=cdA)return 0;}
// `senc k: (distinct;codes) of a big symbol vector on more than one thread, for a group-by with several aggregates
// to hash once (qaggf); () otherwise -- on one thread gaggS hashing per aggregate is cheaper than the codes
A sencT(A x){P(_tP(x)||_t(x)!=tS||_n(x)<PGAG_MIN||par_thread_count(_n(x))<2,x(emp(tA)))
 A u=0,cd=0;P(penc(x,par_thread_count(_n(x)),&u,&cd),x(emp(tA)))
 return x(aV(tA,2,A(u,cd)));}
A gaggT(A x){
 P(_t(x)-tA||(_n(x)-3&&_n(x)-4),gaggC(x))
 A*e=_A(x);A op=e[0],k=e[1],v=e[2],m=_n(x)==4?e[3]:0;
 // amber 2.7: symbol keys on more than one thread: as small codes (penc, threaded), aggregated on the threads over
 // that direct range, the keys mapped back. On one thread gaggS hashes them as it goes, which is cheaper.
 I(!_tP(k)&&_t(k)==tS&&_n(k)>=PGAG_MIN&&par_thread_count(_n(k))>1,
   A u=0,cd=0;I(!penc(k,par_thread_count(_n(k)),&u,&cd),
     I(cd,U nx=_n(x);A x2=an(nx,tA);F(nx,_A(x2)[i]=i==1?cd:_R(e[i]))A r=gaggT(x2);
       I(r&&!_tP(r)&&_t(r)==tA&&_n(r)==3,A ks=i1(u,_R(_A(r)[0]));I(ks,A z=aV(tA,3,A(ks,_R(_A(r)[1]),_R(_A(r)[2])));mr(r);mr(u);return x(z);))
       mr(u);I(r&&!_tP(r)&&_t(r)==tA&&!_n(r),return x(r);)   //declined (op, values or mask): (), as on one thread
       I(r,mr(r))return x(0);)   //codes made but no answer: the error stands
     mr(u);))
 P(_tP(k)||_n(k)<PGAG_MIN,gaggC(x))
 I code=-1;
 I(_ts(op),S nm=su(_v(op));code=!strcmp(nm,"sum")?GA_SUM:!strcmp(nm,"count")?GA_CNT:!strcmp(nm,"min")?GA_MIN:!strcmp(nm,"max")?GA_MAX:!strcmp(nm,"avg")?GA_AVG:!strcmp(nm,"first")?GA_FST:!strcmp(nm,"last")?GA_LST:-1)
 J(_tz(op),code=(I)gl_(op))
 P(!(code==GA_SUM||code==GA_AVG||code==GA_CNT||code==GA_MIN||code==GA_MAX||code==GA_FST||code==GA_LST),gaggC(x))
 UC kt=_t(k);P(!(kt==tG||kt==tH||kt==tI||kt==tL||kt==tS),gaggC(x))
 N n=_n(k);int nt=par_thread_count(n);
 U wk=kt==tG?0:kt==tH?1:(kt==tI||kt==tS)?2:3;
 UC vt=code==GA_CNT?tL:_t(v);B vf=vt==tF;
 if(nt>1&&(code==GA_SUM||code==GA_AVG)&&vf&&!_tP(v)&&_n(v)==n&&(W)n>=PGAF_MIN&&!(m&&(_tP(m)||(_t(m)!=tG&&_t(m)!=tB)||_n(m)!=n))){   //floats: the blocked path
  A mw=m&&_t(m)==tB?cG(_R(m)):0;CO UC*mp_=mw?_V(mw):m?_V(m):0;   //A bit mask is widened to bytes, as gaggC does
  CO V*kp=_V(k);L lo=GARD(wk,kp,0),hi=lo;N sd=n/4096;for(N i=0;i<n;i+=sd){L t=GARD(wk,kp,i);if(t<lo)lo=t;if(t>hi)hi=t;}   //4096 keys first: if they
  if((W)hi-(W)lo<((W)1<<15))gmmP(kp,wk,n,nt,&lo,&hi);   //Already span too much, so does the column, and the full scan is skipped
  W rg=(W)hi-(W)lo+1;   //The key range on all threads: a serial pass cost ~8ms at 10M, wasted when the range is too wide
  if(rg&&rg<=((W)1<<15)&&rg<=8*(W)n+1024){A z_=gaggF(code,k,kt,wk,_V(v),mp_,n,lo,rg,nt);if(z_){if(mw)mr(mw);return x(z_);}}
  if(mw)mr(mw);}
 P(code==GA_SUM&&vf||code==GA_AVG,gaggC(x))
 P(nt<2,gaggC(x))
 I(code!=GA_CNT&&code!=GA_FST&&code!=GA_LST,P(_tP(v)||!(vt==tG||vt==tH||vt==tI||vt==tL||vt==tF)||_n(v)!=n,gaggC(x)))
 I(code==GA_FST||code==GA_LST,P(_tP(v)||_N(v)!=n,gaggC(x)))
 P(m&&(_tP(m)||_t(m)!=tG||_n(m)!=n),gaggC(x))
 U wv=vt==tG?0:vt==tH?1:vt==tI?2:3;
 L gmn,gmx;{F w=WF;L b;MC(&b,&w,SZ b);gmn=o1(b);w=-WF;MC(&b,&w,SZ b);gmx=o1(b);}
 CO V*kp=_V(k),*vp=code==GA_CNT?0:_V(v);CO UC*mp=m?_V(m):0;
 L lo=GARD(wk,kp,0),hi=lo;gmmP(kp,wk,n,nt,&lo,&hi);   //the key range on all threads (a serial scan was ~10 ms on 10M rows)
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
