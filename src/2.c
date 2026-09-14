#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include"simd.h"
// amber item 3: the element-wise kernels below now call src/simd.c instead of
// re-implementing the loop. They keep their (a,b,c,GROUPS) signature -- every
// call site passes a group count, and each group is 32/sizeof(T) elements --
// so the number of elements written, including the padding past n that oZZ()
// then reads for its overflow check, is bit-identical to the old code.
#define AMEL(g,T) ((U)(g)*(U)(32/SZ(T)))
#define AMCMPN 2000000u/*element count above which the direct float compare wins*/
ZN V aFF(CO V*RES a,CO V*RES b,V*RES c,U n){simd_add_f64(AL(a),AL(b),AL(c),AMEL(n,F));}
ZN V sFF(CO V*RES a,CO V*RES b,V*RES c,U n){simd_sub_f64(AL(a),AL(b),AL(c),AMEL(n,F));}
ZN V mFF(CO V*RES a,CO V*RES b,V*RES c,U n){simd_mul_f64(AL(a),AL(b),AL(c),AMEL(n,F));}
ZN V dFF(CO V*RES a,CO V*RES b,V*RES c,U n){simd_div_f64(AL(a),AL(b),AL(c),AMEL(n,F));}
// amber: the INTEGER add kernels wrap on purpose -- oZZ()/ozZ() below detect
// the overflow after the fact from the result's sign bits and the caller
// re-runs one width wider. Signed overflow is undefined behaviour in C
// though, so the optimiser is entitled to assume it never happens and delete
// the very check that depends on it (UBSan flags this on examples/graphs.k).
// Doing the addition in the unsigned counterpart type is defined two's
// complement wraparound and compiles to the identical instruction.
ZN A amdFF(A x,A y,U f)_(U n=xn;P(n-yn,el(y))A z=MINE(y)?y:aF(n);G(&aFF,sFF,mFF,dFF)[f-1](xV,yV,zV,n+3>>2);y-z?y(z):z)
// amber 2.1: the overflow test is fused into the kernel (simd_addc_*: one pass
// over x, y and z instead of the add plus a second read of all three), and the
// integer multiply is a vectorisable flag-accumulating loop instead of a scalar
// loop with a per-element break. On overflow the result is discarded and the
// operation redone one width wider, exactly as before.
// In place when y is a dying temporary (MINE) of the result width: the sum is
// written over y. Should the width overflow, y is recovered EXACTLY from the
// wrapped result (y = z - x, modular) before the operation is redone wider.
Z A addZZ(A x,A y,U f)_(U w=MAX(xw-3,yw-3);P(xw-3-w,x=ct(tG+w,xR);x(addZZ(x,y,f)))y=ct(tG+w,y);U n=yn;A z=MINE(y)?y:an(n,yt);I ov=0;
 S4(w,ov=simd_addc_i8(xV,yV,zV,n),ov=simd_addc_i16(xV,yV,zV,n),ov=simd_addc_i32(xV,yV,zV,n),simd_add_i64(xV,yV,zV,n))
 I(z==y,_at(z)=0)
 P(ov,I(z==y,S4(w,simd_sub_i8(zV,xV,yV,n),simd_sub_i16(zV,xV,yV,n),simd_sub_i32(zV,xV,yV,n),))E(z(0))y=ct(tG+w+1,y);x=ct(tG+w+1,xR);x(addZZ(x,y,f)))z==y?z:y(z))
Z A subZZ(A x,A y,U f)_(U w=MAX(xw-3,yw-3);P(xw-3-w,x=ct(tG+w,xR);x(subZZ(x,y,f)))y=ct(tG+w,y);U n=yn;A z=MINE(y)?y:an(n,yt);I ov=0;
 S4(w,ov=simd_subc_i8(xV,yV,zV,n),ov=simd_subc_i16(xV,yV,zV,n),ov=simd_subc_i32(xV,yV,zV,n),simd_sub_i64(xV,yV,zV,n))
 I(z==y,_at(z)=0)
 P(ov,I(z==y,S4(w,simd_sub_i8(xV,zV,yV,n),simd_sub_i16(xV,zV,yV,n),simd_sub_i32(xV,zV,yV,n),))E(z(0))y=ct(tG+w+1,y);x=ct(tG+w+1,xR);x(subZZ(x,y,f)))z==y?z:y(z))
