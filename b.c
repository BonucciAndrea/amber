#include"a.h" // ngn/k, (c) 2019-2024 ngn, GNU AGPLv3 - https://codeberg.org/ngn/k/raw/branch/master/LICENSE
#define C2(x,a...) case x:C(a)
#define C6(u,v,w,x,y,z,a...) case u:case v:case w:case x:case y:case z:{a;break;}
#define C16(x,a...) case x:case x+1:case x+2:case x+3:case x+4:case x+5:case x+6:case x+7:case x+8:case x+9:case x+10:case x+11:case x+12:case x+13:case x+14:case x+15:{a;break;}
#define C32(x,a...) C16(x,C16(x+16,a))
#define U(x,a...) I(!(x),a;goto l)
#define OFF 4 //offset of constants in a function object
#define n1 -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
#define p1  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
enum {bu,  bv=32,bs=64,bg=80,bd=96,ba=112,bp,bm,bM,bx,bX,by,bY,bG,bS,bl,bL,bz,bj,bo,bP,bV,bc};      //opcodes
Z CO C di[]={                     [ba]= 1, 1, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 2, 0},      //extra bytes after opcode
ds[]={[bv]=n1,n1,n1,   p1,   p1,  [ba]= 1, 1,-1,-1,-1,-1,-1,-1, 1,-1, 1, 0,-1, 0, 1,-1, 0, 1},      //stack size delta
ks[]={                            [ba]=-1,-1, 0, 0, 0, 0, 0, 0, 0, 0,-1, 1, 0, 0, 0, 0, 0, 0};      //stack size delta (coefficient for the next byte)
AX(run,Q(xto)Z I d;P(++d>2048,es8(a,n))P(n-xk,er8(a,n))UC*b=_V(xy),c,nl=_n(xA[3]);A l[nl+*b++],*s=l+L(l);MS(l,0,SZ l);MC(l,a,8*n);//virtual machine
 W((c=*b++),S(c,                                                                                    //          |BYTES |          STACK        |         EFFECT
  C32(bu,U(*s=v1[c-bu](*s)))                                                                        //monad     |bu+m  |.. x -> monads[m][x]   |
  C32(bv,A x=*s++;U(*s=x(v2[c-bv](x,*s))))                                                          //dyad      |bv+d  |.. y x -> dyads[d][x;y]|
  C16(bs,A*p=l+c%16;I(*p,mr(*p))*p=*s++)                                                            //set local |bs+i  |.. x -> ..             |locals[i]:x
  C16(bg,A*p=l+c%16,x=*p;U(*--s=x)xR)                                                               //get local |bg+i  |.. -> .. locals[i]     |
  C16(bd,A*p=l+c%16,x=*p;U(*--s=x)*p=0)                                                             //del local |bd+i  |.. -> .. locals[i]     |locals[i]:NULL (freed)
  C2(ba,bp,UC n=*b++;A x=*s;s+=n;U(*s=x((c==ba?_8:prj)(x,s-n+1,n))))                                //apply|proj|ba,n  |.. z y x -> .. x[y;z]  |
  C6(bm,bM,bx,bX,by,bY,A*p=(c&1?gv:l)+*b++,x=*p;U(x,*s=ev(*s))A y=*s++;                             //          |      |                       |
   I(c==bm||c==bM,y=v2[*b++](x,y);U(y,*--s=0)*p=x(y))                                               //mod asgn  |bm,i,d|.. x -> ..             |vars[i]:dyads[d][vars[i];x]
   E(x=*p=d4(x,y,av+*b++,*s);mr(*s);I(c==bx||c==bX,mr(y);U(x,*s=0)s++)                              //ind asgn  |bx,i,d|.. z y -> ..           |vars[i]:  .[vars[i];y;dyads[d];z]
                                    E(U(x,*s=y(0))U(*s=dot(x,y)))))                                 //ind asgn  |by,i,d|.. z y -> .. r         |vars[i]:r:.[vars[i];y;dyads[d];z]
  C(bG,A x=*--s=gv[*b++];U(x,ev0())xR)                                                              //get global|bG,i  |.. -> .. globals[i]    |
  C(bS,A*p=gv+*b++,x=*s++,y=*p;*p=y?y(x):x)                                                         //set global|bS,i  |.. x -> ..             |globals[i]:x
  C(bl,UC n=*b++;s+=n-1;*s=sqz(aV(tA,n,s-n+1)))                                                     //list      |bl,n  |.. y x -> .. (x;y)     |
  C(bL,UC n=*b++;A x=*s;U(xtt||xN==n,*s=el(x))F(n,*--s=ii(x,n-1-i)))                                //unlist    |bL,n  |.. x -> .. x[0] x[1]   |
  C(bj,UC n=*b++;b+=n)                                                                              //jump      |bj,n  |.. x -> ..             |PC+:n
  C(bz,UC n=*b++;b+=n*!tru(*s++))                                                                   //branch    |bz,n  |.. x -> ..             |if x is falsy, PC+:n
  C(bo,*--s=xR)                                                                                     //recur     |bo    |.. -> .. o             |o is the current lambda
  C(bP,mr(*s++))                                                                                    //pop       |bP    |.. x -> ..             |
  C(bV,UC i=*b++;U(*s=v2[*b++](xA[i+OFF],*s)))                                                      //const dyad|bV,i,d|.. x -> .. r           |r:dyads[d][consts[i];x]
  D(*--s=_R(xA[c-bc+OFF]))))                                                                        //const     |bc+i  |.. -> .. consts[i]     |
 l:d--;A u=*s;MS(l+nl,0,s-l-nl+1<<3);F(L(l),A x=l[i];I(x,mr(x)))I(!u,eS(xx,(UC)_C(xz)[(C*)b-1-_C(xy)]))u)

