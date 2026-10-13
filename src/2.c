#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include"simd.h"
#include"parallel.h"
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
// ---- amber 2.7: element-wise kernels on the thread pool ----------------------------------------------------
// From PEW_MIN elements up, a kernel runs on chunks of whole 32-element blocks, one per thread: every element
// is made by the same machine code as on one thread, so the answer is the same at any thread count. Each
// thread gets at least PEW_MIN/4 elements (fewer threads for a smaller vector).
#define PEW_MIN (1u<<20)
typedef V(*PK3)(CO V*RES,CO V*RES,V*RES,U);   //(a;b;out;count)
typedef V(*PK2)(L,CO V*RES,V*RES,U);           //(number;a;out;count)
typedef V(*PKF)(F,CO F*,F*,U);                 //(float number;a;out;count)
typedef struct{V*f;CO C*a;CO C*b;C*c;L v;F fv;U n,u,sa,sb,sc;int nt,kind;int bad[PAR_MAX_THREADS];}PEW;
Z V pew_w(V*c_,int t){PEW*c=c_;U nb=(c->n+c->u-1)/c->u;U b0=(U)((W)nb*t/c->nt),b1=(U)((W)nb*(t+1)/c->nt);
 U s=b0*c->u,e=b1*c->u;if(e>c->n)e=c->n;if(e<=s)return;U m=e-s;
 switch(c->kind){
  case 0:((PK3)c->f)(c->a+(N)s*c->sa,c->b+(N)s*c->sb,c->c+(N)s*c->sc,m);break;
  case 1:((PK2)c->f)(c->v,c->a+(N)s*c->sa,c->c+(N)s*c->sc,m);break;
  case 2:((PKF)c->f)(c->fv,(CO F*)(c->a+(N)s*8),(F*)(c->c+(N)s*8),m);break;
  case 3:simd_cmpv_f64((CO F*)(c->a+(N)s*8),(CO F*)(c->b+(N)s*8),(UC*)c->c+s,m,(int)c->v,&c->bad[t]);break;
  case 4:simd_cmps_f64((CO F*)(c->a+(N)s*8),c->fv,(UC*)c->c+s,m,(int)c->v,&c->bad[t]);break;
  case 5:simd_subs_f64(c->fv,(CO F*)(c->a+(N)s*8),(F*)(c->c+(N)s*8),m);break;
  case 6:((V(*)(F,CO F*,G*,U))c->f)(c->fv,(CO F*)(c->a+(N)s*8),(G*)c->c+s,m);break;}}
Z int pew_nt(U el){if(el<PEW_MIN)return 1;int nt=par_thread_count(el);U cap=el/(PEW_MIN/4);return nt<(int)cap?nt:(int)cap;}
Z V pew_go(PEW*j){par_run(j->nt,pew_w,j);}
Z V pk3(PK3 f,CO V*a,CO V*b,V*c,U n,U u,U sa,U sb,U sc,U el){int nt=pew_nt(el);if(nt<2){f(a,b,c,n);return;}
 PEW j={0};j.f=(V*)f;j.a=a;j.b=b;j.c=c;j.n=n;j.u=u;j.sa=sa;j.sb=sb;j.sc=sc;j.nt=nt;j.kind=0;pew_go(&j);}
Z V pk2(PK2 f,L v,CO V*a,V*c,U n,U u,U sa,U sc,U el){int nt=pew_nt(el);if(nt<2){f(v,a,c,n);return;}
 PEW j={0};j.f=(V*)f;j.v=v;j.a=a;j.c=c;j.n=n;j.u=u;j.sa=sa;j.sc=sc;j.nt=nt;j.kind=1;pew_go(&j);}
Z V pkf(PKF f,F v,CO F*a,F*c,U n){int nt=pew_nt(n);if(nt<2){f(v,a,c,n);return;}
 PEW j={0};j.f=(V*)f;j.fv=v;j.a=(CO C*)a;j.c=(C*)c;j.n=n;j.u=32;j.nt=nt;j.kind=2;pew_go(&j);}
Z V psubs(F v,CO V*a,V*c,U n){int nt=pew_nt(n);if(nt<2){simd_subs_f64(v,a,c,n);return;}
 PEW j={0};j.fv=v;j.a=a;j.c=c;j.n=n;j.u=32;j.nt=nt;j.kind=5;pew_go(&j);}
Z V pkfg(V(*f)(F,CO F*,G*,U),F v,CO F*a,G*c,U n){int nt=pew_nt(n);if(nt<2){f(v,a,c,n);return;}
 PEW j={0};j.f=(V*)f;j.fv=v;j.a=(CO C*)a;j.c=(C*)c;j.n=n;j.u=32;j.nt=nt;j.kind=6;pew_go(&j);}
Z V pcmpv(CO V*a,CO V*b,V*c,U n,int op,int*bad){int nt=pew_nt(n);if(nt<2){simd_cmpv_f64(a,b,c,n,op,bad);return;}
 PEW j={0};j.a=a;j.b=b;j.c=c;j.n=n;j.u=32;j.v=op;j.nt=nt;j.kind=3;pew_go(&j);F(nt,*bad|=j.bad[i])}
Z V pcmps(CO V*a,F v,V*c,U n,int op,int*bad){int nt=pew_nt(n);if(nt<2){simd_cmps_f64(a,v,c,n,op,bad);return;}
 PEW j={0};j.a=a;j.fv=v;j.c=c;j.n=n;j.u=32;j.v=op;j.nt=nt;j.kind=4;pew_go(&j);F(nt,*bad|=j.bad[i])}
ZN A amdFF(A x,A y,U f)_(U n=xn;P(n-yn,el(y))A z=MINE(y)?y:aF(n);_at(z)=0;pk3(G(&aFF,sFF,mFF,dFF)[f-1],xV,yV,zV,n+3>>2,8,32,32,32,n);y-z?y(z):z)
// amber 2.1: the overflow test is fused into the kernel (simd_addc_*: one pass
// over x, y and z instead of the add plus a second read of all three), and the
// integer multiply is a vectorisable flag-accumulating loop instead of a scalar
// loop with a per-element break. On overflow the result is discarded and the
// operation redone one width wider, exactly as before.
// In place when y is a dying temporary (MINE) of the result width: the sum is
// written over y. Should the width overflow, y is recovered EXACTLY from the
// wrapped result (y = z - x, modular) before the operation is redone wider.
//Amber 2.9.0: 64-bit ints as q's: a null (0N) in, a null out, for + - * (narrower ints hold no null). One pass, the test
//in the loop (the loop waits on memory; the compiler vectorises the select)
Z V addN(CO L*RES a,CO L*RES b,L*RES o,N n){F(n,L p=a[i],q=b[i];o[i]=p==NL|q==NL?NL:(L)((W)p+(W)q))}
Z V subN(CO L*RES a,CO L*RES b,L*RES o,N n){F(n,L p=a[i],q=b[i];o[i]=p==NL|q==NL?NL:(L)((W)p-(W)q))}
Z V mulN(CO L*RES a,CO L*RES b,L*RES o,N n){F(n,L p=a[i],q=b[i];o[i]=p==NL|q==NL?NL:(L)((W)p*(W)q))}
Z V addsN(L v,CO L*RES b,L*RES o,N n){I(v==NL,F(n,o[i]=NL)return;)F(n,L q=b[i];o[i]=q==NL?NL:(L)((W)v+(W)q))}
Z V subsN(L v,CO L*RES b,L*RES o,N n){I(v==NL,F(n,o[i]=NL)return;)F(n,L q=b[i];o[i]=q==NL?NL:(L)((W)v-(W)q))}
Z V mulsN(L v,CO L*RES b,L*RES o,N n){I(v==NL,F(n,o[i]=NL)return;)F(n,L q=b[i];o[i]=q==NL?NL:(L)((W)v*(W)q))}
Z A addZZ(A x,A y,U f)_(U w=MAX(xw-3,yw-3);P(xw-3-w,x=ct(tG+w,xR);x(addZZ(x,y,f)))y=ct(tG+w,y);U n=yn;A z=MINE(y)?y:an(n,yt);I ov=0;
 S4(w,ov=simd_addc_i8(xV,yV,zV,n),ov=simd_addc_i16(xV,yV,zV,n),ov=simd_addc_i32(xV,yV,zV,n),addN(xV,yV,zV,n))
 I(z==y,_at(z)=0)
 P(ov,I(z==y,S4(w,simd_sub_i8(zV,xV,yV,n),simd_sub_i16(zV,xV,yV,n),simd_sub_i32(zV,xV,yV,n),))E(z(0))y=ct(tG+w+1,y);x=ct(tG+w+1,xR);x(addZZ(x,y,f)))z==y?z:y(z))
