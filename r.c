#include<math.h> // ngn/k, (c) 2019-2024 ngn, GNU AGPLv3 - https://codeberg.org/ngn/k/raw/branch/master/LICENSE
#include"a.h"
///prng: xoshiro256+ (public domain) http://vigna.di.unimi.it/xorshift/ seeded with the central column of rule30, little-endian:
//s:2/|+4 64#(n{(|(8#2)\30)@2/'3':0,x,0}\n=!2*n)@'n:256
//R:{,/|(0,(#y)!x)_y} /rotate
//X:{2/~=/(64#2)\'(x;y)} /xor
//f:{r:+/s 0 3;t:s[1]*(*/17#2);s[2]:X/s 2 0;s[3]:X/s 3 1;s[1]:X/s 1 2;s[0]:X/s 0 3;s[2]:X[s 2]t;s[3]:2/R[45]@(64#2)\s 3;r} /next
//d:|'(64#2)\'256/'256!(0x76e15d3efefdcbbf;0xc5004e441c522fb3;0x77710069854ee241;0x39109bb02acbe635)
//l:{r:s;t::&4;{$[x;t::t X's;];f[]}''d;s::t;r} /long jump
//`0:{"{",(","/x),"},"}'{"0x",`hex@`c$(8#256)\x}''+l'!4
#define M 4
Z W s[][M]={{0xd5a986ae75c9a33b,0x9c57a73dcd5e41b7,0x3fe497b4dd1be68d,0x3f57adc392affdef},{0x1016d8e3483a8f0f,0xcb0c33c0e78feede,0x7b5dda788f9f577d,0xf1e01f806161118a},
 {0x81f9e6260eb8e5df,0x5943e008d9222efa,0x8f514f6e6fb18ba4,0x6dacfe2135f9599e},{0xfa9b718d8d0769bf,0x4d46d3d50833e8c9,0x696678daaa7b4cc6,0x3cb5c708d53cc982}};//prng state
Z W b[M];Z U nb;//buf
X1(prng,Ru(aV(tL,4*M,s))REGHIL(P(xn-4*M,el(x))MC(s,xV,SZ s);nb=0;x(au))Ril(W v=gl(x);I(!v,v=now())F(4,Fj(M,s[i][j]=v=v*6364136223846793005+1442695040888963407/*knuth mmix*/))au)R_(et(x)))
Z V h(U x,U y){F(M,s[x][i]^=s[y][i])}
Z V r4(){nb=M;W t[M];F(M,b[i]=s[0][i]+s[3][i])F(M,t[i]=s[1][i]<<17)h(2,0);h(3,1);h(1,2);h(0,3);F(M,s[2][i]^=t[i])F(M,s[3][i]=(s[3][i]<<45|s[3][i]>>19))}//next 4*64 bits
Z W r()_(I(nb<4,r4())b[--nb])//random 64 bits
Z U ri(W m)_((U)r()*m>>32)//random int mod m
Z W rw(W m)_(m>>32?r()%m:ri(m))
Z F rf()_(W v=1023ll<<52|(r()&-1ull>>12);-1+*(F*)&v)//random float 0..1

Z A rt(U n,C t)_(A x=an(n,t);F(((W)n<<Tw[t])+255>>8,r4();MC(xV+(i<<5),b,32))x)//roll n full-range (including negative) items of type t (int of a specific width or char)
Z CO W msk[]={0xffffffffffffffffll,0x5555555555555555ll,0x1111111111111111ll,0x0101010101010101ll,0x0001000100010001ll,0x0000000100000001ll,0x0000000000000001ll};
Z A ro(U n,W m)_(P(!(m&m-1),P(!m,rt(n,tL))C t=tZ(m-1);A x=rt(n,t);L v=(m-1)*msk[Tw[t]];L*p=xL;F((n<<Tw[t])+255>>8,Fj(4,*p++&=v))x)
 C t=tZ(m-1);A x=an(n,t);S4(t-tG,F(n,xg=ri(m)),F(n,xh=ri(m)),F(n,xi=ri(m)),F(n,xl=r())I(m,F(n,xl%=m)))x)//roll
Z A ded(U n,U m)_(A x=an(m,tZ(m));tilV(xV,0,m,xw-3);S4(xw-3,F(n,U j=ri(m-i)+i;SW(xG[i],xG[j])),F(n,U j=ri(m-i)+i;SW(xH[i],xH[j])),F(n,U j=ri(m-i)+i;SW(xI[i],xI[j])),)rsz(n,x))//dense deal
Z A des(U n,W m)_(A x=aL(n);L*a=xL;F(n,a[i]=i)W t=1ll<<(66-CLZ(n));A y=aL(t);L*h=yL;L*v=h+1;MS(h,0,t<<3);//sparse
 W b=t-2;My(F(n,W j=rw(m-i)+i;
  I(j<(W)n,SW(a[i],a[j]))
  E(W p=2*j;I s=j;W(1,I hp=h[p&=b];B(!hp,h[p]=j)B((W)hp==j,s=v[p])p+=2)v[p]=a[i];a[i]=s)))ct(tZ(m),x))//linear probe
Z A de(U n,W m)_(n>m?el0():!(m>>31)&&n+64>m>>4?ded(n,m):des(n,m))//deal
Z A rd(L n,L m)_(m<0?(n<0?ed0():rt(n,tZ(m))):n==NL?rd(-m,m):n-(I)n?ez0():n<0?de(-n,m):ro(n,m))//roll or deal
A rndF(L n)_(P(n<0,ed0())A x=aF(n);F(n,xf=rf())x)//random floats 0..1
A rnd(L n,A x)_(X(Ril(rd(n,gl(x)))Rc(P((32|xv)=='a',cC(add(x,Nx(rd(n,26)))))rt(n,tC))Rf(x(mul(x,Nx(rndF(n)))))RMT(x(x1(Nx(rd(n,xN)))))R_(et(x)))0)