#define Nr(a...) {I r_=cr(a);P(r_-OK,r_);}                                                          //compile rvalue; return on error
#define Nl(a...) {I r_=cl(a);P(r_-OK,r_);}                                                          //compile lvalue; return on error
#define OK -1                                                                                       //returned by cl() and cr() on success
#define MB 256                                                                                      //max bytecode size
#define M(a) {b[nb]=a;m[nb]=o;nb+=nb<MB-1;}                                                         //append byte
Z A u;Z UC b[MB],m[MB],lu[16];Z I nb,nl,l[16],cr(A,B);                                              //u:lambda(src;b:bytes;m:map;l:locals;consts..)  lu:last usages
ZN I li(I v)_(U i=fI(l,nl,v);P(i==nl,-1)lu[i]=nb;i)                                                 //index of a local variable (returns -1 if not found)
Z B cm(A x/*0*/){X(Rv(!xv)Ru(1)RS(P(xn-1,0)S s=su(*xI);U n=SL(s);n&&s[n-1]==':')R_(0))}             //is x a valid modifier? i.e. :: or primitive monad or symbol ending with ":"
Z V cc(A x/*0*/,I o){U n=un,i=OFF;W(i<n&&!mtc_(x,ua),i++)I(i>=n,PSH(u,xR))M(i+bc-OFF)}              //append a "load constant" instruction
Z I cl(A x,A y/*00*/,B r){Q(cm(xx))I v=_v(xx),o=xo;                                                 //compile lvalue (x:assignmentNode,y:tree,r:wantResult)
 Y(R_(o)
   RS(I(yn==1,I w=*yI,i=li(w);P(xx==av&&nl,I(i<0,i=nl;P(i>15,o)l[nl++]=w;lu[i]=nb)M(bs+i)I(r,M(bg+i))OK)P(i>=0,M(bm)M(i)M(v)I(r,M(bg+i))OK))
      UC i=gi(y);M(v?bM:bS)M(i)I(v,M(v))I(r,M(bG)M(i))OK)
   RA(I n=yn-1;P(!n||n>8u,o)A z=yx;P(z==MKL&&(xx==av||_t(xx)==tu),M(bL)M(n)F(n,Nl(x,yA[i+1],0))I(r,P(xx-av,o))E(M(bP))OK)
      ZS(F(n,Nr(yA[n-i],1))M(bl)M(n)I i=zn-1?-1:li(*zI);I(i>=0,M(r?by:bx))E(i=gi(z);M(r?bY:bX))M(i)M(v)OK)o))}