Z A subZZ(A x,A y,U f)_(U w=MAX(xw-3,yw-3);P(xw-3-w,x=ct(tG+w,xR);x(subZZ(x,y,f)))y=ct(tG+w,y);U n=yn;A z=MINE(y)?y:an(n,yt);I ov=0;
 S4(w,ov=simd_subc_i8(xV,yV,zV,n),ov=simd_subc_i16(xV,yV,zV,n),ov=simd_subc_i32(xV,yV,zV,n),subN(xV,yV,zV,n))
 I(z==y,_at(z)=0)
 P(ov,I(z==y,S4(w,simd_sub_i8(xV,zV,yV,n),simd_sub_i16(xV,zV,yV,n),simd_sub_i32(xV,zV,yV,n),))E(z(0))y=ct(tG+w+1,y);x=ct(tG+w+1,xR);x(subZZ(x,y,f)))z==y?z:y(z))
Z A mulZZ(A x,A y,U f)_(U n=yn,w=MAX(xw-3,yw-3);P(xw-3-w,x=ct(tG+w,xR);x(mulZZ(x,y,f)))y=ct(tG+w,y);A z=an(n,yt);I ov=0;
 S4(w,ov=simd_mulc_i8(xV,yV,zV,n),ov=simd_mulc_i16(xV,yV,zV,n),ov=simd_mulc_i32(xV,yV,zV,n),mulN(xV,yV,zV,n))
 P(ov,z(0);x=ct(tG+w+1,xR);x(mulZZ(x,ct(tG+w+1,y),f)))y(z))

Z A addzZ(L v,A y,U f)_(U n=yn,w=MAX(tZ(v)-tG,yw-3);y=ct(tG+w,y);A z=MINE(y)?y:an(n,yt);I ov=0;
 S4(w,ov=simd_addsc_i8(v,yV,zV,n),ov=simd_addsc_i16(v,yV,zV,n),ov=simd_addsc_i32(v,yV,zV,n),addsN(v,yV,zV,n))
 I(z==y,_at(z)=0)
 P(ov,I(z==y,S4(w,simd_addsc_i8(-(G)v,zV,yV,n),simd_addsc_i16(-(H)v,zV,yV,n),simd_addsc_i32(-(I)v,zV,yV,n),))E(z(0))y=ct(tG+w+1,y);addzZ(v,y,f))z==y?z:y(z))
Z A subzZ(L v,A y,U f)_(U n=yn,w=MAX(tZ(v)-tG,yw-3);y=ct(tG+w,y);A z=MINE(y)?y:an(n,yt);I ov=0;/* v - y */
 S4(w,ov=simd_subsc_i8(v,yV,zV,n),ov=simd_subsc_i16(v,yV,zV,n),ov=simd_subsc_i32(v,yV,zV,n),subsN(v,yV,zV,n))
 I(z==y,_at(z)=0)
 P(ov,I(z==y,S4(w,simd_subsc_i8(v,zV,yV,n),simd_subsc_i16(v,zV,yV,n),simd_subsc_i32(v,zV,yV,n),))E(z(0))y=ct(tG+w+1,y);subzZ(v,y,f))z==y?z:y(z))

Z A mulzZ(L a,A y,U f)_(U n=yn,w=MAX(tZ(a)-tG,yw-3);y=ct(tG+w,y);A z=an(n,yt);I ov=0;
 S4(w,ov=simd_mulsc_i8(a,yV,zV,n),ov=simd_mulsc_i16(a,yV,zV,n),ov=simd_mulsc_i32(a,yV,zV,n),mulsN(a,yV,zV,n))
 ov?mulzZ(a,ct(tG+w+1,z(y)),f):y(z))

#define AMMOD(TY,AT) {CO TY*RES p=yV;AT mm=(AT)m;S4(zw-3,F(zn,{AT r=(AT)p[i]%mm;zg=(G)(r<0?r+mm:r);}),F(zn,{AT r=(AT)p[i]%mm;zh=(H)(r<0?r+mm:r);}),F(zn,{AT r=(AT)p[i]%mm;zi=(I)(r<0?r+mm:r);}),F(zn,{AT r=(AT)p[i]%mm;zl=(L)(r<0?r+mm:r);}))}
Z A modzZ(L m,A y,U f)_(P(!m,ytC?cG(y):y)
 I(m<0&&ytC,y=cG(y))
 // amber 2.3: a power-of-two divisor floors with an arithmetic shift instead of a divide
 // (shifted in 64 bits: s can be up to 62, past the width of the narrow types)
 P(m<0&&m!=NL&&!(-m&(-m-1)),U s=(U)CTZ((W)-m);A z=an(yn,yt);S4(yw-3,F(zn,zg=(G)((L)yg>>s)),F(zn,zh=(H)((L)yh>>s)),F(zn,zi=(I)((L)yi>>s)),F(zn,zl=yl>>s))y(z))
 P(m<0,m=-m;A z=an(yn,yt);S4(yw-3,F(zn,C v=yg;zg=v<0?-1-~v/m:v/m),F(zn,H v=yh;zh=v<0?-1-~v/m:v/m),F(zn,I v=yi;zi=v<0?-1-~v/m:v/m),F(zn,L v=yl;zl=v<0?-1-~v/m:v/m))y(z))
 // amber 2.1: a general modulus used to read every element through iw() -- a
 // function call and two 64-bit divisions per element. Width-switched plain
 // loops with one division and a sign fix-up; 32-bit arithmetic when both the
 // data and the modulus fit it (the 32-bit idiv is several times cheaper).
 P(m&m-1,A z=an(yn,tZ(m));U wy=yw-3;
  I(wy<3&&m<=0x7fffffffll,S4(wy,AMMOD(G,I),AMMOD(H,I),AMMOD(I,I),))E(S4(wy,AMMOD(G,L),AMMOD(H,L),AMMOD(I,L),AMMOD(L,L)))
  y(z))
 m--;U t=tZ(m),w=t-tG;
 // amber 2.4.1: a shared y was copied by mut() and then masked, two passes; mask into a new vector
 P(yt==t&&!MINE(y),A z=an(yn,t);S4(w,F(zn,zg=yg&(G)m),F(zn,zh=yh&(H)m),F(zn,zi=yi&(I)m),F(zn,zl=yl&m))y(z))
 y=mut(N(ct(t,y)));F(3-w,m|=m<<(8<<w+i))L*p=yV;F((yn<<w)+31>>5,Fj(4,*p++&=m))y)
// Digest #15: the float remainder from fmod (exact), then made non-negative. It was {y-x*(-x)!_y}, whose _y
// saturates past 2^63 (7!1e19 was 7.8e17) and makes 0w 0N (5!-0w was -0w; now 0n, as q). -0.0 stays -0.0.
Z F fmz(F v,F m)_(F r=__builtin_fmod(v,m);P(r<0,r+m)W u;MC(&u,&r,8);P(u<<1,r)MC(&u,&v,8);P(!(u<<1),v)0.0)   //An exact multiple: 0.0, -0.0 only from y -0.0 itself (as before); on bits: the build has -fno-signed-zeros
Z A modzf(L n,A y,U f)_(P(!n,y)P(n<0,en(y))F m=(F)n;P(ytf,F v=*yF;y(af(fmz(v,m))))A z=MINE(y)?y:aF(yn);_at(z)=0;CO F*p=yF;F*q=zF;F(yn,q[i]=fmz(p[i],m))y-z?y(z):z)
Z A mmmzZ(L v,A y,U f)_(C t=tZ(v),u=tG+yw-3;I(u<t||u-yt,y=ct(t,y))E(t=u)U n=yn;A z=MINE(y)?y:an(n,t);_at(z)=0;C w=t-tG;n+=31>>w;L m=-(f==7);v^=m;
 S4(w,F(n&~31,zg=m^MIN(v,m^yg)),F(n&~15,zh=m^MIN(v,m^yh)),F(n&~7,zi=m^MIN(v,m^yi)),F(n&~3,zl=m^MIN(v,m^yl)))y-z?y(z):z)
Z A mmmZZ(A x,A y,U f)_(C w=xw-3;P(w<yw-3,x=ct(tG+yw-3,xR);x(mmmZZ(x,y,f)))y=ct(tG+w,y);U n=yn;A z=MINE(y)?y:an(n,tG+w);_at(z)=0;n+=31>>w;L m=-(f==7);
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
 U n=xn;A z=aG(n);My(pk3(A(&ltnG,ltnH,ltnI,ltnL,eqlG,eqlH,eqlI,eqlL)[(f==10)<<2|w],a,b,zV,n,32,1u<<w,1u<<w,1,n))z)
Z A cmpzZ(L v,A y,U f)_(U w=yw-3;P(tG+w<tZ(v),U n=yn;A z=aG(n);MS(zG,f==8?v<0:f==9?v>0:0,n);y(z))   //Amber 2.8.0: a number past the list's width: one
 //value, as a byte mask as any compare gives (it was an int list, which & read as counts: &(n?100)>500 took 0.8 ms, not 0.1)
 U n=yn;A z=aG(n);My(pk2(A(&ltng,ltnh,ltni,ltnl,gtng,gtnh,gtni,gtnl,eqlg,eqlh,eqli,eqll)[f-8<<2|w],v,yV,zG,n,32,1u<<w,1,n))z)

