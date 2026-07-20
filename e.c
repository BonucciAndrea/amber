#include"a.h" // ngn/k, (c) 2019-2024 ngn, GNU AGPLv3 - https://codeberg.org/ngn/k/raw/branch/master/LICENSE
Z C b[4096],*r=b;Z U d;
NI A err0(S s)_(r=b;d=0;N n=MIN(SL(s),32);r=b;*r++='\'';MC(r,s,n);r+=n;*r++=10;0)
ZN A err1(A x,S s)_(x(0);err0(s))
ZN A err8(CO A*a,U n,S s)_(mrn(n,a);err0(s))
NI V eQ(S s,U n,U i){
 I(++d>=5,I(d==5,MC(r," ..\n",4);r+=4)return)
 S p=s+i,q=p,t=p;U h=64,o=1;
 W(s<p&&t-h<p&&p[-1]&&p[-1]-10,p--;o+=(*p&0xc0)!=0x80)
 W(q<s+n&&q<=t+h&&*q&&*q-10,q++)
 *r++=32;MC(r,p,q-p);
 I(p<=t-h,*r=r[1]='.')
 I(q>t+h,r[q-p-2]=r[q-p-1]='.')
 r+=q-p;*r++=10;MS(r,32,o);r+=o;*r++='^';*r++=10;}
NI V eS(A x/*0*/,U i)_(eQ(xV,xn,i))
A3(try,/*100*/x=x(dot(x,yR));P(x,x)I(ztU,z=z1(aCn(b,r-b)))E(zR)r=b;d=0;z)
A1(epr,write(2,b,r-b);r=b;x)
A1(err,XC(x=str0(x);err1(x,xV))P(x==au,aCn(b,r-b))err1(x,"err"))
NI A die(S s)_(U n=SL(s);C v[n+1];MC(v,s,n);v[n]=10;write(1,"'",1);write(2,v,n+1);exit(1);0)

#define M(t,m)\
 NI A0(e##t##0,err0(    #m))\
 NI A1(e##t   ,err1(x,  #m))\
 NI AA(e##t##8,err8(a,n,#m))
ERR
