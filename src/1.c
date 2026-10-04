#include<math.h> // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include"a.h"
X1(neg,RE(neg(gZ(x)))Rilc(az((L)(0-(W)gl(x))))Rf(af(-gf(x)))RC(neg(cG(x)))RmMA(e1f(neg,x))RB(neg(cG(x)))R_(et(x))
 RGHILF(U n=xn;I(xw-3<3&&minfZ(0,x)==(L)(~0ull<<((8<<(xw-3))-1)),x=ct(tH+xw-3,x))A y=MINE(x)?x:an(n,xt),z=x-y?x:au;_at(y)=0;n=((n<<xw-3)+31&~31)>>xw-3;
  Mz(X(C(tG,F(n,yg=-xg))C(tH,F(n,yh=-xh))C(tI,F(n,yi=(I)(0u-(U)xi)))C(tL,F(n,yl=(L)(0-(W)xl)))D(F(n,yf=-xf))))y))
X1(not,RmMA(e1f(not,x))RU(x(ai(x==au)))
 RB(x=mut(x);W*a=xV;F((xn+255&-256)>>6,*a++^=-1)x)R_(eql(xtsS?as(0):ai(0),x)))
// amber 2.7: null of a float vector on the thread pool from 1M items (one byte out per item)
#include"parallel.h"
TD struct{CO F*p;G*o;U n;int nt;}NJ;
Z V njw(V*c_,int t){NJ*c=c_;U s=(U)((W)c->n*t/c->nt),e=(U)((W)c->n*(t+1)/c->nt);CO F*RES p=c->p;G*RES o=c->o;for(U i=s;i<e;i++)o[i]=p[i]!=p[i];}
Z A nulF(A x){U n=xn;A y=aG(n);int nt=n<(1u<<20)?1:par_thread_count(n);if(nt>(int)(n>>18))nt=(int)(n>>18);
 I(nt<2,G*RES o=_V(y);CO F*RES p=xF;F(n,o[i]=p[i]!=p[i]))E(NJ c={xF,(G*)_V(y),n,nt};par_run(nt,njw,&c));x(y);return y;}
X1(nul,RmMA(e1f(nul,x))RU(x(ai(x==au)))RB(whr(len(x)))RF(nulF(x))Rf(x(ai(*xF!=*xF)))R_(eql(cn[xt],x)))
X1(flr,RmMA(e1f(flr,x))RcC(K1("{`c$x+32*~\"A[\"'x}",x))RsS(cS(flr(str(x))))RilEBGHIL(x)RfF(A y=an(xn,xt+tl-tf);L o=0;Mx(F(yn,F v=xf;L b=__builtin_fabs(v)<0x1p63;o|=b^1;yl=(L)__builtin_floor(b?v:0))I(o,F(yn,F v=xf;B b=__builtin_fabs(v)<0x1p63;L r=(L)__builtin_floor(b?v:0);yl=b?r:v>0?WL:NL)))y)R_(et(x)))

// Amber 2.5 (exp): `abs (amber.k: abs:{`abs x}). It was {x*signum x}, five passes; one here, the same bits:
// A float flips its sign only below 0, a NaN and a -0.0 stay as they were (NaN*-1 is that NaN, -0.0*0 is -0.0);
// an int vector is |x| at its width, one wider if it holds the width's least value (the multiply overflowed and
// redid it wider, as for neg), and 64-bit 0N stays 0N (it wraps). The sign is flipped on the bits so that no
// -fno-signed-zeros fold can turn the test into a fabs. Anything else goes the old way.
Z F fabs1(F v)_(W u;MC(&u,&v,8);u^=(W)(v<0.0)<<63;MC(&v,&u,8);v)
X1(kabs,Ril(L v=gl(x);az(v<0?(L)(0-(W)v):v))Rf(af(fabs1(gf(x))))
 RGHILF(I(xt==tF,A y=MINE(x)?x:an(xn,tF);_at(y)=0;CO F*p=xF;W*q=(W*)_V(y);
   F(xn,F v=p[i];W u;MC(&u,&v,8);u^=(W)(v<0.0)<<63;q[i]=u)return x-y?x(y):y;)
  I(xw-3<3&&minfZ(0,x)==(L)(~0ull<<((8<<(xw-3))-1)),x=ct(tH+xw-3,x))A y=MINE(x)?x:an(xn,xt);_at(y)=0;
  S4(xw-3,F(xn,G v=xg;yg=v<0?(G)-v:v),F(xn,H v=xh;yh=v<0?(H)-v:v),F(xn,U v=(U)xi,m=(U)(xi>>31);yi=(I)((v^m)-m)),F(xn,W v=(W)xl,m=(W)(xl>>63);yl=(L)((v^m)-m)))
  x-y?x(y):y)
 R_(K1("{x*(x>0)-x<0}",x)))
#define M(k,f) X1(k,RfF(A y=MINE(x)?x:an(xn,xt);_at(y)=0;F(xn+3&~3,yf=f(xf));x-y?x(y):y)RmMA(e1f(k,x))R_(k(N(cF(x)))))
M(ksin,sin)M(kcos,cos)M(klog,log)M(kexp,exp)M(sqr,SQ)