Z A addzE(L v,A x)_(Lij P(v>0?j>WL-v:i<NL-v,addzZ(v,gZ(x),1))x(0);aE(i+v,j+v))   //ends past the int range: a vector (ints wrap), not a wrapped range
// Amber 2.5 (exp): the number-with-vector float loops are functions of their own (noipa: one copy of the
// machine code) so that these primitives and the fusion engine below make every element the same way.
#if defined(__clang__)
#define FZK NI
#else
#define FZK __attribute__((noipa))
#endif
Z FZK V fzadd(F v,CO F*y,F*z,U n){SIMD F(n,z[i]=v+y[i])}
Z FZK V fzmul(F v,CO F*y,F*z,U n){SIMD F(n,z[i]=v*y[i])}
Z FZK V fzdvl(F v,CO F*y,F*z,U n){SIMD F(n,z[i]=v/y[i])}
Z FZK V fzdvr(CO F*x,F v,F*z,U n){SIMD F(n,z[i]=x[i]/v)}
Z A addfF(F v,A y,U f)_(A z=MINE(y)?y:aF(yn);_at(z)=0;pkf(fzadd,v,yF,zF,zn+3&-4);y-z?y(z):z)
Z A mulfF(F v,A y,U f)_(A z=MINE(y)?y:aF(yn);_at(z)=0;pkf(fzmul,v,yF,zF,zn+3&-4);y-z?y(z):z)
Z A subfF(F v,A y,U f)_(A z=MINE(y)?y:aF(yn);_at(z)=0;psubs(v,yV,zV,yn);y-z?y(z):z)/* v - y */
Z A admfF(F v,A y,U f)_((f==3?mulfF:f==2?subfF:addfF)(v,y,f))
Z A dvdfF(F v,A y,U f)_(A z=MINE(y)?y:aF(yn);_at(z)=0;pkf(fzdvl,v,yF,zF,zn+3&-4);y-z?y(z):z)
Z A dvdFf(A x,F v,U f)_(A z=aF(xn);fzdvr(xF,v,zF,xn);z)
// ---- amber 2.5 (exp): fusion --------------------------------------------------------------------------------
// fzrun runs the program the compiler made for an element-wise tree (src/b.c fz): d is (program bytes;
// literals..), l the frame's locals. Leaves are float or int vectors of one length n>=FZ_MIN and numbers;
// + - * % & | anywhere, < > = at the root, maybe +/ on top. Anything else (and an operation on two numbers)
// Returns 0 and the unfused code runs. The tree is evaluated FZ_BK elements at a time, intermediates in small
// buffers that stay in cache, every step as the unfused primitive does it:
//  - floats: the very kernel function the primitive calls, with the operands arif() gives it (a number left
//    of - is the subs kernel, right of it v+x with -v), so every element is the same double;
//  - ints: + - * in int64, the exact value (narrow ops widen on overflow and redo, 64-bit ones wrap, as
//    here); the width the unfused result would have is max(the operands' widths, the narrowest holding every
//    value), found after the pass from each step's min and max; & | are min and max;
//  - an int meeting % or a float becomes a float as cF makes it (0N is 0n);
//  - float & | and < > = go by the order keys of src/o.c (of1/of0): -0.0 is 0.0, NaN is 0n and first;
//  - a number too wide for the int vector it is compared with gives cmpzZ's answer (rsz of 0/1: an I vector).
// Blocks are split over threads. +/ of ints or comparisons sums in 64-bit wrap per thread (exact in any order);
// +/ of floats writes the vector and simd_sum_f64 sums it, the call +/ makes (a copy of its loop could end with
// the other of two NaNs: only the same machine code gives the same bits).
#define FZ_MIN 2048u
#define FZ_BK 512u
#define FZ_PAR (1u<<15)
#define FZ_N 32           //Program steps
TD struct{UC op,k,w,s;}FZS;   //op: 'v' float vector, 'z' int vector (w: width 0..3), 'c' float number, 'i' int number,
                              //Else the operation; k: 0 float, 1 int, 2 comparison; s: 1 for a number
TD struct{FZS p[FZ_N];U np,dp,n,nb,nt;int ab;B sum,i32;UC ow,tr[FZ_N];CO V*lv[FZ_N];F lc[FZ_N];L li[FZ_N];V*out;L*mn,*mx;W*acc;}FZC;
// src/o.c's order key for floats (of1, of0), the same integer steps
Z CO W fzo_=(-1ull>>12)-1;Z L fzt_(L v)_(v^(W)(v>>63)>>1)
Z L fzco(L v)_(W b=(W)v<<1;!b?0:b>0xffe0000000000000ull?NFL:v)
Z L fzk1(F x)_(L v;MC(&v,&x,8);fzt_(fzco(v))+fzo_)
Z F fzk0(L v)_(v=fzt_(v-fzo_);F x;MC(&x,&v,8);x)
// A NaN or a -0.0 among x[0..n): where the keys and IEEE order part
Z B fzbad(CO F*x,U n){W b=0;F(n,W u;MC(&u,x+i,8);b|=(u<<1>0xffe0000000000000ull)|(u==1ull<<63))return b!=0;}
Z B fzbad1(F v)_(fzbad(&v,1))
TD __int128 FZL;
Z U fzfit(FZL lo,FZL hi){U w=0;W(w<3&&(lo<-((FZL)1<<((8<<w)-1))||hi>((FZL)1<<((8<<w)-1))-1),w++)return w;}
// The block evaluator, made for int lanes of type T (L or I; UT its unsigned); DW: the width T stores (3 or 2)
#if defined(__x86_64__) && defined(__ELF__) && defined(__GNUC__) && !defined(wasm) && !defined(__AVX2__)
#define FZMV __attribute__((target_clones("avx2","default")))
#else
#define FZMV
#endif
#define FZI(T,UT,X,Y) S(op,C('+',F(cnt,R[i]=(T)((UT)(X)+(UT)(Y))))C('-',F(cnt,R[i]=(T)((UT)(X)-(UT)(Y))))C('*',F(cnt,R[i]=(T)((UT)(X)*(UT)(Y))))\
 C('&',F(cnt,T u_=X,v_=Y;R[i]=MIN(u_,v_)))C('|',F(cnt,T u_=X,v_=Y;R[i]=MAX(u_,v_))))
