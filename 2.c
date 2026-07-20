#include"a.h" // ngn/k, (c) 2019-2024 ngn, GNU AGPLv3 - https://codeberg.org/ngn/k/raw/branch/master/LICENSE
Z CO W msk[]={0x8080808080808080ll,0x8000800080008000ll,0x8000000080000000ll};
#define M(o,f,T) ZN V f(CO V*RES a,CO V*RES b,V*RES c,U n){CO T*x=AL(a),*y=AL(b);T*r=AL(c);F(n,Fj(32/SZ(T),*r++=*x++o*y++))}
 M(+,aFF,F)M(*,mFF,F)M(/,dFF,F)M(+,aLL,L)M(+,aII,I)M(+,aHH,H)M(+,aGG,G)
#undef M
ZN A amdFF(A x,A y,U f)_(U n=xn;P(n-yn,el(y))A z=MINE(y)?y:aF(n);G(&aFF,0,mFF,dFF)[f-1](xV,yV,zV,n+3>>2);y-z?y(z):z)
ZN B oZZ(CO W*x,CO W*y,CO W*r,U n,U w)_(x=AL(x);y=AL(y);r=AL(r);W t[4]={};F(((W)n<<w)+31>>5,Fj(4,t[j]|=(*r^*x)&(*r^*y);r++;x++;y++))!!((t[0]|t[1]|t[2]|t[3])&msk[w]))
Z A addZZ(A x,A y,U f)_(U w=MAX(xw-3,yw-3);P(xw-3-w,x=ct(tG+w,xR);x(addZZ(x,y,f)))y=ct(tG+w,y);U n=yn;A z=an(n,yt);
 G(&aGG,aHH,aII,aLL)[w](xV,yV,zV,n+(31>>w)>>5-w);
 P(w<3&&oZZ(xV,yV,zV,n,w),z(0);y=ct(tG+w+1,y);x=ct(tG+w+1,xR);x(addZZ(x,y,f)))y(z))
Z A mulZZ(A x,A y,U f)_(U n=yn,i=0,w=MAX(xw-3,yw-3);P(xw-3-w,x=ct(tG+w,xR);x(mulZZ(x,y,f)))y=ct(tG+w,y);A z=an(n,yt);
 S4(w,W(i<n,I v=(I)xg*yg;B(v-(G)v)zg=v;i++),
      W(i<n,I v=(I)xh*yh;B(v-(H)v)zh=v;i++),
      W(i<n,L v=(L)xi*yi;B(v-(I)v)zi=v;i++),
      W(i<n,zl=xl*yl;i++))
 P(i<n,z(0);x=ct(tG+w+1,xR);x(mulZZ(x,ct(tG+w+1,y),f)))y(z))

#define M(f,T) ZN V f(L v,V*RES b,V*RES c,U n){CO T*y=AL(b);T*r=AL(c);F(n,Fj(32/SZ(T),*r++=v+*y++))}
 M(alL,L)M(aiI,I)M(ahH,H)M(agG,G)
#undef M
ZN B ozZ(L v,CO W*y,CO W*r,U n,U w)_(r=AL(r);y=AL(y);F(3-w,v|=v<<(8<<w+i))W t[4]={};F(((W)n<<w)+31>>5,Fj(4,t[j]|=(*r^v)&(*r^*y);r++;y++))!!((t[0]|t[1]|t[2]|t[3])&msk[w]))
Z A addzZ(L v,A y,U f)_(U n=yn,w=MAX(tZ(v)-tG,yw-3);y=ct(tG+w,y);A z=an(n,yt);G(&agG,ahH,aiI,alL)[w](v,yV,zV,n+(31>>w)>>5-w);
 P(w<3&&ozZ(v,yV,zV,n,w),z(0);y=ct(tG+w+1,y);addzZ(v,y,f))y(z))

Z A mulzZ(L a,A y,U f)_(U n=yn,i=0,w=MAX(tZ(a)-tG,yw-3);y=ct(tG+w,y);A z=an(n,yt);
 S4(w,W(i<n,I v=a*yg;B(v-(G)v)zg=v;i++),
      W(i<n,I v=a*yh;B(v-(H)v)zh=v;i++),
      W(i<n,L v=a*yi;B(v-(I)v)zi=v;i++),
      W(i<n,zl=a*yl;i++))
 i<n?mulzZ(a,ct(tG+w+1,z(y)),f):y(z))

