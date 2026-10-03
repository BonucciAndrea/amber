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
ZN A amdFF(A x,A y,U f)_(U n=xn;P(n-yn,el(y))A z=MINE(y)?y:aF(n);_at(z)=0;G(&aFF,sFF,mFF,dFF)[f-1](xV,yV,zV,n+3>>2);y-z?y(z):z)
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
Z A modzf(L n,A y,U f)_(P(!n,y)P(n<0,en(y))A u=az(n);u(K2("{y-x*(-x)!_y}",u,y)))
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
 U n=xn;A z=aG(n);My(A(&ltnG,ltnH,ltnI,ltnL,eqlG,eqlH,eqlI,eqlL)[(f==10)<<2|w](a,b,zV,n))z)
Z A cmpzZ(L v,A y,U f)_(U w=yw-3;P(tG+w<tZ(v),y(rsz(yn,ai(f==8?v<0:f==9?v>0:0))))
 U n=yn;A z=aG(n);My(A(&ltng,ltnh,ltni,ltnl,gtng,gtnh,gtni,gtnl,eqlg,eqlh,eqli,eqll)[f-8<<2|w](v,yV,zG,n))z)

Z A addzE(L v,A x)_(Lij P(v>0?j>WL-v:i<NL-v,addzZ(v,gZ(x),1))x(0);aE(i+v,j+v))   //ends past the int range: a vector (ints wrap), not a wrapped range
// amber 2.5 (exp): the number-with-vector float loops are functions of their own (noipa: one copy of the
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
Z A addfF(F v,A y,U f)_(A z=MINE(y)?y:aF(yn);_at(z)=0;fzadd(v,yF,zF,zn+3&-4);y-z?y(z):z)
Z A mulfF(F v,A y,U f)_(A z=MINE(y)?y:aF(yn);_at(z)=0;fzmul(v,yF,zF,zn+3&-4);y-z?y(z):z)
Z A subfF(F v,A y,U f)_(A z=MINE(y)?y:aF(yn);_at(z)=0;simd_subs_f64(v,yV,zV,yn);y-z?y(z):z)/* v - y */
Z A admfF(F v,A y,U f)_((f==3?mulfF:f==2?subfF:addfF)(v,y,f))
Z A dvdfF(F v,A y,U f)_(A z=MINE(y)?y:aF(yn);_at(z)=0;fzdvl(v,yF,zF,zn+3&-4);y-z?y(z):z)
Z A dvdFf(A x,F v,U f)_(A z=aF(xn);fzdvr(xF,v,zF,xn);z)
// ---- amber 2.5 (exp): float fusion ------------------------------------------------------------------------
// fzrun runs the program the compiler made for an element-wise tree of + - * % (src/b.c fz): d is
// (program bytes; literals..), l the frame's locals. It reads the leaves; unless every one is a float vector of
// one length n>=FZ_MIN or a number (and no operation has two numbers) it returns 0 and the unfused code runs.
// Otherwise the tree is evaluated FZ_BK elements at a time, each block's intermediates in a small buffer that
// stays in cache, and each operation through the function the unfused primitive uses, with the operands
// arif() would give it (a number on the left of - is the subs kernel, on the right v+x with -v; an int number
// goes through cF as arif does), so every element is the same double. A plain tree is split over threads by
// whole blocks. With +/ at the root the blocks write the root vector and simd_sum_f64 sums it, the very call +/
// makes: a copy of its loop fed block by block adds in the same order, but where two NaNs meet the compiler's
// choice of operand order decides which one survives, so only the same machine code gives the same bits.
#define FZ_MIN 2048u
#define FZ_BK 512u        //a multiple of 16: the sum's 4 accumulators x 4 lanes (x 8 lanes with 512-bit vectors)
#define FZ_PAR (1u<<17)
#define FZ_N 32           //program steps
TD struct{UC op,s;}FZS;    //'v' vector leaf, 'c' number leaf, else + - * % on the two below
TD struct{FZS p[FZ_N];U np,dp,n,nb,nt;CO F*lv[FZ_N];F lc[FZ_N];F*out;}FZC;
Z V fzblk(CO FZC*c,U o,U cnt,F*bf,F*dst){CO F*sv[FZ_N];F ss[FZ_N];B sk[FZ_N];U d=0,cr=cnt+3&~3u;
 for(U j=0;j<c->np;j++){FZS s=c->p[j];
  if(s.op=='v'){sv[d]=c->lv[j]+o;sk[d++]=0;continue;}
  if(s.op=='c'){ss[d]=c->lc[j];sk[d++]=1;continue;}
  U a=d-2,e=d-1;d--;F*r=j==c->np-1?dst:bf+(N)a*FZ_BK;
  if(!sk[a]&&!sk[e]){CO F*x=sv[a],*y=sv[e];
   S(s.op,C('+',simd_add_f64(x,y,r,cr))C('-',simd_sub_f64(x,y,r,cr))C('*',simd_mul_f64(x,y,r,cr))C('%',simd_div_f64(x,y,r,cr)))}
  else if(sk[a]){F v=ss[a];CO F*y=sv[e];
   S(s.op,C('+',fzadd(v,y,r,cr))C('-',simd_subs_f64(v,y,r,cnt))C('*',fzmul(v,y,r,cr))C('%',fzdvl(v,y,r,cr)))}
  else{CO F*x=sv[a];F v=ss[e];
   S(s.op,C('+',fzadd(v,x,r,cr))C('-',fzadd(-v,x,r,cr))C('*',fzmul(v,x,r,cr))C('%',fzdvr(x,v,r,cnt)))}
  sv[a]=r;sk[a]=0;}}