Z A mulZZ(A x,A y,U f)_(U n=yn,w=MAX(xw-3,yw-3);P(xw-3-w,x=ct(tG+w,xR);x(mulZZ(x,y,f)))y=ct(tG+w,y);A z=an(n,yt);I ov=0;
 S4(w,ov=simd_mulc_i8(xV,yV,zV,n),ov=simd_mulc_i16(xV,yV,zV,n),ov=simd_mulc_i32(xV,yV,zV,n),simd_mul_i64(xV,yV,zV,n))
 P(ov,z(0);x=ct(tG+w+1,xR);x(mulZZ(x,ct(tG+w+1,y),f)))y(z))

Z A addzZ(L v,A y,U f)_(U n=yn,w=MAX(tZ(v)-tG,yw-3);y=ct(tG+w,y);A z=MINE(y)?y:an(n,yt);I ov=0;
 S4(w,ov=simd_addsc_i8(v,yV,zV,n),ov=simd_addsc_i16(v,yV,zV,n),ov=simd_addsc_i32(v,yV,zV,n),simd_adds_i64(v,yV,zV,n))
 I(z==y,_at(z)=0)
 P(ov,I(z==y,S4(w,simd_addsc_i8(-(G)v,zV,yV,n),simd_addsc_i16(-(H)v,zV,yV,n),simd_addsc_i32(-(I)v,zV,yV,n),))E(z(0))y=ct(tG+w+1,y);addzZ(v,y,f))z==y?z:y(z))
Z A subzZ(L v,A y,U f)_(U n=yn,w=MAX(tZ(v)-tG,yw-3);y=ct(tG+w,y);A z=MINE(y)?y:an(n,yt);I ov=0;/* v - y */
 S4(w,ov=simd_subsc_i8(v,yV,zV,n),ov=simd_subsc_i16(v,yV,zV,n),ov=simd_subsc_i32(v,yV,zV,n),simd_subs_i64(v,yV,zV,n))
 I(z==y,_at(z)=0)
 P(ov,I(z==y,S4(w,simd_subsc_i8(v,zV,yV,n),simd_subsc_i16(v,zV,yV,n),simd_subsc_i32(v,zV,yV,n),))E(z(0))y=ct(tG+w+1,y);subzZ(v,y,f))z==y?z:y(z))

Z A mulzZ(L a,A y,U f)_(U n=yn,w=MAX(tZ(a)-tG,yw-3);y=ct(tG+w,y);A z=an(n,yt);I ov=0;
 S4(w,ov=simd_mulsc_i8(a,yV,zV,n),ov=simd_mulsc_i16(a,yV,zV,n),ov=simd_mulsc_i32(a,yV,zV,n),{CO L*RES p=yV;L*RES r=zV;F(n,r[i]=(L)((W)a*(W)p[i]))})
 ov?mulzZ(a,ct(tG+w+1,z(y)),f):y(z))

#define AMMOD(TY,AT) {CO TY*RES p=yV;AT mm=(AT)m;S4(zw-3,F(zn,{AT r=(AT)p[i]%mm;zg=(G)(r<0?r+mm:r);}),F(zn,{AT r=(AT)p[i]%mm;zh=(H)(r<0?r+mm:r);}),F(zn,{AT r=(AT)p[i]%mm;zi=(I)(r<0?r+mm:r);}),F(zn,{AT r=(AT)p[i]%mm;zl=(L)(r<0?r+mm:r);}))}
Z A modzZ(L m,A y,U f)_(P(!m,y)
 P(m<0,m=-m;A z=an(yn,yt);S4(yw-3,F(zn,C v=yg;zg=v<0?-1-~v/m:v/m),F(zn,H v=yh;zh=v<0?-1-~v/m:v/m),F(zn,I v=yi;zi=v<0?-1-~v/m:v/m),F(zn,L v=yl;zl=v<0?-1-~v/m:v/m))y(z))
 // amber 2.1: a general modulus used to read every element through iw() -- a
 // function call and two 64-bit divisions per element. Width-switched plain
 // loops with one division and a sign fix-up; 32-bit arithmetic when both the
 // data and the modulus fit it (the 32-bit idiv is several times cheaper).
 P(m&m-1,A z=an(yn,tZ(m));U wy=yw-3;
  I(wy<3&&m<=0x7fffffffll,S4(wy,AMMOD(G,I),AMMOD(H,I),AMMOD(I,I),))E(S4(wy,AMMOD(G,L),AMMOD(H,L),AMMOD(I,L),AMMOD(L,L)))
  y(z))
 m--;U t=tZ(m),w=t-tG;y=mut(N(ct(t,y)));F(3-w,m|=m<<(8<<w+i))L*p=yV;F((yn<<w)+31>>5,Fj(4,*p++&=m))y)