Z A modzZ(L m,A y,U f)_(P(!m,y)
 P(m<0,m=-m;A z=an(yn,yt);S4(yw-3,F(zn,C v=yg;zg=v<0?-1-~v/m:v/m),F(zn,H v=yh;zh=v<0?-1-~v/m:v/m),F(zn,I v=yi;zi=v<0?-1-~v/m:v/m),F(zn,L v=yl;zl=v<0?-1-~v/m:v/m))y(z))
 P(m&m-1,A z=an(yn,tZ(m));U wy=yw-3;S4(zw-3,F(zn,zg=(iw(y,wy,i)%m+m)%m),F(zn,zh=(iw(y,wy,i)%m+m)%m),F(zn,zi=(iw(y,wy,i)%m+m)%m),F(zn,zl=(iw(y,wy,i)%m+m)%m))y(z))
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
Z A addfF(F v,A y,U f)_(A z=MINE(y)?y:aF(yn);U n=zn+3&-4;F(n,zf=v+yf)y-z?y(z):z)
Z A mulfF(F v,A y,U f)_(A z=MINE(y)?y:aF(yn);U n=zn+3&-4;F(n,zf=v*yf)y-z?y(z):z)
Z A admfF(F v,A y,U f)_((f==3?mulfF:addfF)(v,y,f))
Z A dvdfF(F v,A y,U f)_(A z=MINE(y)?y:aF(yn);U n=zn+3&-4;F(n,zf=v/yf)y-z?y(z):z)
Z A dvdFf(A x,F v,U f)_(A z=aF(xn);F(xn,zf=xf/v)z)
Z A dvdzZ(L v,A y,U f)_(dvdfF(v,cF(y),f))
Z A dvdZZ(A x,A y,U f)_(x=cF(xR);x(amdFF(x,cF(y),f)))
Z A arizz(L a,L b,U f)_(P(f==4,af((F)a/b))az(f==1?a+b:f==3?a*b:f==5?(!a?b:a<0?(b<0?-1-~b/-a:b/-a):(b%a+a)%a):f==6?MIN(a,b):f==7?MAX(a,b):f==8?a<b:f==9?a>b:f==10?a==b:0))
Z A arizZ(L v,A y,U f)_(A(&addzZ,0,mulzZ,dvdzZ,modzZ,mmmzZ,mmmzZ,cmpzZ,cmpzZ,cmpzZ)[f-1](v,y,f))
Z A ariZZ(A x,A y,U f)_(P(xn-yn,el(y))A(&addZZ,0,mulZZ,dvdZZ,0,mmmZZ,mmmZZ,cmpZZ,cmpZZ,cmpZZ)[f-1](x,y,f))
ZN A ariz(A x,A y,U f){S(xtT<<1|ytT,R(0,arizz(gl_(x),gl(y),f))R(1,arizZ(gl_(x),y,f))R(2,P(f==4,ari(x,cF(y)))arizZ(gl(y),xR,f-8<2u?f^8^9:f))R_(ariZZ(x,y,f)))}
ZN A arif(A x,A y,U f)_(C t=xt,u=yt;
 P(f==5,xtz?modzf(gl(x),y,f):et(y))
 P(t-tf&&t-tF,x=Ny(cF(xR));x(ari(x,y)))
 P(u-tf&&u-tF,ari(x,N(cF(y))))
 P(f<5,U k=(t<tM)<<1|(u<tM);S(k,
  R(0,F a=*xF,b=gf(y);af(f==1?a+b:f==3?a*b:a/b))
  R(1,f<4?admfF(*xF,y,f):dvdfF(*xF,y,f))
  R(2,f<4?admfF(gf(y),xR,f):dvdFf(x,gf(y),f))
  R_(amdFF(x,y,f)))0)
 P(f==10,ariz(x,y,f))
 x=of1(xR);y=ari(x,of1(y));x(f<8&&y?of0(y):y))

Z U f;//0=dex,1=add,2=sub,3=mul,4=dvd,5=mod,6=mnm,7=mxm,8=ltn,9=gtn,10=eql
A2(ari,C t=xt,u=yt;U v=1<<t|1<<u;
 P(!(v&~(1<<tG|1<<tH|1<<tI|1<<tL|1<<tC|1<<ti|1<<tl|1<<tc)),ariz(x,y,f))
 P(v&(1<<tm|1<<tM|1<<tA),e2(av+f,x,y))
 P(t==tB,x=cG(xR);x(ari(x,y)))
 P(u==tB,ari(x,cG(y)))
 P(t==tE,P(f==1&&ytzc,addzE(gl(y),xR))x=gZ(xR);x(ari(x,y)))
 P(u==tE,P(f==1&&xtzc,addzE(gl_(x),y))ari(x,gZ(y)))
 P(v&(1<<tf|1<<tF),arif(x,y,f))
 I(f-8<3u,
  P(v&1<<tS,P(f==10&&!(v&~(1<<ts|1<<tS)),ariz(x,y,f))e2f(ari,x,y))
  P(v&1<<ts,v^1<<ts?et(y):ai(f==8?qA(x,y)<0:f==9?qA(x,y)>0:xv==yv)))
 et(y))

#define M(s,i) A2(s,U o=f;f=i;x=ari(x,y);f=o;x)
 M(add,1)M(mul,3)M(dvd,4)M(mod,5)M(mnm,6)M(mxm,7)M(ltn,8)M(gtn,9)M(eql,10)
#undef M
A2(dex,y)A2(sub,add(x,N(neg(y))))X2(exc,RMT(ytm||rnk(x)<0?ed(y):ytt?exc(x,rsz(xN,y)):xN-yN?el(y):am(xR,y))Rs(x=rsz(yN,x);x(exc(x,y)))Rilc(mod(x,y))R_(et(y)))
