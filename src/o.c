#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
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
 P(!xtR||(LH(tB,xt,tS)&&xt==yt&&xn==yn),
   B e=!memcmp(xV,yV,((W)xn<<Tw[xt])+7>>3);
   e||!(xtf||xtF)?e:mtcF(xV,yV,xtf?1u:xn))
 F(xn|!xn,P(!mtc_(xa,ya),0))1)

A2(mtc,/*01*/y(ai(mtc_(x,y))))
Z CO W o=(-1ull>>12)-1;Z L t(L v)_(v^(W)(v>>63)>>1);Z A of_(A,I);
Z L o0(L v)_(t(v-o))Z V of0LL(CO L*a,L*r,N n){F(n+3&~3,r[i]=o0(a[i]))}A1(of0,Q(xtlL);of_(x,0))
Z L o1(L v)_(t(v)+o)Z V of1LL(CO L*a,L*r,N n){F(n+3&~3,r[i]=o1(a[i]))}A1(of1,Q(xtfF);of_(x,1))
Z A of_(A x,I f)_(N n=xn;C t=xt+(tf-tl)*(1-2*f);A y=MINE(x)?AT(t,xR):an(n,t);Mx((f?of1LL:of0LL)(xV,yV,n))y)
Z I ql(L i,L j)_(i<j?-1:i>j)
I qf(F u,F v)_(ql(o1(*(L*)&u),o1(*(L*)&v)))
I qA(A x,A y/*00*/)_(I v=TS[xt]-TS[yt];P(v,v)
 X(Ril(ql(gl_(x),gl_(y)))
   Rf(qf(*xF,*yF))
   Rs(S s=su(xv);C t[8];U n=SL(s);I(n<5,s=MC(t,s,n+1))strcmp(s,su(yv)))
   RT(F(MIN(xn,yn),A z=ii(x,i),u=ii(y,i);I d=qA(z,u);mr(z(u));P(d,d))ql(xn,yn))
   Ropqr(x=str(xR);y=str(yR);I r=qA(x,y);x(y(0));r)
   R_(ql(x,y)))0)
Z I*ascZ(CO UC*v,UC*g,I*a,I*b,I n,I w)_(U c[257];tilV(a,0,n,2);Fj(w,MS(c,0,SZ c);F(n,g[i]=v[w*a[i]+j])F(n,c[g[i]+1]++)I(c[1+*g]-n,F(255,c[i+1]+=c[i])F(n,b[c[g[i]]++]=a[i])SW(b,a)))a)
Z A grdm(A x/*1*/,A1 f)_(A y=kv(&x);x(x1(Nx(f(y)))))

Z V mrg(A x/*0*/,I*p,I*q,I*b,I*d,I k){I*r=p-q+b;W(1,I(qA(xA[*p],xA[*b])<k,*r++=*p++;P(p==q))E(*r++=*b++;B(b==d)))MC(r,p,q-p<<2);}//merge(k=1),mergeR(k=0)
Z V cis(A x/*0*/,I*p,N n,I*r){F(n,I j=0,k=i,v=p[i];A y=xA[v];W(j<k,I m=j+k>>1;I(qA(y,xA[r[m]])<0,k=m)E(j=m+1))memmove(r+j+1,r+j,i-j<<2);r[j]=v)}//copying_insertionsort
Z V cms(A x/*0*/,I*p,N n,I*r){P(n<17,cis(x,p,n,r);)N m=n/2;cms(x,p+m,n-m,r+m);cms(x,p,m,p+m);mrg(x,p+m,p+2*m,r+m,r+n,1);}//copying_mergesort
A1(ascA,N n=xn;A z=aI(n);I*p=zI;tilV(p,0,n,2);P(n<17,cis(x,p,n,p);x(z))N m=n/2;A y=aI(n-m);I*t=yI;cms(x,p+m,n-m,t);cms(x,p,m,p+n-m);mrg(x,t,t+n-m,p+n-m,p+n,0);x(y(z)))
// The pre-batch-2 grade, kept verbatim as the fallback for every case rdxg()
// declines (an exotic width, or an arena that could not supply scratch): the
// subtract-the-minimum normalisation plus ascZ()'s byte-wise radix, which
// re-gathers the value array through the permutation on every one of its
// passes. Floats reach it only via of1(), i.e. through a fully materialised
// order-preserving copy of the vector -- exactly the two costs rdxg() removes.
Z A1(ascB,P(xtF,asc(of1(x)))
 x=N(K1("{x-&/x}",x));N n=xn;A y=aC(n),z=aI(n),u=aI(n);Mx(My(u=ascZ(xV,yV,zV,uV,n,(1ll<<xw)+7>>3)==zV?u(z):z(u)))u)