Z A modzf(L n,A y,U f)_(P(!n,y)P(n<0,en(y))K2("{y-x*(-x)!_y}",az(n),y))
Z A mmmzZ(L v,A y,U f)_(C t=tZ(v),u=tG+yw-3;I(u<t||u-yt,y=ct(t,y))E(t=u)U n=yn;A z=MINE(y)?y:an(n,t);C w=t-tG;n+=31>>w;L m=-(f==7);v^=m;
 S4(w,F(n&~31,zg=m^MIN(v,m^yg)),F(n&~15,zh=m^MIN(v,m^yh)),F(n&~7,zi=m^MIN(v,m^yi)),F(n&~3,zl=m^MIN(v,m^yl)))y-z?y(z):z)
Z A mmmZZ(A x,A y,U f)_(C w=xw-3;P(w<yw-3,x=ct(tG+yw-3,xR);x(mmmZZ(x,y,f)))y=ct(tG+w,y);U n=yn;A z=MINE(y)?y:an(n,tG+w);n+=31>>w;L m=-(f==7);
 S4(w,F(n&~31,zg=m^MIN(m^xg,m^yg)),F(n&~15,zh=m^MIN(m^xh,m^yh)),F(n&~7,zi=m^MIN(m^xi,m^yi)),F(n&~3,zl=m^MIN(m^xl,m^yl)))y-z?y(z):z)

TD G G4[4],G8[8],G16[16],G32[32];TD H H16[16];TD I I8[8];TD L L4[4];
ZN V ltng(L v,CO V*RES a,V*RES b,U n){G w=v;CO G32*p=a;G32*r=b;F(n+31>>5,Fj(32,r[i][j]=w< p[i][j]))}
ZN V ltnh(L v,CO V*RES a,V*RES b,U n){H w=v;CO H16*p=a;G16*r=b;F(n+15>>4,Fj(16,r[i][j]=w< p[i][j]))}
ZN V ltni(L v,CO V*RES a,V*RES b,U n){I w=v;CO I8 *p=a;G8 *r=b;F(n+ 7>>3,Fj( 8,r[i][j]=w< p[i][j]))}
ZN V ltnl(L v,CO V*RES a,V*RES b,U n){L w=v;CO L4 *p=a;G4 *r=b;F(n+ 3>>2,Fj( 4,r[i][j]=w< p[i][j]))}
ZN V gtng(L v,CO V*RES a,V*RES b,U n){G w=v;CO G32*p=a;G32*r=b;F(n+31>>5,Fj(32,r[i][j]=w> p[i][j]))}
ZN V gtnh(L v,CO V*RES a,V*RES b,U n){H w=v;CO H16*p=a;G16*r=b;F(n+15>>4,Fj(16,r[i][j]=w> p[i][j]))}
ZN V gtni(L v,CO V*RES a,V*RES b,U n){I w=v;CO I8 *p=a;G8 *r=b;F(n+ 7>>3,Fj( 8,r[i][j]=w> p[i][j]))}
ZN V gtnl(L v,CO V*RES a,V*RES b,U n){L w=v;CO L4 *p=a;G4 *r=b;F(n+ 3>>2,Fj( 4,r[i][j]=w> p[i][j]))}
ZN V eqlg(L v,CO V*RES a,V*RES b,U n){G w=v;CO G32*p=a;G32*r=b;F(n+31>>5,Fj(32,r[i][j]=w==p[i][j]))}
ZN V eqlh(L v,CO V*RES a,V*RES b,U n){H w=v;CO H16*p=a;G16*r=b;F(n+15>>4,Fj(16,r[i][j]=w==p[i][j]))}
ZN V eqli(L v,CO V*RES a,V*RES b,U n){I w=v;CO I8 *p=a;G8 *r=b;F(n+ 7>>3,Fj( 8,r[i][j]=w==p[i][j]))}
ZN V eqll(L v,CO V*RES a,V*RES b,U n){L w=v;CO L4 *p=a;G4 *r=b;F(n+ 3>>2,Fj( 4,r[i][j]=w==p[i][j]))}
ZN V ltnG(CO V*RES a,CO V*RES b,V*RES c,U n){CO G32*p=a,*q=b;G32*r=c;F(n+31>>5,Fj(32,r[i][j]=p[i][j]< q[i][j]))}
ZN V ltnH(CO V*RES a,CO V*RES b,V*RES c,U n){CO H16*p=a,*q=b;G16*r=c;F(n+15>>4,Fj(16,r[i][j]=p[i][j]< q[i][j]))}
ZN V ltnI(CO V*RES a,CO V*RES b,V*RES c,U n){CO I8 *p=a,*q=b;G8 *r=c;F(n+ 7>>3,Fj( 8,r[i][j]=p[i][j]< q[i][j]))}
ZN V ltnL(CO V*RES a,CO V*RES b,V*RES c,U n){CO L4 *p=a,*q=b;G4 *r=c;F(n+ 3>>2,Fj( 4,r[i][j]=p[i][j]< q[i][j]))}
ZN V eqlG(CO V*RES a,CO V*RES b,V*RES c,U n){CO G32*p=a,*q=b;G32*r=c;F(n+31>>5,Fj(32,r[i][j]=p[i][j]==q[i][j]))}
ZN V eqlH(CO V*RES a,CO V*RES b,V*RES c,U n){CO H16*p=a,*q=b;G16*r=c;F(n+15>>4,Fj(16,r[i][j]=p[i][j]==q[i][j]))}
ZN V eqlI(CO V*RES a,CO V*RES b,V*RES c,U n){CO I8 *p=a,*q=b;G8 *r=c;F(n+ 7>>3,Fj( 8,r[i][j]=p[i][j]==q[i][j]))}
ZN V eqlL(CO V*RES a,CO V*RES b,V*RES c,U n){CO L4 *p=a,*q=b;G4 *r=c;F(n+ 3>>2,Fj( 4,r[i][j]=p[i][j]==q[i][j]))}
Z A cmpZZ(A x,A y,U f)_(U w=xw-3;P(w<yw-3,x=ct(tG+yw-3,xR);x(cmpZZ(x,y,f)))I(yw-3<w,y=ct(tG+w,y))V*a=xV,*b=yV;I(f==9,SW(a,b))
 U n=xn;A z=aG(n);My(A(&ltnG,ltnH,ltnI,ltnL,eqlG,eqlH,eqlI,eqlL)[(f==10)<<2|w](a,b,zV,n))z)