#define FZQ(X,Y) S(op,C('<',F(cnt,R[i]=(X)<(Y)))C('>',F(cnt,R[i]=(X)>(Y)))C('=',F(cnt,R[i]=(X)==(Y))))
#define FZM(X,Y) I(op=='&',F(cnt,F u_=X,v_=Y;r[i]=v_<u_?v_:u_))E(F(cnt,F u_=X,v_=Y;r[i]=v_>u_?v_:u_))
#define FZBLK(NAME,T,UT,DW) \
Z FZMV V NAME(CO FZC*c,U o,U cnt,F*bf,L*mn,L*mx,W*acc){CO V*sp[FZ_N];F sf[FZ_N];L sz[FZ_N];UC sk[FZ_N],ss[FZ_N],tb[FZ_BK];U d=0,cr=cnt+3&~3u;\
 for(U j=0;j<c->np;j++){FZS s=c->p[j];F*bs=bf+(N)d*FZ_BK;\
  if(s.op=='v'){sp[d]=(CO F*)c->lv[j]+o;sk[d]=0;ss[d++]=0;continue;}\
  if(s.op=='z'){T*q_=(T*)bs;I(s.w==DW,sp[d]=(CO T*)c->lv[j]+o)\
   E(S(s.w,C(0,CO G*g_=(CO G*)c->lv[j]+o;F(cnt,q_[i]=g_[i]))C(1,CO H*g_=(CO H*)c->lv[j]+o;F(cnt,q_[i]=g_[i]))C(2,CO I*g_=(CO I*)c->lv[j]+o;F(cnt,q_[i]=g_[i])))sp[d]=bs)\
   sk[d]=1;ss[d++]=0;continue;}\
  if(s.op=='c'){sf[d]=c->lc[j];sk[d]=0;ss[d++]=1;continue;}\
  if(s.op=='i'){sz[d]=c->li[j];sk[d]=1;ss[d++]=1;continue;}\
  U a=d-2,e=d-1;d--;B last=j==c->np-1;UC op=s.op;F*ba=bf+(N)a*FZ_BK,*be=bf+(N)e*FZ_BK;\
  if(s.k==1){B dir=last&&!c->sum&&c->ow==DW;T*R=dir?(T*)c->out+o:(T*)ba;\
   I(ss[a],T v=(T)sz[a];CO T*y=sp[e];FZI(T,UT,v,y[i]))J(ss[e],CO T*x=sp[a];T v=(T)sz[e];FZI(T,UT,x[i],v))E(CO T*x=sp[a],*y=sp[e];FZI(T,UT,x[i],y[i]))\
   I(c->tr[j],T lo_=R[0],hi_=R[0];F(cnt,T u_=R[i];lo_=MIN(lo_,u_);hi_=MAX(hi_,u_))mn[j]=MIN(mn[j],(L)lo_);mx[j]=MAX(mx[j],(L)hi_))\
   I(last&&c->sum,W t=0;F(cnt,t+=(W)(L)R[i])*acc+=t)\
   I(last&&!c->sum&&!dir,S(c->ow,C(0,G*q=(G*)c->out+o;F(cnt,q[i]=(G)R[i]))C(1,H*q=(H*)c->out+o;F(cnt,q[i]=(H)R[i]))\
    C(2,I*q=(I*)c->out+o;F(cnt,q[i]=(I)R[i]))C(3,L*q=(L*)c->out+o;F(cnt,q[i]=(L)R[i]))))\
   sp[a]=R;sk[a]=1;ss[a]=0;continue;}\
  if(s.k==2&&sk[a]&&sk[e]){UC*R=c->sum?tb:(UC*)c->out+o;\
   I(ss[a],L v=sz[a];CO T*y=sp[e];FZQ(v,(L)y[i]))J(ss[e],CO T*x=sp[a];L v=sz[e];FZQ((L)x[i],v))E(CO T*x=sp[a],*y=sp[e];FZQ(x[i],y[i]))\
   I(c->sum,W t=0;F(cnt,t+=R[i])*acc+=t)continue;}\
  F va=0,ve=0;CO F*x=0,*y=0;\
  I(ss[a],va=sk[a]?(sz[a]==NL?NF:(F)sz[a]):sf[a])J(sk[a],CO T*z_=sp[a];for(U i_=cr;i_--;){L v_=z_[i_];ba[i_]=v_==NL?NF:(F)v_;}x=ba)E(x=sp[a])\
  I(ss[e],ve=sk[e]?(sz[e]==NL?NF:(F)sz[e]):sf[e])J(sk[e],CO T*z_=sp[e];for(U i_=cr;i_--;){L v_=z_[i_];be[i_]=v_==NL?NF:(F)v_;}y=be)E(y=sp[e])\
  if(s.k==2||op=='&'||op=='|'){\
   B bad=(ss[a]?fzbad1(va):fzbad(x,cnt))||(ss[e]?fzbad1(ve):fzbad(y,cnt));\
   if(s.k==2){UC*R=c->sum?tb:(UC*)c->out+o;\
    I(!bad,I(ss[a],FZQ(va,y[i]))J(ss[e],FZQ(x[i],ve))E(FZQ(x[i],y[i])))\
    E(L ka=ss[a]?fzk1(va):0,ke=ss[e]?fzk1(ve):0;I(ss[a],FZQ(ka,fzk1(y[i])))J(ss[e],FZQ(fzk1(x[i]),ke))E(FZQ(fzk1(x[i]),fzk1(y[i]))))\
    I(c->sum,W t=0;F(cnt,t+=R[i])*acc+=t)continue;}\
   F*r=last?(F*)c->out+o:ba;\
   I(!bad,I(ss[a],FZM(va,y[i]))J(ss[e],FZM(x[i],ve))E(FZM(x[i],y[i])))\
   E(B mx_=op=='|';L ka=ss[a]?fzk1(va):0,ke=ss[e]?fzk1(ve):0;\
     F(cnt,L u_=ss[a]?ka:fzk1(x[i]),v_=ss[e]?ke:fzk1(y[i]);r[i]=fzk0(mx_?MAX(u_,v_):MIN(u_,v_))))\
   sp[a]=r;sk[a]=0;ss[a]=0;continue;}\
  F*r=last?(F*)c->out+o:ba;\
  if(!ss[a]&&!ss[e])S(op,C('+',simd_add_f64(x,y,r,cr))C('-',simd_sub_f64(x,y,r,cr))C('*',simd_mul_f64(x,y,r,cr))C('%',simd_div_f64(x,y,r,cr)))\
  else if(ss[a])S(op,C('+',fzadd(va,y,r,cr))C('-',simd_subs_f64(va,y,r,cnt))C('*',fzmul(va,y,r,cr))C('%',fzdvl(va,y,r,cr)))\
  else S(op,C('+',fzadd(ve,x,r,cr))C('-',fzadd(-ve,x,r,cr))C('*',fzmul(ve,x,r,cr))C('%',fzdvr(x,ve,r,cnt)))\
  sp[a]=r;sk[a]=0;ss[a]=0;}}
FZBLK(fzblk64,L,W,3)
FZBLK(fzblk32,I,U,2)
Z V fzw(V*c_,int t){FZC*c=c_;U b0=(U)((N)c->nb*t/c->nt),b1=(U)((N)c->nb*(t+1)/c->nt);F bf[c->dp*FZ_BK];
 L mn[FZ_N],mx[FZ_N];W acc=0;F(FZ_N,mn[i]=WL;mx[i]=NL)
 U r=c->np-1;B wa=c->p[r].k==1&&!c->sum&&c->tr[r];                     //Watch the root's width
 for(U k=b0;k<b1;k++){U o=k*FZ_BK,m=MIN(FZ_BK,c->n-o);I(wa&&__atomic_load_n(&c->ab,__ATOMIC_RELAXED),break)
  I(c->i32,fzblk32(c,o,m,bf,mn,mx,&acc))E(fzblk64(c,o,m,bf,mn,mx,&acc))
  I(wa&&fzfit(mn[r],mx[r])>c->ow,__atomic_store_n(&c->ab,1,__ATOMIC_RELAXED);break)}
 MC(c->mn+(N)t*FZ_N,mn,SZ mn);MC(c->mx+(N)t*FZ_N,mx,SZ mx);c->acc[t]=acc;}