Z V fzw(V*c_,int t){FZC*c=c_;U b0=(U)((N)c->nb*t/c->nt),b1=(U)((N)c->nb*(t+1)/c->nt);F bf[c->dp*FZ_BK];
 for(U k=b0;k<b1;k++){U o=k*FZ_BK,m=MIN(FZ_BK,c->n-o);fzblk(c,o,m,bf,c->out+o);}}
A fzrun(A d,A*l){A pg=_A(d)[0];CO UC*q=(CO UC*)_V(pg),*e=q+_n(pg);B sum=*q++&1;FZC c;B kd[FZ_N];U np=0,dp=0,n=0;
 W(q<e,UC o=*q++;A v=0;
  I(o=='l',v=l[*q++])J(o=='g',v=gv[q[0]|(U)q[1]<<8];q+=2)J(o=='k',v=_A(d)[*q++])
  E(P(dp<2||kd[dp-1]&&kd[dp-2]||np>=FZ_N,0)c.p[np++]=(FZS){o,0};dp--;kd[dp-1]=0;continue)
  P(!v||np>=FZ_N,0)
  I(!_tP(v)&&_T(v)==tF,U m=_n(v);P(n&&m!=n,0)n=m;c.lv[np]=_F(v);c.p[np++]=(FZS){'v',0};kd[dp++]=0)
  E(UC t=_t(v);P(t!=tf&&t!=ti&&t!=tl,0)c.lc[np]=t==tf?*_F(v):gf(cF(_R(v)));c.p[np++]=(FZS){'c',0};kd[dp++]=1)
  )
 P(dp!=1||n<FZ_MIN||kd[0],0)
 c.np=np;c.n=n;c.nb=(n+FZ_BK-1)/FZ_BK;c.dp=0;{U k=0;F(np,I(c.p[i].op=='v'||c.p[i].op=='c',k++)E(k--)c.dp=MAX(c.dp,k))}
 int nt=n>=FZ_PAR?par_thread_count(n):1;I(nt>(int)c.nb,nt=(int)c.nb)c.nt=(U)nt;
 A z=aF(n);P(!z,0)_at(z)=0;c.out=zF;
 I(nt>1,par_run(nt,fzw,&c))E(fzw(&c,0))
 P(sum,F r=simd_sum_f64(zF,n);mr(z);af(r))
 return z;}