X1(asc,Rt(opn(x))Rm(grdm(x,asc))RM(K1("{(!#x){x@<y x}/|.+x}",x))RS(asc(str(x)))RA(P(xn-(I)xn,ez(x))ascA(x))RE(Lij x(0);aE(0,j-i))
 RGC(P(xn-(I)xn,ez(x))N n=xn;I c[257]={},*c129=c+129;F(n,c129[xg]++)F(256,c[i+1]+=c[i])A y=aI(n);I*c128=c+128;Mx(F(n,yI[c128[xg]++]=i))ct(tZ(n-1),y))
 // amber batch 2: 16/32/64-bit integers and IEEE-754 doubles go through the
 // key-carrying LSD radix in src/v.c -- one sequential pass per SIGNIFICANT key
 // byte, constant byte columns skipped, already-ordered input recognised in the
 // single extraction pass and answered with the identity permutation.
 R4(tH,tI,tL,tF,P(xn-(I)xn,ez(x))N n=xn;A y=cntgrd(x);I(!y,y=rdxg(x))P(!y,ascB(x))x(ct(tZ(n-1),y)))
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
 R5(tA,tE,tL,tF,tM,K1("{$[#x;x[*'g]!g@:<g:(&~x~':x i)_i:<x;x!0#,!0]}",x)))
Z A1(cSI,Q(xtS||xtI)C t=tS^tI^xt;MINE(x)?AT(t,x):x(aV(t,xn,xV)))
X1(unq,RM(en(x))Rm(unq(val(x)))RE(x)RS(cSI(unq(cSI(x))))Ril(rndF(gl(x)))R_(et(x))RB(unq(cG(x)))
 RGC(C a[256]={},r[256],t=xt;U n=0;Mx(F(xn,UC v=xg;I(!a[v],a[v]=1;r[n++]=v)))aV(t,n,r))
 R5(tA,tH,tI,tL,tF,P(xn<2,x)
  {A u_=unqL(x);P(u_,x(u_))}                 /*amber: C hash/LUT distinct, 0 = not handled*/
  P(xn<<xw-3<pg&&!xtA,K1("{x@&(x?x)=!#x}",x))K1("{x@i@<i@:&@[;0;:;1]@~~':x@i:<x}",x)))

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
#define GA_ADD(g,i) {af[g]=0;al_[g]=0;gc[g]=0;gf[g]=(I)(i);I(code==GA_MIN,af[g]=WF;al_[g]=WL)I(code==GA_MAX,af[g]=-WF;al_[g]=NL)}
A1(gaggC,P(_t(x)-tA||(_n(x)-3&&_n(x)-4),et(x))A*e=_A(x);A op=e[0],k=e[1],v=e[2],m=_n(x)==4?e[3]:0;
 I code=-1;
 I(_ts(op),S nm=su(_v(op));code=!strcmp(nm,"sum")?GA_SUM:!strcmp(nm,"count")?GA_CNT:!strcmp(nm,"min")?GA_MIN:!strcmp(nm,"max")?GA_MAX:!strcmp(nm,"avg")?GA_AVG:!strcmp(nm,"first")?GA_FST:!strcmp(nm,"last")?GA_LST:-1)
 J(_tz(op),code=(I)gl_(op))
 P(code<0||code>GA_LST,x(emp(tA)))
 UC kt=_t(k);P(_tP(k)||!(kt==tG||kt==tH||kt==tI||kt==tL||kt==tS),x(emp(tA)))
 N n=_n(k);U wk=kt==tG?0:kt==tH?1:(kt==tI||kt==tS)?2:3;
 UC vt=code==GA_CNT?tL:_t(v);B vf=vt==tF;U wv=vt==tG?0:vt==tH?1:vt==tI?2:3;
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
   C(GA_SUM,I(vf,af[g]+=((CO F*)vp)[i])E(al_[g]+=GARD(wv,vp,i)))
   C(GA_AVG,I(vf,af[g]+=((CO F*)vp)[i])E(af[g]+=(F)GARD(wv,vp,i)))
   C(GA_MIN,I(vf,F t=((CO F*)vp)[i];I(t<af[g],af[g]=t))E(L t=GARD(wv,vp,i);I(t<al_[g],al_[g]=t)))
   C(GA_MAX,I(vf,F t=((CO F*)vp)[i];I(t>af[g],af[g]=t))E(L t=GARD(wv,vp,i);I(t>al_[g],al_[g]=t)))
   C(GA_LST,gf[g]=(I)i)
   D())}
 P(fail,free(gk);free(gf);free(af);free(al_);free(gc);free(slot);free(ht);I(mb_,mr(mb_))x(emp(tA)))
 // ---- results
 A ky=an(ng,kt);S4(wk,F(ng,_G(ky)[i]=(G)gk[i]),F(ng,_H(ky)[i]=(H)gk[i]),F(ng,_I(ky)[i]=(I)gk[i]),F(ng,_L(ky)[i]=gk[i]))
 A fr=aV(tI,ng,gf);A vl;
 S(code,
  C(GA_SUM,I(vf,vl=aV(tF,ng,af))E(vl=aV(tL,ng,al_)))
  C(GA_AVG,vl=an(ng,tF);F(ng,_F(vl)[i]=af[i]/(F)gc[i]))
  C(GA_MIN,I(vf,vl=aV(tF,ng,af))E(vl=aV(tL,ng,al_)))
  C(GA_MAX,I(vf,vl=aV(tF,ng,af))E(vl=aV(tL,ng,al_)))
  C(GA_CNT,vl=aV(tL,ng,gc))
  D(vl=i1(v,_R(fr))))
 free(gk);free(gf);free(af);free(al_);free(gc);free(slot);free(ht);I(mb_,mr(mb_))
 P(!vl,mr(ky);mr(fr);x(0))
 x(aV(tA,3,A(ky,vl,fr))))
#undef GA_GROW
#undef GA_ADD
