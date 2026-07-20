#include<stdbool.h> // ngn/k, (c) 2019-2024 ngn, GNU AGPLv3 - https://codeberg.org/ngn/k/raw/branch/master/LICENSE
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
#define HD 32ll//header
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
#define AL(x) __builtin_assume_aligned(x,32)
#define CLZ   __builtin_clzll
#define CTZ   __builtin_ctzll
#define MC    __builtin_memcpy
#define MS    __builtin_memset
#define PC    __builtin_popcountll
#define SL    __builtin_strlen
#define SQ    __builtin_sqrt
#define REFB  1
#define MINE(x) (_r(x)==REFB)

TD void V;TD bool B;TD char G,C;TD char unsigned UC;TD CO C*S;TD short H;TD unsigned short UH;TD int I;TD unsigned int U;TD long long L;TD double F;TD size_t N;
TD unsigned long long W,A,A0(),A1(A),A2(A,A),A3(A,A,A),A4(A,A,A,A),AA(CO A*,U),AX(A,CO A*,U);

#define A0(f,b...) A f(                )_(b)
#define A1(f,b...) A f(A x             )_(b)/*1*/
#define A2(f,b...) A f(A x,A y         )_(b)/*01*/
#define A3(f,b...) A f(A x,A y,A z     )_(b)
#define A4(f,b...) A f(A x,A y,A z,A u )_(b)
#define AX(f,b...) A f(A x,CO A*a,U n  )_(DBG(Q(n<=8));b)/*0,1..1,n*/
#define AA(f,b...) A f(    CO A*a,U n  )_(DBG(Q(n<=8));b)
A1 _R,aA1,asc,AZ,blw,cB,cG,cC,cF,cH,cI,cL,cS,dsc,enl,epr,err,fir,flp,flr,frk,gZ,gg,grp,hex,imx,imn,inv,jS,js0,js1,kcos,kexp,klog,ksin,kst,las,len,m0,m1,mkn,mRa,mr,mut,
 neg,not,nul,of0,of1,opn,out,prng,qkmp,qpri,qte,raz,rev,rs0,spl,sqr,sqz,sqzZ,str,str0,til,typ,u0c,u1c,u2c,unh,unq,val,whr;
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
C*sf(C*,L),*sl(C*,L),sup(A*,A*),tZ(L),*strchrnul(S,I);
UC gi(A);
I qA(A,A),qf(F,F),rnk(A);
U _K(A),si(S,C),_N(A),js_eval(C*,U,C*,U),fG(CO G*,U,G),fI(CO I*,U,I),fL(CO L*,U,L),us(S);
L cfm(CO A*,I),gl_(A),gl(A),iw(A,U,L),now(),pl(S*),minfZ(L,A),addfB(CO V*,U),addfZ(L,A),pf(S*);
S su(U),pID(S);
W pu(S*);
F gf(A);
EX A1*v1[];EX A2*v2[];EX AA*v8[];EX A gv[256],cns,cn[],ci[2][5];EX I pg;EX TY(CO C[])vc,TS,Tw,TR,TT,TX,Tk;EX S*argv,*env;

//                    0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25
//                      () !i ,1 ,i ,i ,i ,i ,f "" ,` +m X!  5  6 .6 "c" ` {} 1+ ++ +/ +:  +  / 2:
enum                 {tA=1,tE,tB,tG,tH,tI,tL,tF,tC,tS,tM,tm,ti,tl,tf,tc,ts,to,tp,tq,tr,tu,tv,tw,tx,tn};
#define T_ CO C TS[]="0""A""I""I""I""I""I""I""F""C""S""M""m""i""i""f""c""s""o""p""q""r""u""v""w""x",/*type symbols     */\
                Tw[]={0, 6, 6, 0, 3, 4, 5, 6, 6, 3, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6},/*log2(size)       */\
                TT[]={0,tA,tL,tB,tG,tH,tI,tL,tF,tC,tS,tM,tM,tI,tL,tF,tC,tS,tA,tA,tA,tA,tA,tA,tA,tA},/*list type        */\
                TX[]={0,tG,tG,tG,tG,tH,tI,tL,tF,tG,tI,tG,tG, 0, 0,tF,tG,tI,tG,tG,tG,tG,tG,tG,tG,tG},/*arith conformance*/\
                Tk[]="0""L""I""I""I""I""I""I""F""C""S""T""D""i""i""f""c""s""?""?""?""?""?""?""?""?";/*for the api (k.h)*/
#define TR(t) ((1<<tA|1<<tM|1<<tm|1<<to|1<<tp|1<<tq|1<<tr)>>(t)&1)//reftypes
#define TP(t) ((1<<ti|1<<tc|1<<ts|1<<tu|1<<tv|1<<tw|1<<tx)>>(t)&1)//packed types
#define TU(t) ((t)>=to)                                           //function types

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
#define NL (1ll<<63)
#define WL (~NL)
#define K(s,a...) ({Z A f;k8(&f,s,A(a),L(A(a)));})
#define K1(s,x)   ({Z A f;k1(&f,s,x);})
#define K2(s,x,y) ({Z A f;k2(&f,s,x,y);})

#define ERR M(c,compile)M(d,domain)M(i,index)M(l,length)M(n,nyi)M(o,io)M(p,parse)M(r,rank)M(s,stack)M(t,type)M(v,value)M(z,limit)
#define M(t,m) A0 e##t##0;A1 e##t;AA e##t##8;
 ERR
#undef M
#define N(x,a...) ({A r_=(x);P(!r_,a;0)r_;})//error pass-through

#define ov(x) ov_(#x":",(L)(x))
#define oo os("["__FILE__":"M2(__LINE__)"]");
#define nop {asm volatile("fnop");}
U os(S);W ov_(S,W);