Z I cr(A x/*0*/,B r)_(I o=xo;                                                                       //compile rvalue (x:tree,r:wantResult)
 XS(I i=xn-1?-1:li(*xI);I(i>=0,M(bg+i))J(xn==1&&*xI=='o',M(bo))E(M(bG)M(gi(x)))I(!r,M(bP))OK)       // x.y      variable (possibly qualified)
 P(!xtA||!xn,I(r,cc(x-GAP?x:au,o))OK)                                                               // 0        constant
 U n=xn;A y=xx;                                                                                     //
 P(y==GAP,F(n-1,Nr(xA[i+1],i==n-2&&r))OK)                                                           // [x;y]    block
 P(n==1,I(r,cc(y,o))OK)                                                                             // `a       quoted
 P(n==3&&cm(y)&&_tsSA(xy),
  YS(Nr(xz,1);Nr(xy,1);A z=enl(cS(drp(-1,str(ii(y,0)))));Nr(z,1);mr(z);M(ba)M(2)z=aA1(au);Nl(z,xy,r);z(0);OK)
  Nr(xz,1);Nl(x,xy,r);OK)// x[y]+:z     assignment
 P(n>3&&(y==av||y==DLR),n--;I p[n];A*a=xA;F(n&~1,Nr(*++a,1);M(i&1?bj:bz)p[i]=nb;M(0))               // :[x;y;z] cond
  Nr(n&1?*++a:au,1);F(n&~1,I d=(i&1?nb-1:p[i+1])-p[i];I(i&1,I j=(n&~1)-1;W(i<j&&d>255,d=p[j]-1-p[i];j-=2))P(d>255,o)b[p[i]]=d)I(!r,M(bP))OK)
 I(n==2&&y==FIR,A z=xy;I(ztA&&zn==2,P(zx-REV<3u,Nr(zy,1);M(bu+zx-REV+LAS-au)I(!r,M(bP))OK)))        // *|x      recognized idioms
 I p=0;F(n-1,A z=xA[n-1-i];I(z-GAP,Nr(z,1))E(p=1;cc(GAP,o)))I(p,Nr(xx,1);M(bp)M(n-1))               // x[y;]    projection
 J(y==MKL,n--;P(n>255u,o)M(bl)M(n))                                                                 // (x;y)    list
 J(n==2&&ytu,M(bu+yv))                                                                              // +x       monad
 J(n==3&&ytv,I(!p&&!_tSA(xy),Q(b[nb-1]>=bc);I i=b[nb-1]-bc;b[nb-1]=bV;M(i)M(yv))E(M(bv+yv)))        // x+y      dyad
 E(P(n>9,o)Nr(xx,1)M(ba)M(n-1))                                                                     // x[y]     application
 I(!r,M(bP))OK)
A1(qte,/*1*/xtS||xtA?aA1(x):x)                                                                      //quote
Z A2(c2,/*00*/P(xtw&&!ytSA,1)/*P(x==TIL&&ytZ&&yn<4,F(yn,P(gl(ii(y,i))>100u,0))1)*/0)                //constant folding
Z A3(c3,/*000*/P(ADD<=x&&x<=MUL&&ytzZ&&ztzZ&&(ytt||ztt||yn==zn)&&MAX(xN,yN)<101,1)0)                //constant folding
Z A1(cf,P(!xtA||!xn,x)P(xx==MKL,F(xn,A y=xa;YSA(x))qte(N(drp(1,x))))P(xn==2?c2(xx,xy):xn==3?c3(xx,xy,xz):0,qte(N(val(x))))A y=rsz(xn,au);F(xn,ya=cf(xa);xa=au;P(!ya,die("CF")))AO(xo,x(y)))
Z I mxs(I i,I s)_(I r=s;W(1,UC c=MIN(bc,b[i++]);r=MAX(r,s);P(!c,r)s+=ds[c]+ks[c]*b[i];i+=di[c]+(c==bj)*b[i];I(c==bz,r=MAX(r,mxs(i+b[i-1],s))))r)//max stack
Z B shy(A x/*0*/)_(!xtA?0:xn&&xx==GAP?shy(xA[xn-1]):xn==3&&cm(xx)&&_tSA(xy))                        //is last expr an assignment?
A3(cpl,/*111*/nb=1;MS(lu,-1,SZ lu);I k=0;I(z,k=zn;MC(l,zV,OFF*k);z(0))nl=k;u=aA(OFF);y=Nx(cf(y));ux=x;uy=uz=uA[3]=au;B s=shy(y);I r=cr(y,!s);y(0);P(r-OK,ec0();eS(ux,r);u(0))
 I o=0;I(s,cc(au,o))P(un>255||nb>MB-2||nl>L(l)-2,ez0();eS(ux,0);u(0))M(bu)P(nb>MB-2||un>255-bc+OFF,eS(ux,0);u(0);ez0())
 F(nl,I j=lu[i];I(j>=0&&b[j]==bg,b[j]=bd))*b=mxs(1,0);*m=-1;uy=aCn(b,nb);uz=aCn(m,nb);uA[3]=aV(tS,nl,l);AK(k,AT(to,u)))
#undef M