Z B hnl(A v)_(CO L*RES p=_V(v);L o=0;F(_n(v),o|=p[i]==NL)!!o)   //a 64-bit int list holding a 0N
A fzrun(A d,A*l){A pg=_A(d)[0];CO UC*q=(CO UC*)_V(pg),*e=q+_n(pg);FZC c;c.sum=*q++&1;UC kd[FZ_N],sd[FZ_N];U np=0,dp=0,n=0;
 W(q<e,UC o=*q++;A v=0;
  I(o=='l',v=l[*q++])J(o=='g',v=gv[q[0]|(U)q[1]<<8];q+=2)J(o=='k',v=_A(d)[*q++])
  E(P(dp<2||sd[dp-1]&&sd[dp-2]||np>=FZ_N,0)B cmp=o=='<'||o=='>'||o=='=';P(cmp&&q<e,0)
    UC k=cmp?2:o!='%'&&kd[dp-1]==1&&kd[dp-2]==1?1:0;c.p[np++]=(FZS){o,k,0,0};dp--;kd[dp-1]=k;sd[dp-1]=0;continue)
  P(!v||np>=FZ_N,0)
  I(!_tP(v)&&_T(v)==tF,U m=_n(v);P(n&&m!=n,0)n=m;c.lv[np]=_V(v);c.p[np++]=(FZS){'v',0,0,0};kd[dp]=0;sd[dp++]=0)
  J(!_tP(v)&&LH(tG,_T(v),tL),U m=_n(v);P(n&&m!=n,0)P(_T(v)==tL&&hnl(v),0)n=m;   //a 0N in 64 bits: the unfused dyads, which keep it (2.9.0)
   c.lv[np]=_V(v);c.p[np++]=(FZS){'z',1,(UC)(_T(v)-tG),0};kd[dp]=1;sd[dp++]=0)
  E(UC t=_t(v);P(t!=tf&&t!=ti&&t!=tl,0)
    I(t==tf,c.lc[np]=*_F(v);c.p[np++]=(FZS){'c',0,0,1};kd[dp]=0)E(P(gl_(v)==NL,0)c.li[np]=gl_(v);c.p[np++]=(FZS){'i',1,0,1};kd[dp]=1)sd[dp++]=1))
 P(dp!=1||n<FZ_MIN||sd[0],0)
 c.np=np;c.n=n;c.nb=(n+FZ_BK-1)/FZ_BK;c.dp=0;{U k=0;F(np,I(c.p[i].op=='v'||c.p[i].op=='z'||c.p[i].op=='c'||c.p[i].op=='i',k++)E(k--)c.dp=MAX(c.dp,k))}
 // Before the pass: the bounds of every int step, the least and most width it can end at, and which steps
 // need watching (their bound does not fit the least width their operands can have)
 FZL lo[FZ_N],hi[FZ_N];UC wl[FZ_N],wm[FZ_N];U st[FZ_N],sp_=0;
 F(np,FZS s=c.p[i];c.tr[i]=0;
  I(s.op=='z',L a_=s.w==3?NL:-(1ll<<((8<<s.w)-1)),b_=s.w==3?WL:(1ll<<((8<<s.w)-1))-1;lo[i]=a_;hi[i]=b_;wl[i]=wm[i]=s.w;st[sp_++]=i;continue)
  I(s.op=='i',lo[i]=hi[i]=c.li[i];wl[i]=wm[i]=0;st[sp_++]=i;continue)
  I(s.op=='v'||s.op=='c',st[sp_++]=i;continue)
  U ea=st[sp_-1],aa=st[sp_-2];sp_--;st[sp_-1]=i;
  I(s.k==1,FZS sa=c.p[aa],se=c.p[ea];
   U la=sa.s?tZ(c.li[aa])-tG:wl[aa],le=se.s?tZ(s.op=='-'?(L)(0-(W)c.li[ea]):c.li[ea])-tG:wl[ea];
   U ma=sa.s?la:wm[aa],me=se.s?le:wm[ea];wl[i]=MAX(la,le);wm[i]=MAX(ma,me);
   FZL x0=lo[aa],x1=hi[aa],y0=lo[ea],y1=hi[ea];
   S(s.op,C('+',lo[i]=x0+y0;hi[i]=x1+y1)C('-',lo[i]=x0-y1;hi[i]=x1-y0)
    C('*',FZL p0=x0*y0,p1=x0*y1,p2=x1*y0,p3=x1*y1;lo[i]=MIN(MIN(p0,p1),MIN(p2,p3));hi[i]=MAX(MAX(p0,p1),MAX(p2,p3)))
    C('&',lo[i]=MIN(x0,y0);hi[i]=MIN(x1,y1))C('|',lo[i]=MAX(x0,y0);hi[i]=MAX(x1,y1)))
   I(s.op=='+'||s.op=='-'||s.op=='*',U f=fzfit(lo[i],hi[i]);I(f>wl[i],c.tr[i]=1)wm[i]=MAX(wm[i],f))
   I(lo[i]<NL||hi[i]>WL,lo[i]=NL;hi[i]=WL)))                           //It may wrap (only where a 64-bit op wraps too)
 c.i32=1;F(np,FZS s=c.p[i];I(s.op=='z'&&s.w==3||s.op=='i'&&c.li[i]!=(I)c.li[i]||s.k==1&&s.op!='z'&&s.op!='i'&&(lo[i]<-((FZL)1<<31)||hi[i]>((FZL)1<<31)-1),c.i32=0))
 int nt=n>=FZ_PAR?par_thread_count(n):1;I(nt>(int)c.nb,nt=(int)c.nb)c.nt=(U)nt;
 UC rk=c.p[np-1].k;c.ow=rk==1?wl[np-1]:3;L mnb[PAR_MAX_THREADS*FZ_N],mxb[PAR_MAX_THREADS*FZ_N];W acb[PAR_MAX_THREADS];c.mn=mnb;c.mx=mxb;c.acc=acb;
 A z=0;c.ab=0;
 again:I(!c.sum||rk==0,z=an(n,rk==0?tF:rk==1?tG+c.ow:tG);P(!z,0)_at(z)=0;c.out=_V(z))
 I(nt>1,par_run(nt,fzw,&c))E(fzw(&c,0))
 I(c.ab,mr(z);c.ab=0;c.ow=wm[np-1];goto again)                        //The guess was too narrow: at the widest
 P(c.sum&&rk==0,F r=par_bsum_f64(zF,n);mr(z);af(r))
 P(c.sum,W t=0;F(nt,t+=acb[i])az((L)t))
 // widths: replay the steps; an int step's width is max(its operands', the narrowest holding its values)
 U wd[FZ_N];sp_=0;
 F(np,FZS s=c.p[i];
  I(s.op=='v'||s.op=='z'||s.op=='c'||s.op=='i',wd[i]=s.op=='z'?s.w:0;st[sp_++]=i;continue)
  U ea=st[sp_-1],aa=st[sp_-2];sp_--;st[sp_-1]=i;
  I(s.k==1,FZS sa=c.p[aa],se=c.p[ea];
   U wa=sa.s?tZ(c.li[aa])-tG:wd[aa],we=se.s?tZ(s.op=='-'?(L)(0-(W)c.li[ea]):c.li[ea])-tG:wd[ea];wd[i]=MAX(wa,we);
   I(c.tr[i],L lo_=WL,hi_=NL;F_(t,nt,lo_=MIN(lo_,mnb[t*FZ_N+i]);hi_=MAX(hi_,mxb[t*FZ_N+i]))wd[i]=MAX(wd[i],fzfit(lo_,hi_))))
  // A comparison of an int vector with an int number wider than it: cmpzZ's rsz (the number taken on the left)
  I(s.k==2&&i==np-1,FZS sa=c.p[aa],se=c.p[ea];
   I(sa.k==1&&se.k==1&&(sa.s||se.s),L v=sa.s?c.li[aa]:c.li[ea];U w=sa.s?wd[ea]:wd[aa];U f=s.op=='<'?8:s.op=='>'?9:10;I(se.s&&f<10,f^=1)
    I(tG+w<tZ(v),mr(z);return rsz(n,ai(f==8?v<0:f==9?v>0:0))))))
 I(rk==1&&wd[np-1]>c.ow,mr(z);c.ow=(UC)wd[np-1];goto again)          //Wider than guessed (an operand widened): again at its width
 I(rk==1&&wd[np-1]<c.ow,z=ct(tG+wd[np-1],z))                         //After a rerun at the widest
 return z;}
// The VM's bF (src/b.c): b at its operands j c p, l the frame's locals, k the lambda's constants. Out of line so
// that the dispatch loop's registers are not disturbed; p, a local leaf, is looked at first and unless it holds
// a float or int vector long enough there is nothing more to do (scalar code, short vectors).
NI A fzop(CO UC*b,A*l,A*k){UC p=b[2];I(p<16,A v=l[p];P(!v||_tP(v)||(_T(v)!=tF&&!LH(tG,_T(v),tL))||_n(v)<FZ_MIN,0))return fzrun(k[b[1]],l);}
Z A dvdzZ(L v,A y,U f)_(dvdfF(v==NL?NF:v,cF(y),f))
Z A dvdZZ(A x,A y,U f)_(x=cF(xR);x(amdFF(x,cF(y),f)))
// amber: scalar-scalar fallback arithmetic. Every step that can involve the
// long null (0N == LLONG_MIN) is done in the unsigned counterpart type:
// `-a`, `a+b`, `a*b` and the modulo fix-up all overflow signed range for
// extreme operands (found by tests/fuzz.py under UBSan, e.g. `2!-0w`), which
// is undefined behaviour. Same bits, defined semantics -- Amber's integer
// arithmetic has always been documented as wrapping, not trapping.
#define NEGW(v) ((L)(0-(W)(v)))
Z A arizz(L a,L b,U f)_(P(f==4,af((a==NL?NF:(F)a)/(b==NL?NF:(F)b)))P((f-1<3u||f==5)&&(a==NL||b==NL),az(NL))   //a null gives a null, as q (2.9.0; it wrapped)
 az(f==1?(L)((W)a+(W)b)
   :f==2?(L)((W)a-(W)b)
   :f==3?(L)((W)a*(W)b)
   :f==5?(!a?b:a<0?(b<0?(L)((W)-1-(W)(~b/NEGW(a))):b/NEGW(a)):({L r_=b%a;r_<0?r_+a:r_;}))
   :f==6?MIN(a,b):f==7?MAX(a,b):f==8?a<b:f==9?a>b:f==10?a==b:0))
Z A modzQ(L m,A y,U f)_(U n=yn;P(m==NL,A z=aL(n);F(n,zl=NL)y(z))P(yt-tL||!hnl(y),modzZ(m,y,f))A r=modzZ(m,_R(y),f);P(!r,y(0))r=cL(r);P(!r,y(0))r=mut(r);F(n,I(yl==NL,_L(r)[i]=NL))y(r))   //a 0N keeps its place (2.9.0)
Z A arizZ(L v,A y,U f)_(A(&addzZ,subzZ,mulzZ,dvdzZ,modzQ,mmmzZ,mmmzZ,cmpzZ,cmpzZ,cmpzZ)[f-1](v,y,f))
Z A ariZZ(A x,A y,U f)_(P(xn-yn,el(y))A(&addZZ,subZZ,mulZZ,dvdZZ,0,mmmZZ,mmmZZ,cmpZZ,cmpZZ,cmpZZ)[f-1](x,y,f))
ZN A ariz(A x,A y,U f){S(xtT<<1|ytT,R(0,arizz(gl_(x),gl(y),f))R(1,arizZ(gl_(x),y,f))R(2,P(f==4,ari(x,cF(y)))P(f==2,arizZ((L)(0-(W)gl(y)),xR,1))arizZ(gl(y),xR,f-8<2u?f^8^9:f))R_(ariZZ(x,y,f)))}
// = on floats compares values: -0.0 is 0.0 and every NaN is 0n (as ~ does), while the comparison
// below it is on bit patterns. fzn gives that form, copying only when x has a -0.0 or a NaN.
Z A fzn(A x/*1*/)_(P(!xtf&&!xtF,x)U n=xtf?1:xn;CO F*p=xF;U i=0;W b_=0;   //bits through memcpy: under -fno-signed-zeros (and strict aliasing) GCC folded a -0.0 test on the double away
 W(i<n&&(MC(&b_,p+i,8),b_<<1<=0xffe0000000000000ull&&b_!=1ull<<63),i++)P(i==n,x)
 P(xtf,x(af(b_<<1>0xffe0000000000000ull?NF:0.0)))x=mut(x);W*q=(W*)xF;F(n,W c_;MC(&c_,q+i,8);I(c_<<1>0xffe0000000000000ull,F z_=NF;MC(q+i,&z_,8))J(c_==1ull<<63,c_=0;MC(q+i,&c_,8)))x)