// the VM's bF (src/b.c): b at its operands j c p, l the frame's locals, k the lambda's constants. Out of line so
// that the dispatch loop's registers are not disturbed; p, a local leaf, is looked at first and unless it holds
// a float vector long enough there is nothing more to do (scalar code, short vectors).
NI A fzop(CO UC*b,A*l,A*k){UC p=b[2];I(p<16,A v=l[p];P(!v||_tP(v)||_T(v)!=tF||_n(v)<FZ_MIN,0))return fzrun(k[b[1]],l);}
Z A dvdzZ(L v,A y,U f)_(dvdfF(v==NL?NF:v,cF(y),f))
Z A dvdZZ(A x,A y,U f)_(x=cF(xR);x(amdFF(x,cF(y),f)))
// amber: scalar-scalar fallback arithmetic. Every step that can involve the
// long null (0N == LLONG_MIN) is done in the unsigned counterpart type:
// `-a`, `a+b`, `a*b` and the modulo fix-up all overflow signed range for
// extreme operands (found by tests/fuzz.py under UBSan, e.g. `2!-0w`), which
// is undefined behaviour. Same bits, defined semantics -- Amber's integer
// arithmetic has always been documented as wrapping, not trapping.
#define NEGW(v) ((L)(0-(W)(v)))
Z A arizz(L a,L b,U f)_(P(f==4,af((a==NL?NF:(F)a)/(b==NL?NF:(F)b)))
 az(f==1?(L)((W)a+(W)b)
   :f==2?(L)((W)a-(W)b)
   :f==3?(L)((W)a*(W)b)
   :f==5?(!a?b:a<0?(b<0?(L)((W)-1-(W)(~b/NEGW(a))):b/NEGW(a)):(L)(((W)((L)((W)(b%a)+(W)a)%a))))
   :f==6?MIN(a,b):f==7?MAX(a,b):f==8?a<b:f==9?a>b:f==10?a==b:0))
Z A arizZ(L v,A y,U f)_(A(&addzZ,subzZ,mulzZ,dvdzZ,modzZ,mmmzZ,mmmzZ,cmpzZ,cmpzZ,cmpzZ)[f-1](v,y,f))
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
Z A eqFF(A x,A y/*00*/)_(B a=xtf,b=ytf;P(a&&b||!a&&!b&&xn-yn,0)U n=a?yn:xn;A z=an(n,tG);G*RES r=zV;CO F*RES p=xF,*RES q=yF;
 P(a,F u=*p;F(n,r[i]=EQV(u,q[i]))z)P(b,F v=*q;F(n,r[i]=EQV(p[i],v))z)F(n,r[i]=EQV(p[i],q[i]))z)
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
 P(t>=tdt||u>=tdt,P(xtt&&ytt,tari(x,y,f))e2(av+f,x,y))  //a temporal atom with a list: item by item (tari read the list as one int)
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
A ucb(A x/*1*/)_(P(xtc,ai((UC)xv))P(xtC,A y=an(xn,tH);F(xn,yh=(UC)xc)x(y))x)                        //chars as unsigned bytes
A2(ltu,P((xtc||xtC)&&(ytc||ytC),A u=ucb(xR);u(ltn(u,ucb(y))))ltn(x,y))                           //the verb <: chars order among themselves as unsigned bytes (with numbers, and ltn: numeric)
A2(gtu,P((xtc||xtC)&&(ytc||ytC),A u=ucb(xR);u(gtn(u,ucb(y))))gtn(x,y))
A cub(A x/*1*/)_(P(!x,0)P(xti,ac((C)xv))P(LH(tG,xt,tL),A y=an(xn,tC);S4(xw-3,F(xn,yc=(C)xg),F(xn,yc=(C)xh),F(xn,yc=(C)xi),F(xn,yc=(C)xl))x(y))K1("{`c$x}",x))                                                            //unsigned bytes (as ucb gives) back to chars
A2(mnu,P((xtc||xtC)&&(ytc||ytC),A u=ucb(xR);cub(u(mnm(u,ucb(y)))))mnm(x,y))                     //the verb &: of two chars, the lesser as unsigned bytes, a char (with a number: numeric, as ever)
A2(mxu,P((xtc||xtC)&&(ytc||ytC),A u=ucb(xR);cub(u(mxm(u,ucb(y)))))mxm(x,y))                     //the verb |
A2(dex,y)A2(sub,U o=f;f=2;x=ari(x,y);f=o;x)X2(exc,RMT(ytm||rnk(x)<0?ed(y):ytt?exc(x,rsz(xN,y)):xN-yN?el(y):am(xR,y))Rs(x=rsz(yN,x);x(exc(x,y)))Rilc(/* amber 1.9.3: `!` with a negative integer left argument is the
 * q-family system-verb slot -- -8!x serialises to a byte vector and -9!y
 * deserialises it (src/ser.c). Only -8 and -9, and only on a genuine
 * integer atom, are intercepted; every other left argument (negative ones
 * included) and every char atom still reach mod() exactly as before, so no
 * existing `!` behaviour moves. */
 I(_t(x)-tc,L sv_=gl_(x);I(sv_==-8||sv_==-9,return sv_==-8?ser8(y):des9(y)))
 mod(x,y))R_(et(y)))