Z A cmpzZ(L v,A y,U f)_(U w=yw-3;P(tG+w<tZ(v),y(rsz(yn,ai(f==8?v<0:f==9?v>0:0))))
 U n=yn;A z=aG(n);My(A(&ltng,ltnh,ltni,ltnl,gtng,gtnh,gtni,gtnl,eqlg,eqlh,eqli,eqll)[f-8<<2|w](v,yV,zG,n))z)

Z A addzE(L v,A x)_(Lij x(0);aE(i+v,j+v))
Z A addfF(F v,A y,U f)_(A z=MINE(y)?y:aF(yn);U n=zn+3&-4;SIMD F(n,zf=v+yf)y-z?y(z):z)
Z A mulfF(F v,A y,U f)_(A z=MINE(y)?y:aF(yn);U n=zn+3&-4;SIMD F(n,zf=v*yf)y-z?y(z):z)
Z A subfF(F v,A y,U f)_(A z=MINE(y)?y:aF(yn);simd_subs_f64(v,yV,zV,yn);y-z?y(z):z)/* v - y */
Z A admfF(F v,A y,U f)_((f==3?mulfF:f==2?subfF:addfF)(v,y,f))
Z A dvdfF(F v,A y,U f)_(A z=MINE(y)?y:aF(yn);U n=zn+3&-4;SIMD F(n,zf=v/yf)y-z?y(z):z)
Z A dvdFf(A x,F v,U f)_(A z=aF(xn);SIMD F(xn,zf=xf/v)z)
Z A dvdzZ(L v,A y,U f)_(dvdfF(v,cF(y),f))
Z A dvdZZ(A x,A y,U f)_(x=cF(xR);x(amdFF(x,cF(y),f)))
// amber: scalar-scalar fallback arithmetic. Every step that can involve the
// long null (0N == LLONG_MIN) is done in the unsigned counterpart type:
// `-a`, `a+b`, `a*b` and the modulo fix-up all overflow signed range for
// extreme operands (found by tests/fuzz.py under UBSan, e.g. `2!-0w`), which
// is undefined behaviour. Same bits, defined semantics -- Amber's integer
// arithmetic has always been documented as wrapping, not trapping.
#define NEGW(v) ((L)(0-(W)(v)))
Z A arizz(L a,L b,U f)_(P(f==4,af((F)a/b))
 az(f==1?(L)((W)a+(W)b)
   :f==2?(L)((W)a-(W)b)
   :f==3?(L)((W)a*(W)b)
   :f==5?(!a?b:a<0?(b<0?(L)((W)-1-(W)(~b/NEGW(a))):b/NEGW(a)):(L)(((W)((L)((W)(b%a)+(W)a)%a))))
   :f==6?MIN(a,b):f==7?MAX(a,b):f==8?a<b:f==9?a>b:f==10?a==b:0))