// = of float vectors in one pass: IEEE == already takes -0.0 for 0.0, and the second term
// makes every NaN equal; 0 (nothing consumed) for shapes it does not handle.
#define EQV(u,v) ((u)==(v)|((u)!=(u))&((v)!=(v)))
Z V eqvvk(CO V*RES a,CO V*RES b,V*RES c,U n){CO F*RES p=a,*RES q=b;G*RES r=c;F(n,r[i]=EQV(p[i],q[i]))}
Z V eqsvk(F u,CO F*q,G*r,U n){F(n,r[i]=EQV(u,q[i]))}
Z A eqFF(A x,A y/*00*/)_(B a=xtf,b=ytf;P(a&&b||!a&&!b&&xn-yn,0)U n=a?yn:xn;A z=an(n,tG);G*RES r=zV;CO F*RES p=xF,*RES q=yF;
 P(a,pkfg(eqsvk,*p,q,r,n);z)P(b,pkfg(eqsvk,*q,p,r,n);z)pk3(eqvvk,p,q,r,n,32,8,8,1,n);z)   //2.7: on the thread pool from 1M items
ZN A arif(A x,A y,U f)_(C t=xt,u=yt;
 P(f==5,xtz?modzf(gl_(x),y,f):et(y))
 P(t-tf&&t-tF,x=Ny(cF(xR));x(ari(x,y)))
 P(u-tf&&u-tF,ari(x,N(cF(y))))
 P(f<5,U k=(t<tM)<<1|(u<tM);S(k,
  R(0,F a=*xF,b=gf(y);af(f==1?a+b:f==2?a-b:f==3?a*b:a/b))
  R(1,f<4?admfF(*xF,y,f):dvdfF(*xF,y,f))
  R(2,f==2?admfF(-gf(y),xR,1):f<4?admfF(gf(y),xR,f):dvdFf(x,gf(y),f))
  R_(amdFF(x,y,f)))0)
 A e=f==10&&(t==tF||u==tF)?eqFF(x,y):0;P(e,y(e))
 P(f==10,A u=fzn(xR);u(ariz(u,fzn(y),f)))
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
     I(vv,pcmpv(xV,yV,_V(z_),nc_,f==9,&bad_))
     J(sv,pcmps(xV,*(CO F*)yV,_V(z_),nc_,f==9,&bad_))
     E(   pcmps(yV,*(CO F*)xV,_V(z_),nc_,f==8,&bad_))
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
// Amber 2.9.0: q's minute, second, month and timespan too, and typed nulls. tval reads a packed temporal's null (NIT) as NL,
// so a null is NL whatever its kind, and the lowest number, as q orders it
Z __attribute__((always_inline)) inline L tval(A x)_(UC k=_t(x);k>=tdt?TQN(k)?*(L*)_V(x):(I)x==NIT?NL:(L)(I)x:gl_(x))   //inlined in tari, as it was before tcf (clang then called it out of line in the arithmetic check: date+1 was 1-2% slower)
Z __attribute__((always_inline)) inline L tvr(A x)_(UC k=_t(x);k>=tdt?TQN(k)?*(L*)_V(x):(L)(I)x:gl_(x))   //tval's number as stored: a packed null is the lowest int, as an order needs (tcl)
Z A tmk(UC k,L w)_(k<tdt?az(w):TQN(k)?AT(k,al(w)):w==NL?atq(k,NIT):w-(I)w?ez0():atq(k,w))   //a packed temporal is 32 bits: past them 'limit, as the literal (it wrapped)
// ns in one unit of a kind, for two kinds mixed: a date, a time, a minute, a second; a timestamp and a timespan are ns
Z L tun(UC k)_(k==tdt?86400000000000:k==ttm?1000000:k==tmn?60000000000:k==tsc?1000000000:1)
Z B tclk(UC k)_(k==ttm||k==tmn||k==tsc||k==tsp)   //a clock kind: a time of day or a span
// the kind two temporals give with + - & | (op 1 2 6 7), or 0 for 'type, as q: one kind minus itself is an int for a date
// or a month, a timespan for a timestamp, else that kind; a timestamp with a span or a time of day, or a date with one, a
// timestamp; a time of day with a timespan a timespan; a timestamp minus a date (or a date minus one) a timespan
Z UC trk(UC a,UC b,U op)_(P(op==6||op==7,a==b?a:0)P(op-1>1u,0)P(a==b,op==1?tclk(a)?a:0:a==tdt||a==tmo?ti:a==tnp?tsp:a)
 P((a==tnp||a==tdt)&&tclk(b),tnp)P(op==1&&tclk(a)&&(b==tnp||b==tdt),tnp)P(op==2&&(a==tnp&&b==tdt||a==tdt&&b==tnp),tsp)
 P(tclk(a)&&tclk(b)&&(a==tsp||b==tsp),tsp)0)
// v2 convention: consume y, leave x for the caller (bv opcode releases x; bV borrows a constant x).
// Only the documented cases (issue #18, then q's rules in 2.9.0): a temporal plus, minus or (a span or a time of day) times
// an int, two temporals as trk says, % of them as floats, comparisons. Anything else is 'type: tari read any other operand's
// low bits, so date+2.3 or date*2 were silent nonsense. A null gives the result's null, as q (0Nd+1 is 0Nd; it was 'domain).
// x:y is y (it gave x) - digest #27
NI Z A tcf(A,A,U),tkc(A,A,UC,UC,U),tcl(A,A,U),tdv(A,A),tal(A,A,U);Z __attribute__((always_inline)) inline B tcq(A x)_(_t(x)==tA&&_n(x)&&_t(*(A*)_V(x))>=tdt)   //tcq: a general list whose first item is a temporal, for tcl
Z A tari(A x,A y,U op)_(UC ka=_t(x),kb=_t(y);
 I(op>7,L va,vb;W tmq=1ull<<ts|1ull<<tdt|1ull<<ttm|1ull<<tnp|1ull<<tmn|1ull<<tsc|1ull<<tmo|1ull<<tsp;   //a comparison: each stored number as tval reads it; a float sent to tcf, from the arm a float takes anyway, and two temporal kinds to tkc, from y's temporal arms (#83 Q9)
 I(_t0(x),va=(I)x)E(P(ka==tf,tcf(x,y,op))va=*(L*)_V(x))   //packed (an int, a date, a time) or on the heap (a long, a timestamp, a float); a packed null is the lowest int, as NL
 I(_t0(y),vb=(I)y;P(__builtin_expect(kb-ka&&tmq>>kb&1&&tmq>>ka&1,0),ka==ts||kb==ts?et(y):tkc(x,y,ka,kb,op)))E(P(kb==tf,tcf(x,y,op))vb=*(L*)_V(y);P(__builtin_expect(kb-ka&&TQN(kb)&&tmq>>ka&1,0),ka==ts?et(y):tkc(x,y,ka,kb,op)))
 //a symbol is 'type, as q (#18; its intern index was compared: 2000.01.01=` was 1): tmq is the temporal kinds and the symbol, so the tests that sent two temporal kinds to tkc take it too, and no test is added
 mr(y);return ai((I)(op==8?va<vb:op==9?va>vb:va==vb));)   //(it read the float's bits: date<2.5 was 1); a comparison first
 B qa=ka>=tdt,qb=kb>=tdt;P(!op,y)
 I(op-1<2u&&qa!=qb&&!TQN(qa?ka:kb),UC rk=qa?ka:kb,ki=qa?kb:ka;P(ki-ti&&ki-tl,et(y))I u=qa?(I)x:(I)y;L c=gl_(qa?y:x);mr(y);   //a packed temporal and an int, + or -: the common case, first
  P(u==NIT||c==NL,atq(rk,NIT))L w=op==1?(L)((W)u+(W)c):qa?(L)((W)u-(W)c):(L)((W)c-(W)u);P(w-(I)w,ez0())return atq(rk,w);)
 I(op<8,P(op==4,tdv(x,y))UC rk;
  I(qa&&qb,rk=trk(ka,kb,op);P(!rk,et(y)))
  E(B ia=ka==ti||ka==tl,ib=kb==ti||kb==tl;rk=qa?ka:kb;P(!((op==1||op==2||op==3&&tclk(rk))&&(qa?ib:ia)),et(y)))
  L va=tval(x),vb=tval(y);mr(y);
  P(op<6&&(va==NL||vb==NL),tmk(rk,NL))   //a null, of either side: the result's null, as q (& | keep it lowest)
  P(qa&&qb&&ka-kb,__int128 a=(__int128)va*tun(ka),b=(__int128)vb*tun(kb),r=op==1?a+b:a-b;P(r-(L)r,ez0())tmk(rk,(L)r))   //two kinds: in ns
  L vv=op==1?(L)((W)va+(W)vb):op==2?(L)((W)va-(W)vb):op==3?(L)((W)va*(W)vb):op==6?MIN(va,vb):op==7?MAX(va,vb):va;
  P(__builtin_expect(rk>=tdt&&!TQN(rk)&&vv==(I)vv,1),atq(rk,vv))return tmk(rk,vv);)   //a packed temporal in range, in line (tmk is out of line)
 et(y))
//Amber 2.9.0: + - * & | < > = with a temporal list (tDT..tSP), in one loop: trk's kinds and tari's rules item by item (a null
//gives the result's null, a packed result past 32 bits is 'limit, two kinds mixed meet in ns). An int list or atom is one
//kind, ti. What tvo leaves (a float, a general list, %, two temporal kinds compared, as tkc does) goes item by item (e2)
Z __attribute__((always_inline)) inline L trv(CO V*p,U w,U i)_(w==4?({I v=((CO I*)p)[i];v==NIT?NL:(L)v;}):w==8?((CO L*)p)[i]:w==2?(L)((CO H*)p)[i]:(L)((CO G*)p)[i])   //an item's number, a 32-bit null as NL (an int list has none at 32 bits: Amber keeps 0N 64-bit)
#define TVK(t) (TQV(t)?TQA(t):TQ(t)?(t):(t)==ti||(t)==tl||LH(tG,t,tL)?ti:0)   //an operand's kind: a temporal's, ti for an int, 0 for the rest
NI Z A tvo(A x,A y,U op)_(UC t=xt,u=yt;
 P(t==tE,x=gZ(xR);A r=tvo(x,y,op);x(r))P(u==tE,tvo(x,gZ(y),op))
 UC ka=TVK(t),kb=TVK(u);P(!ka||!kb||op==4||op==5||op>10,e2(av+op,x,y))P(!op,y)
 B la=t<tM,lb=u<tM,qa=ka-ti,qb=kb-ti;P(la&&lb&&xn-yn,el(y))U n=la?xn:yn,wa=la?1u<<(Tw[t]-3):0,wb=lb?1u<<(Tw[u]-3):0;CO V*pa=la?xV:0,*pb=lb?yV:0;
 L a0=la?0:qa?tval(x):gl_(x),b0=lb?0:qb?tval(y):gl_(y);
 I(op>7,P(qa&&qb&&ka-kb,e2(av+op,x,y))A z=aG(n);G*RES o=zG;   //a comparison: the stored numbers (a null the lowest)
  I(la&&!lb&&wa==4&&b0==(I)b0,CO I*RES p=pa;I c=(I)b0;I(op==8,F(n,o[i]=p[i]<c))J(op==9,F(n,o[i]=p[i]>c))E(F(n,o[i]=p[i]==c)))   //a packed list and an atom, the common case: on 32 bits
  J(la&&!lb&&wa==8,CO L*RES p=pa;L c=b0;I(op==8,F(n,o[i]=p[i]<c))J(op==9,F(n,o[i]=p[i]>c))E(F(n,o[i]=p[i]==c)))
  E(F(n,L a=la?trv(pa,wa,i):a0,b=lb?trv(pb,wb,i):b0;o[i]=op==8?a<b:op==9?a>b:a==b))
  mr(y);return z;)
 UC rk;I(qa&&qb,rk=trk(ka,kb,op);P(!rk,et(y)))E(rk=qa?ka:kb;P(!(op==1||op==2||op==3&&tclk(rk)),et(y)))
 B cv=qa&&qb&&ka-kb,pk=rk-ti&&!TQN(rk);L sa=cv?tun(ka):1,sb=cv?tun(kb):1;A z=an(n,rk==ti?tL:TQL(rk));B ov=0;
 I(pk&&la&&!lb&&wa==4&&!cv&&op-1<2u,CO I*RES p=pa;I*RES o=zI;   //a packed list plus or minus an int (date+1): on 32 bits
  I(b0==NL,F(n,o[i]=NIT))J(b0-(I)b0,F(n,ov|=p[i]!=NIT;o[i]=NIT))   //32 bits wrap, the overflow from the signs: one pass the compiler vectorises
  J(op==1,I c=(I)b0;I f=0;F(n,I v=p[i];I r=(I)((U)v+(U)c);f|=(v!=NIT)&(((v^r)&(c^r))<0);o[i]=v==NIT?NIT:r)ov=f)
  J(qa,I c=(I)b0;I f=0;F(n,I v=p[i];I r=(I)((U)v-(U)c);f|=(v!=NIT)&(((v^c)&(v^r))<0);o[i]=v==NIT?NIT:r)ov=f)
  E(I c=(I)b0;I f=0;F(n,I v=p[i];I r=(I)((U)c-(U)v);f|=(v!=NIT)&(((c^v)&(c^r))<0);o[i]=v==NIT?NIT:r)ov=f))
 E(F(n,L a=la?trv(pa,wa,i):a0,b=lb?trv(pb,wb,i):b0,r;
  I(op<6&&(a==NL||b==NL),r=NL)
  J(cv,__int128 c=(__int128)a*sa,d=(__int128)b*sb,e=op==1?c+d:c-d;ov|=e!=(L)e;r=(L)e)
  E(r=op==1?(L)((W)a+(W)b):op==2?(L)((W)a-(W)b):op==3?(L)((W)a*(W)b):op==6?MIN(a,b):MAX(a,b))
  I(pk,ov|=r!=NL&&r!=(I)r;zI[i]=r==NL?NIT:(I)r)E(zL[i]=r)))
 mr(y);P(ov,z(ez0()))rk==ti?sqzZ(z):z)
A2(ari,C t=xt,u=yt;
 P(TQV(t)||TQV(u),tvo(x,y,f))   //a temporal list (2.9.0)
 P(t>=tdt||u>=tdt,P(xtt&&ytt,tari(x,y,f))P(t<tM&&t-tA||u<tM&&u-tA,tvo(x,y,f))I(f-8<3u&&(tcq(x)||tcq(y)),A r=tcl(x,y,f);P(r,y(r)))e2(av+f,x,y))   //a temporal atom with a typed list: tvo (2.9.0)  //a temporal atom with a list: item by item (tari read the list as one int); < > = in one loop when they can (tcl)
 W v=1ull<<t|1ull<<u;   //after the temporals; 64 bits, the codes pass 31
 P(!(v&~(1ull<<tG|1ull<<tH|1ull<<tI|1ull<<tL|1ull<<tC|1ull<<ti|1ull<<tl|1ull<<tc)),ariz(x,y,f))
 P(v&(1ull<<tm|1ull<<tM|1ull<<tA),I(f-1<2u&&!(v&~(1ull<<tA|1ull<<ti))&&(tcq(x)||tcq(y)),A r=tal(x,y,f);P(r,y(r)))I(f-8<3u&&!(v&~(1ull<<tA|1ull<<ti|1ull<<tl))&&(tcq(x)||tcq(y)),A r=tcl(x,y,f);P(r,y(r)))e2(av+f,x,y))   //< > = of general lists, or of one with an int: temporals in one loop when they can (tcl)
 P(t==tB,x=cG(xR);x(ari(x,y)))
 P(u==tB,ari(x,cG(y)))
 P(t==tE,P(f==1&&ytzc,addzE(gl(y),xR))P(f==2&&ytzc,addzE((L)(0-(W)gl(y)),xR))x=gZ(xR);x(ari(x,y)))
 P(u==tE,P(f==1&&xtzc,addzE(gl_(x),y))ari(x,gZ(y)))
 P(v&(1ull<<tf|1ull<<tF),arif(x,y,f))
 I(f-8<3u,
  P(v&1ull<<tS,P(f==10&&!(v&~(1ull<<ts|1ull<<tS)),ariz(x,y,f))e2f(ari,x,y))
  P(v&1ull<<ts,v^1ull<<ts?et(y):ai(f==8?qA(x,y)<0:f==9?qA(x,y)>0:xv==yv)))
 et(y))

#define M(s,i) A2(s,U o=f;f=i;x=ari(x,y);f=o;x)
 M(add,1)M(mul,3)M(dvd,4)M(mod,5)M(mnm,6)M(mxm,7)M(ltn,8)M(gtn,9)M(eql,10)
#undef M
A ucb(A x/*1*/)_(P(xtc,ai((UC)xv))P(xtC,A y=an(xn,tH);F(xn,yh=(UC)xc)x(y))x)                        //chars as unsigned bytes
A2(ltu,P((xtc||xtC)&&(ytc||ytC),A u=ucb(xR);u(ltn(u,ucb(y))))ltn(x,y))                           //the verb <: chars order among themselves as unsigned bytes (with numbers, and ltn: numeric)
A2(gtu,P((xtc||xtC)&&(ytc||ytC),A u=ucb(xR);u(gtn(u,ucb(y))))gtn(x,y))
A cub(A x/*1*/)_(P(!x,0)P(xti,ac((C)xv))P(LH(tG,xt,tL),A y=an(xn,tC);S4(xw-3,F(xn,yc=(C)xg),F(xn,yc=(C)xh),F(xn,yc=(C)xi),F(xn,yc=(C)xl))x(y))K1("{`c$x}",x))                                                            //unsigned bytes (as ucb gives) back to chars
A2(mnu,P((xtc||xtC)&&(ytc||ytC),A u=ucb(xR);cub(u(mnm(u,ucb(y)))))mnm(x,y))                     //the verb &: of two chars, the lesser as unsigned bytes, a char (with a number: numeric, as ever)
A2(mxu,P((xtc||xtC)&&(ytc||ytC),A u=ucb(xR);cub(u(mxm(u,ucb(y)))))mxm(x,y))                     //the verb |
Z NI __attribute__((cold)) A exck(A x,A y)_(K2("{$[x;xkey[x#cols unkey y;unkey y];unkey y]}",x,y))   //n!t (exc; out of line)
A2(dex,y)A2(sub,U o=f;f=2;x=ari(x,y);f=o;x)X2(exc,RMT(ytm?el(y):ytt?exc(x,rsz(xN,y)):xN-yN?el(y):am(xR,y))Rs(x=rsz(yN,x);x(exc(x,y)))Rilc(/* amber 1.9.3: `!` with a negative integer left argument is the
 * q-family system-verb slot -- -8!x serialises to a byte vector and -9!y
 * deserialises it (src/ser.c). Only -8 and -9, and only on a genuine
 * integer atom, are intercepted; every other left argument (negative ones
 * included) and every char atom still reach mod() exactly as before, so no
 * existing `!` behaviour moves. */
 I(_t(x)-tc,L sv_=gl_(x);I(sv_==-8,A r_=qser(y,0);mr(y);return r_;)I(sv_==-9,return qdes(y))I(sv_==-11,return qreplay(y))I(sv_==-3,return kst(y)))   //2.9.0: -8! -9! are q's bytes (Amber's own: `ser `des), -11! replays a log   //-3!x: q's show, the text of x (2.9.0; it was k's divide by 3, which `fdv names)
 I(_tz(x)&&!_tP(y)&&(yt==tM||yt==tm&&_tM(yx)),return exck(x,y))   //Amber 2.8.0: n!t keys a table on its first n columns, as q (0!kt unkeys; it was kt mod 0, kt)
 mod(x,y))R_(et(y)))
// a temporal compared with a float (tari): as q casts the float to the temporal, the stored number (days, ms or ns) against
// the float rounded half away from zero; 0n, -0w 0w and floats past the int range go the int-float way, so 0n is below every
// temporal but the null timestamp, which it equals, and 0w above, as in q. Consumes y, leaves x, as tari.
// a temporal compared with a float (tari): as q casts the float to the temporal, the stored number (days, ms or ns) against
// the float rounded half away from zero; 0n, -0w 0w and floats past the int range go the int-float way, so 0n is below every
// temporal but the null timestamp, which it equals, and 0w above, as in q. Consumes y, leaves x, as tari.
NI Z A tcf(A x,A y,U op)_(B qa=_t(x)>=tdt;A t=qa?x:y;F v=*_F(qa?y:x);L s=TQN(_t(t))?*(L*)_V(t):(L)(I)t;
 P(!(__builtin_fabs(v)<0x1p63),P(qa,A u=az(s);u(ari(u,y)))mr(y);ari(x,az(s)))
 L r=(L)__builtin_round(v),va=qa?s:r,vb=qa?r:s;mr(y);ai((I)(op==8?va<vb:op==9?va>vb:va==vb)))
// % of temporals and ints, as q: floats of the stored numbers (a null 0n). Consumes y, as tari.
Z F tvf(A x)_(_t(x)==tf?*_F(x):({L v=tval(x);v==NL?__builtin_nan(""):(F)v;}))
NI Z A tdv(A x,A y)_(UC a=_t(x),b=_t(y);P(!(a>=tdt||a==ti||a==tl||a==tf)||!(b>=tdt||b==ti||b==tl||b==tf),et(y))F r=tvf(x)/tvf(y);mr(y);af(r))
// Two kinds compared as q (#83 Q9; the stored numbers were compared, days with ms or ns). A date, a month and a timestamp
// compare as ns since 2000 (a date at its midnight, a month at its first day's, exactly, so no date wraps); a time, a minute,
// a second and a timespan as ns; a timestamp against a time, a minute or a second is its time of day in that unit, floored,
// as q's `time$ `minute$ `second$ (so 1999.12.31D11:59:59.999999999 is 11:59:59.999), against a timespan its ns. A date or
// a month against a time of day or a span is 'type. A null is below every value and equal to none of another kind.
// Consumes y, as tari.
Z L tod(L v)_(L m=v%86400000000000;m+(m<0)*86400000000000)
L tmd(L v)_(L q=v/12-(v%12<0),m=v-12*q+1,y=2000+q-(m<=2),e=(y>=0?y:y-399)/400,o=y-e*400,p=m+(m>2?-3:9);e*146097+365*o+o/4-o/100+(153*p+2)/5-730425)   //a month's first day (Hinnant's days_from_civil, as p.c's ymd2days)
Z B tcal(UC k)_(k==tdt||k==tnp||k==tmo)
NI Z A tkc(A x,A y,UC ka,UC kb,U op)_(B ca=tcal(ka),cb=tcal(kb);P(ca-cb&&(ka==tdt||ka==tmo||kb==tdt||kb==tmo),et(y))L va=tval(x),vb=tval(y);mr(y);__int128 a,b;
 I(ca==cb,a=ka==tmo?(__int128)tmd(va)*86400000000000:(__int128)va*tun(ka);b=kb==tmo?(__int128)tmd(vb)*86400000000000:(__int128)vb*tun(kb))
 E(UC kc=ca?kb:ka;L t=ca?va:vb,c=ca?vb:va;L u=kc==tsp?t:tod(t)/tun(kc);a=ca?u:c;b=ca?c:u)
 I(va==NL,a=-((__int128)1<<100))I(vb==NL,b=-((__int128)1<<100))
 ai((I)(op==8?a<b:op==9?a>b:a==b)))
// < > = of a general list of temporals with an atom, or of two general lists of one length (ari): when each item is a
// temporal of one kind and the other side is that kind or an int (or, for two lists, each pair is one kind), the stored
// numbers compare in one loop, as tari would compare them item by item; otherwise 0, and e2 goes item by item (to tcf
// and tkc for a float or another kind). The loop reads each item's type once, so tari's dispatch per item is gone. Consumes nothing.
// + - of a general list of one packed temporal kind (a date, a time, a minute, a second, a month) and an int (ari): in one
// loop, as tari gives them item by item (a null stays one); else 0, and e2 goes item by item (and to 'limit past 32 bits).
// Consumes nothing. (Amber 2.9.0: tari's null and kind checks made the item by item way 14% slower.)
NI Z A tal(A x,A y,U op)_(B l=_t(x)==tA;A s=l?y:x,t=l?x:y;P(_t(t)-tA||_t(s)-ti,0)U n=_n(t);P(!n,0)CO A*a=(A*)_V(t);UC k=_t(a[0]);P(k<tdt||TQN(k),0)
 L c=(I)s;A r=aA(n);A*p=(A*)_V(r);
 F(n,A e=a[i];P(_t(e)-k,_n(r)=(U)i;mr(r);0)I u=(I)e;I(u==NIT,p[i]=e;continue)L w=l?op==1?(L)u+c:(L)u-c:op==1?c+(L)u:c-(L)u;P(w-(I)w,_n(r)=(U)i;mr(r);0)p[i]=atq(k,w))r)
NI Z A tcl(A x,A y,U op)_(UC tx=_t(x),ty=_t(y);A r,*a,*b;U n;I*p;
 I(tx==tA&&ty==tA,n=_n(x);P(!n||_n(y)-n,0)a=(A*)_V(x);b=(A*)_V(y);UC k=_t(a[0]);P(k<tdt,0)r=an(n,tI);p=_I(r);
  I(TQN(k),F(n,P(_t(a[i])-k||_t(b[i])-k,mr(r);0)L c=*(L*)_V(a[i]),d=*(L*)_V(b[i]);p[i]=op==8?c<d:op==9?c>d:c==d))E(F(n,P(_t(a[i])-k||_t(b[i])-k,mr(r);0)I c=(I)a[i],d=(I)b[i];p[i]=op==8?c<d:op==9?c>d:c==d))return r;)
 B l=tx==tA;A s=l?y:x,t=l?x:y;P(_t(t)-tA,0)UC k=_t(s);P(!(k>=tdt||k==ti||k==tl),0)n=_n(t);P(!n,0)a=(A*)_V(t);
 UC m=k>=tdt?k:_t(a[0]);P(m<tdt,0)L v=tvr(s);r=an(n,tI);p=_I(r);
 I(TQN(m),F(n,P(_t(a[i])-m,mr(r);0)L u=*(L*)_V(a[i]),c=l?u:v,d=l?v:u;p[i]=op==8?c<d:op==9?c>d:c==d))E(F(n,P(_t(a[i])-m,mr(r);0)L u=(I)a[i],c=l?u:v,d=l?v:u;p[i]=op==8?c<d:op==9?c>d:c==d))r)   //one loop per storage: m is the kind of every item