Z A arizZ(L v,A y,U f)_(A(&addzZ,subzZ,mulzZ,dvdzZ,modzZ,mmmzZ,mmmzZ,cmpzZ,cmpzZ,cmpzZ)[f-1](v,y,f))
Z A ariZZ(A x,A y,U f)_(P(xn-yn,el(y))A(&addZZ,subZZ,mulZZ,dvdZZ,0,mmmZZ,mmmZZ,cmpZZ,cmpZZ,cmpZZ)[f-1](x,y,f))
ZN A ariz(A x,A y,U f){S(xtT<<1|ytT,R(0,arizz(gl_(x),gl(y),f))R(1,arizZ(gl_(x),y,f))R(2,P(f==4,ari(x,cF(y)))P(f==2,arizZ((L)(0-(W)gl(y)),xR,1))arizZ(gl(y),xR,f-8<2u?f^8^9:f))R_(ariZZ(x,y,f)))}
ZN A arif(A x,A y,U f)_(C t=xt,u=yt;
 P(f==5,xtz?modzf(gl(x),y,f):et(y))
 P(t-tf&&t-tF,x=Ny(cF(xR));x(ari(x,y)))
 P(u-tf&&u-tF,ari(x,N(cF(y))))
 P(f<5,U k=(t<tM)<<1|(u<tM);S(k,
  R(0,F a=*xF,b=gf(y);af(f==1?a+b:f==2?a-b:f==3?a*b:a/b))
  R(1,f<4?admfF(*xF,y,f):dvdfF(*xF,y,f))
  R(2,f==2?admfF(-gf(y),xR,1):f<4?admfF(gf(y),xR,f):dvdFf(x,gf(y),f))
  R_(amdFF(x,y,f)))0)
 P(f==10,ariz(x,y,f))
 // amber item 4: direct IEEE comparison instead of the of1() integer-domain
 // round trip. Bails (and falls through to the unchanged path below) the
 // moment any operand is a NaN or a negative zero -- the only two classes
 // where the two orderings disagree. See simd.h for the argument.
 // SIZE GATE. The of1() path materialises an 8n-byte integer copy of the
 // vector, so it is only worth avoiding once that copy stops fitting in cache.
 // Measured, interleaved, at 1M elements (8 MB, L3-resident) the direct kernel
 // is 0.84x -- SLOWER -- because the of1() copy is nearly free there and the
 // byte-per-element output loop vectorises worse than the integer compare the
 // old path uses. At 10M (80 MB) it is 1.39x. AMCMPN sits between the two.
 I(f>7&&f<10&&(xn>=AMCMPN||yn>=AMCMPN),{
   B vv=xtF&&ytF&&xn==yn,sv=xtF&&ytf,vs=xtf&&ytF;
   I(vv||sv||vs,{
     U nc_=vv||sv?xn:yn;
     A z_=aG(nc_);int bad_=0;
     // `>` is op 1 and `<` is op 0; a scalar on the LEFT flips the sense.
     I(vv,simd_cmpv_f64(xV,yV,_V(z_),nc_,f==9,&bad_))
     J(sv,simd_cmps_f64(xV,*(CO F*)yV,_V(z_),nc_,f==9,&bad_))
     E(   simd_cmps_f64(yV,*(CO F*)xV,_V(z_),nc_,f==8,&bad_))
     I(!bad_,{y(0);return z_;})/*arif owns y, NOT x: the general path below does of1(xR) but of1(y)*/
     mr(z_);})})
 x=of1(xR);y=ari(x,of1(y));x(f<8&&y?of0(y):y))

// Thread-local: `f` is ari()'s implicit op-selector, set-then-read (and
// save/restored across nested arith) within a single top-level call. Each peach
// worker evaluates arithmetic concurrently, so this per-call register must be
// per-thread or one worker's op would leak into another's nested ari().
Z AM_TLS_IE U f;//0=dex,1=add,2=sub,3=mul,4=dvd,5=mod,6=mnm,7=mxm,8=ltn,9=gtn,10=eql
// amber: native temporal scalar arithmetic.  date/time/timestamp atoms carry
// their base units (days/ms/ns); + and - keep the temporal type, temporal-minus-
// same-temporal yields a plain int (a difference), comparisons yield bool.
Z L tval(A x)_(UC k=_t(x);k==tdt||k==ttm?(L)(I)x:k==tnp?*(L*)_V(x):gl_(x))
Z A tmk(UC k,L w)_(k==tdt?adt((I)w):k==ttm?atm((I)w):k==tnp?antp(w):az(w))
// v2 convention: consume y, leave x for the caller (bv opcode releases x; bV borrows a constant x).
Z A tari(A x,A y,U op)_(UC ka=_t(x),kb=_t(y);B qa=ka>=tdt,qb=kb>=tdt;L va=tval(x),vb=tval(y);mr(y);
 P(op>=8,ai((I)(op==8?va<vb:op==9?va>vb:va==vb)))
 L vv=op==1?va+vb:op==2?va-vb:op==3?va*vb:op==6?MIN(va,vb):op==7?MAX(va,vb):va;
 UC rk=(qa&&qb)?(op==2?0:ka):(qa?ka:kb);
 tmk(rk,vv))
A2(ari,C t=xt,u=yt;U v=1<<t|1<<u;
 P(t>=tdt||u>=tdt,tari(x,y,f))
 P(!(v&~(1<<tG|1<<tH|1<<tI|1<<tL|1<<tC|1<<ti|1<<tl|1<<tc)),ariz(x,y,f))
 P(v&(1<<tm|1<<tM|1<<tA),e2(av+f,x,y))
 P(t==tB,x=cG(xR);x(ari(x,y)))
 P(u==tB,ari(x,cG(y)))
 P(t==tE,P(f==1&&ytzc,addzE(gl(y),xR))P(f==2&&ytzc,addzE((L)(0-(W)gl(y)),xR))x=gZ(xR);x(ari(x,y)))
 P(u==tE,P(f==1&&xtzc,addzE(gl_(x),y))ari(x,gZ(y)))
 P(v&(1<<tf|1<<tF),arif(x,y,f))
 I(f-8<3u,
  P(v&1<<tS,P(f==10&&!(v&~(1<<ts|1<<tS)),ariz(x,y,f))e2f(ari,x,y))
  P(v&1<<ts,v^1<<ts?et(y):ai(f==8?qA(x,y)<0:f==9?qA(x,y)>0:xv==yv)))
 et(y))

#define M(s,i) A2(s,U o=f;f=i;x=ari(x,y);f=o;x)
 M(add,1)M(mul,3)M(dvd,4)M(mod,5)M(mnm,6)M(mxm,7)M(ltn,8)M(gtn,9)M(eql,10)
#undef M
A2(dex,y)A2(sub,U o=f;f=2;x=ari(x,y);f=o;x)X2(exc,RMT(ytm||rnk(x)<0?ed(y):ytt?exc(x,rsz(xN,y)):xN-yN?el(y):am(xR,y))Rs(x=rsz(yN,x);x(exc(x,y)))Rilc(/* amber 1.9.3: `!` with a negative integer left argument is the
 * q-family system-verb slot -- -8!x serialises to a byte vector and -9!y
 * deserialises it (src/ser.c). Only -8 and -9, and only on a genuine
 * integer atom, are intercepted; every other left argument (negative ones
 * included) and every char atom still reach mod() exactly as before, so no
 * existing `!` behaviour moves. */
 I(_t(x)-tc,L sv_=gl_(x);I(sv_==-8||sv_==-9,mr(x);return sv_==-8?ser8(y):des9(y)))
 mod(x,y))R_(et(y)))
