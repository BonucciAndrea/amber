/* the POSIX socket, poll and time names, hidden under a strict -std=c99 unless asked for before the first system header */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE
#endif
#if !defined(wasm)
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<time.h>
#include<netinet/in.h>
#include<netinet/tcp.h>
#include<netdb.h>
#include<poll.h>
#include<errno.h>
#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#endif
#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
Z V qerrp(V);
/* qipc.c - q's wire format (kdb+ IPC), 2.9.0: -8!x and -9!y as q's bytes, and the messages a handle sends and takes.
 *
 * A message is an 8-byte header and a value. The header: 1 (little-endian), the message type (0 async, 1 sync, 2 a
 * response), 1 if the rest is compressed, 0, then the whole message's length (int32). The value: its type byte,
 * negative for an atom (-7 a long, -11 a symbol, -14 a date, ..), 0..19 for a list (then an attribute byte and an
 * int32 count), 98 a table (an attribute byte and the dict of its columns), 99 a dict (keys, then values), 100 a
 * lambda (its context symbol and its text), 101 :: (and q's other unary primitives, by index), -128 an error (its
 * text). A symbol is its name and a NUL. q's attributes are Amber's: 1 `s, 2 `u, 3 `p, 4 `g.
 *
 * Amber to q: an int, of any width Amber keeps it at, is a long (7); a bit list a boolean list (1); a range its ints;
 * the temporal kinds their own (12..19). q to Amber: a boolean is an int (a list of them a bit list); a byte, short or
 * int an int, its null 0N; a real a float; a datetime a timestamp. A guid, an enumeration and q's derived functions
 * are 'nyi; a lambda is its text, compiled here.
 *
 * -9! reads untrusted bytes: every read is bounds checked, the depth is limited, and bad bytes are 'domain.
 */
V*realloc(V*,N);V free(V*);
#define QDEPTH 256
#define QNAN 0xfff8000000000000ull   //q's 0n
typedef struct{UC*p;N n,cap;I bad;}QW;
Z V qput(QW*b,CO V*s,N k){I(b->bad,return)I(b->n+k>b->cap,N c=b->cap?b->cap:256;W(c<b->n+k,c+=c)UC*q=realloc(b->p,c);I(!q,b->bad=1;return)b->p=q;b->cap=c)
 MC(b->p+b->n,s,k);b->n+=k;}
Z V qb(QW*b,UC v){qput(b,&v,1);}
Z V qi(QW*b,I v){qput(b,&v,4);}
Z V ql(QW*b,L v){qput(b,&v,8);}
Z V qs(QW*b,U id){S s=su(id);qput(b,s,SL(s)+1);}
Z V qhd(QW*b,C t,UC a,U n){qb(b,(UC)t);qb(b,a);qi(b,(I)n);}
// a list type's q code (0: none); its atom's is the negative
Z C qcode(UC t)_(t==tB?1:LH(tE,t,tL)?7:t==tF?9:t==tC?10:t==tS?11:t==tNP?12:t==tMO?13:t==tDT?14:t==tSP?16:t==tMN?17:t==tSC?18:t==tTM?19:0)
Z V qenc(QW*b,A x,I d);
Z V qencv(QW*b,A x,I d){UC t=xt;U n=xn;UC a=(UC)_at(x);
 I(t==tA,qhd(b,0,a,n);F(n,qenc(b,xa,d+1))return)
 I(t==tE,A y=gZ(xR);qenc(b,y,d);mr(y);return)
 C c=qcode(t);I(!c,b->bad=2;return)qhd(b,c,a,n);
 S(t,
  C(tB,F(n,qb(b,(UC)(xG[i>>3]>>(i&7)&1))))
  C(tG,F(n,ql(b,(L)xg)))C(tH,F(n,ql(b,(L)xh)))C(tI,F(n,ql(b,(L)xi)))
  C(tS,F(n,qs(b,(U)xi)))
  C(tF,F(n,W v;MC(&v,&xf,8);ql(b,(L)(xf!=xf?QNAN:v))))   //a NaN as q's 0n, its sign bit set
  D(qput(b,xV,(N)((W)n<<Tw[t]>>3))))}   //long, float, char and the temporal kinds: their bytes as they are
Z V qenc(QW*b,A x,I d){I(b->bad,return)I(d>QDEPTH,b->bad=2;return)
 I(x==au,qb(b,101);qb(b,0);return)   //::
 UC t=xt;
 I(t<tM,qencv(b,x,d);return)
 I(t==tM,qb(b,98);qb(b,0);qb(b,99);qenc(b,xx,d+1);qenc(b,xy,d+1);return)
 I(t==tm,qb(b,_at(x)==1?127:99);qenc(b,xx,d+1);qenc(b,xy,d+1);return)
 I(t==to,A s=kst(xR);I(!s,b->bad=1;return)qb(b,100);qb(b,0);qencv(b,s,d);mr(s);return)   //a lambda: no context, its text
 S(t,
  C(ti,qb(b,(UC)-7);ql(b,(L)xv))C(tl,qb(b,(UC)-7);ql(b,*xL))C(tf,qb(b,(UC)-9);F f=*xF;W v;MC(&v,&f,8);ql(b,(L)(f!=f?QNAN:v)))
  C(tc,qb(b,(UC)-10);qb(b,(UC)xv))C(ts,qb(b,(UC)-11);qs(b,(U)xv))
  C(tdt,qb(b,(UC)-14);qi(b,(I)x))C(ttm,qb(b,(UC)-19);qi(b,(I)x))C(tmn,qb(b,(UC)-17);qi(b,(I)x))C(tsc,qb(b,(UC)-18);qi(b,(I)x))C(tmo,qb(b,(UC)-13);qi(b,(I)x))
  C(tnp,qb(b,(UC)-12);ql(b,*xL))C(tsp,qb(b,(UC)-16);ql(b,*xL))
  D(b->bad=2))}
// x as q's bytes, behind a message header of type m (or none: -8! has one too, of type 0, as q's)
A qser(A x/*0*/,UC m){QW b={0,0,0,0};UC h[8]={1,m,0,0,0,0,0,0};qput(&b,h,8);qenc(&b,x,0);
 I(b.bad,I(b.p,free(b.p))return b.bad==2?et0():eo0())I n=(I)b.n;MC(b.p+4,&n,4);A r=aV(tC,(U)b.n,b.p);free(b.p);return r;}

/* ---- the other way ------------------------------------------------------- */
typedef struct{CO UC*p;N n,i;I bad;}QR;
Z CO UC*qtake(QR*b,N k){I(b->bad||k>b->n-b->i,b->bad=1;return 0)CO UC*q=b->p+b->i;b->i+=k;return q;}
Z UC qrb(QR*b){CO UC*q=qtake(b,1);return q?*q:0;}
Z I qri(QR*b){CO UC*q=qtake(b,4);I v=0;I(q,MC(&v,q,4))return v;}
Z L qrl(QR*b){CO UC*q=qtake(b,8);L v=0;I(q,MC(&v,q,8))return v;}
Z U qrs(QR*b){CO UC*q=b->p+b->i;N k=0;W(b->i+k<b->n&&q[k],k++)I(b->i+k>=b->n,b->bad=1;return 0)C t[256];U r;
 I(k<SZ t,MC(t,q,k);t[k]=0;r=us(t))E(C*u=realloc(0,k+1);I(!u,b->bad=1;return 0)MC(u,q,k);u[k]=0;r=us(u);free(u))b->i+=k+1;return r;}
Z UC qkind(C c)_(c==12?tnp:c==13?tmo:c==14?tdt:c==16?tsp:c==17?tmn:c==18?tsc:c==19?ttm:0)   //a q temporal code's Amber atom kind
Z A qdec(QR*b,I d);
// an int list of q's width w (1 2 4 8 bytes; s: 1 for a byte, unsigned, no null), widened, its null 0N, then narrowed as Amber keeps ints
Z A qints(QR*b,U n,U w,B s){CO UC*q=qtake(b,(N)n*w);P(!q,0)A z=aL(n);L*RES o=_L(z);
 S(w,C(1,F(n,o[i]=s?(L)q[i]:(L)(C)q[i]))C(2,F(n,H v;MC(&v,q+2*i,2);o[i]=v==(H)0x8000?NL:v))C(4,F(n,I v;MC(&v,q+4*i,4);o[i]=v==NIT?NL:v))D(MC(o,q,(N)n*8)))
 return sqzZ(z);}
Z A qdecv(QR*b,C t,I d){UC a=qrb(b);I c=qri(b);P(b->bad||c<0,b->bad=1;(A)0)U n=(U)c;A z=0;
 P((W)n>b->n-b->i&&t!=0,b->bad=1;(A)0)   //every item takes a byte at least
 S(t,
  C(0,z=an(n|!n,tA);U k=0;W(k<n,A e=qdec(b,d+1);I(!e,mrn(k,_A(z));AZ(z);mr(z);return 0)_A(z)[k++]=e)I(!n,_A(z)[0]=emp(tC))z=AN(n,z))
  C(1,CO UC*q=qtake(b,n);P(!q,(A)0)A g=an(n,tG);F(n,_G(g)[i]=q[i]!=0)z=cB(g))
  C(4,z=qints(b,n,1,1))C(5,z=qints(b,n,2,0))C(6,z=qints(b,n,4,0))C(7,z=qints(b,n,8,0))
  C(8,CO UC*q=qtake(b,(N)n*4);P(!q,(A)0)z=an(n,tF);F(n,float v;MC(&v,q+4*i,4);_F(z)[i]=v))
  C(9,CO UC*q=qtake(b,(N)n*8);P(!q,(A)0)z=an(n,tF);MC(_V(z),q,(N)n*8))
  C(10,CO UC*q=qtake(b,n);P(!q,(A)0)z=aV(tC,n,q))
  C(11,z=an(n,tS);F(n,U s=qrs(b);I(b->bad,mr(z);return 0)_I(z)[i]=(I)s))
  C(15,CO UC*q=qtake(b,(N)n*8);P(!q,(A)0)z=an(n,tNP);F(n,F v;MC(&v,q+8*i,8);_L(z)[i]=v!=v?NL:(L)(v*86400000000000.0+(v<0?-.5:.5))))   //a datetime: a timestamp
  D(UC k=qkind(t);P(!k,b->bad=3;(A)0)U w=TQN(k)?8:4;CO UC*q=qtake(b,(N)n*w);P(!q,(A)0)z=an(n,TQL(k));MC(_V(z),q,(N)n*w)))
 I(z&&a&&a<5&&!_tP(z)&&_t(z)<tM&&_t(z)-tA,_at(z)=atok(z,a)?a:0)
 return z;}
Z A qdec(QR*b,I d){I(b->bad||d>QDEPTH,b->bad=1;return 0)C t=(C)qrb(b);I(b->bad,return 0)
 I(t<0&&t>-20,   //an atom: every case returns
  S(t,R(-1,ai(qrb(b)!=0))R(-4,ai(qrb(b)))R(-5,CO UC*q=qtake(b,2);P(!q,(A)0)H v;MC(&v,q,2);v==(H)0x8000?al(NL):ai(v))R(-6,I v=qri(b);v==NIT?al(NL):ai(v))
   R(-7,az(qrl(b)))R(-8,CO UC*q=qtake(b,4);P(!q,(A)0)float v;MC(&v,q,4);af(v))R(-9,L v=qrl(b);F f;MC(&f,&v,8);af(f))R(-10,ac((C)qrb(b)))R(-11,U s=qrs(b);as(s))
   R(-15,L v=qrl(b);F f;MC(&f,&v,8);antp(f!=f?NL:(L)(f*86400000000000.0+(f<0?-.5:.5))))
   R_(UC k=qkind((C)-t);P(!k,b->bad=3;(A)0)TQN(k)?AT(k,al(qrl(b))):atq(k,qri(b)))))
 P(t>=0&&t<20,qdecv(b,t,d))
 P(t==98,qrb(b);P(qrb(b)!=99,b->bad=1;(A)0)A k=qdec(b,d+1);P(!k,0)A v=qdec(b,d+1);P(!v,mr(k);(A)0)
   P(_t(k)!=tS||_t(v)!=tA||_n(k)!=_n(v),mr(k);mr(v);b->bad=1;(A)0)aM(k,v))
 P(t==99||t==127,A k=qdec(b,d+1);P(!k,0)A v=qdec(b,d+1);P(!v,mr(k);(A)0)P(_N(k)!=_N(v),mr(k);mr(v);b->bad=1;(A)0)A r=am(k,v);I(t==127&&r&&_t(r)==tm,_at(r)=1)r)
 P(t==100,qrs(b);A s=qdec(b,d+1);P(!s,0)P(_t(s)!=tC,mr(s);b->bad=1;(A)0)val(s))   //a lambda: its text, compiled
 P(t==101,UC i=qrb(b);P(i,b->bad=3;(A)0)au)   //:: (q's other unary primitives by index: 'nyi)
 P(t==-128,U s=qrs(b);P(b->bad,(A)0)b->bad=4;err0(su(s)))   //an error: raised with its text
 b->bad=3;return 0;}

/* q's compression (kdb+ c.java's u): a flag byte says which of the next 8 items are a byte as it is and which a
 * copy of earlier output (an index into a table of the last place each pair of bytes was seen, then a count). The
 * message keeps its header, the length of the whole uncompressed message at byte 8 */
Z A qunz(CO UC*s,N sn){P(sn<12,ed0())I ulen;MC(&ulen,s+8,4);P(ulen<8||ulen>(1<<30),ed0())A z=an((U)ulen,tC);UC*o=(UC*)_V(z);MC(o,s,8);o[2]=0;
 I aa[256]={0};N dd=12,p=8,q=8;UC f=0,i=0;
 W(q<(N)ulen,
  I(!i,I(dd>=sn,mr(z);return ed0())f=s[dd++];i=1)
  I(f&i,I(dd+1>=sn,mr(z);return ed0())N r=(N)aa[s[dd++]];I(r+1>=q,mr(z);return ed0())o[q++]=o[r++];o[q++]=o[r++];N m=s[dd++];I(q+m>(N)ulen,mr(z);return ed0())F(m,o[q+i]=o[r+i])
    W(p<q-1,aa[o[p]^o[p+1]]=(I)p;p++)q+=m;p=q)
  E(I(dd>=sn,mr(z);return ed0())o[q++]=s[dd++];W(p<q-1,aa[o[p]^o[p+1]]=(I)p;p++))
  i=(UC)(i<<1))
 return z;}
// a message's value: its header checked (and its rest uncompressed), then decoded; 0 and an error for bad bytes,
// and a q error (-128) raised with its text
A qdes(A y/*1*/){P(_t(y)-tC,mr(y);et0())CO UC*p=_V(y);N n=_n(y);P(n<9||p[0]!=1,mr(y);ed0())I len;MC(&len,p+4,4);P((N)len!=n,mr(y);ed0())
 I(p[2],A u=qunz(p,n);mr(y);P(!u,0)y=u;p=_V(y);n=_n(y))
 QR b={p,n,8,0};A r=qdec(&b,0);I(b.bad==4,mr(y);return 0)
 I(!r||b.bad||b.i!=b.n,I(r,mr(r))I k=b.bad;mr(y);return k==3?en0():ed0())mr(y);return r;}

/* ---- handles, the port, the timer (2.9.0) -------------------------------------------------------------------------
 * A handle is a file descriptor, as q's: hopen gives it, h x sends x and waits for the answer (a sync message), neg[h]x
 * sends it and goes on (async), h[] waits for the next message h sends (a deferred answer: gw.q's neg[.z.w] reply), and
 * on a log (hopen of a file) h x appends each item of x as q's log does. \p n listens on port n; a client's message is
 * given to .z.pg (sync) or .z.ps (async), its connection to .z.po and .z.pc, and .z.w is its handle meanwhile. \t n calls
 * .z.ts every n ms. One poll() waits on all of them: before each read of the terminal (ln.c), and after a script that
 * opened a port or a timer (qloop), as q goes on serving. */
#if !defined(wasm)
#define QH 4096
Z UC qhk[QH];                  //a handle's kind: 0 none, 1 a connection hopen made, 2 a client of \p, 3 a log
Z I qls=-1,qlp=0;              //\p's socket and its port
Z L qtp=0,qtn=0;               //\t's period (ms) and the next tick (ms of the epoch)
Z L qms(V)_(ST timeval t;gettimeofday(&t,0);(L)t.tv_sec*1000+t.tv_usec/1000)
Z I qwr(I f,CO V*p,N n){CO C*s=p;W(n,ssize_t k=write(f,s,n);I(k<0&&errno==EINTR,continue)I(k<=0,return 0)s+=k;n-=(N)k)return 1;}
Z I qrd(I f,V*p,N n){C*s=p;W(n,ssize_t k=read(f,s,n);I(k<0&&errno==EINTR,continue)I(k<=0,return 0)s+=k;n-=(N)k)return 1;}
Z I qsnd(I f,A x/*0*/,UC m){A b=qser(x,m);I(!b,return -1)I r=qwr(f,_V(b),_n(b));mr(b);return r;}
Z I qsnde(I f,S e){I(*e==39,e++)N n=0;W(e[n]&&e[n]!=10,n++);UC h[9]={1,2,0,0,0,0,0,0,0x80};I l=(I)(9+n+1);MC(h+4,&l,4);C z=0;return qwr(f,h,9)&&qwr(f,e,n)&&qwr(f,&z,1);}   //an error answer: -128 and its text
Z I qok(I h)_(h>0&&h<QH&&qhk[h])
Z A qfile(S p){I f=open(p,O_RDONLY);I(f<0,return eo0())L n=lseek(f,0,SEEK_END);I(n<0||n>((L)1<<31)||lseek(f,0,SEEK_SET),close(f);return eo0())A y=an((U)n,tC);I k=qrd(f,_V(y),(N)n);close(f);I(!k,mr(y);return eo0())return y;}   //a whole file
// the next message on f: its value, *m its type (255: f closed, or bytes that are not a message)
Z A qrcv(I f,UC*m){UC h[8];I(!qrd(f,h,8),*m=255;return 0)*m=h[1];I l;MC(&l,h+4,4);I(l<9||h[0]!=1,*m=255;return ed0())
 A y=an((U)l,tC);MC(_V(y),h,8);I(!qrd(f,(UC*)_V(y)+8,(N)l-8),mr(y);*m=255;return 0)return qdes(y);}
Z V qdrop(I fd){I(qok(fd),UC k=qhk[fd];qhk[fd]=0;close(fd);I(k==2,A r=K1("{z.pc x}",ai(fd));I(r,mr(r))E(ecl())))}   //(K1 has a static f of its own: no f in its arguments)
Z V qzw(I fd){A r=K1("{z.w::x}",ai(fd));I(r,mr(r))E(ecl())}
// a client's message: .z.pg's answer to a sync one (its error, as q's, when it fails), .z.ps for an async one
Z V qserve(I f){UC m;A v=qrcv(f,&m);I(m==255,I(!v,ecl())qdrop(f);return)
 I(!v,CO C*e=errtext();I(m==1,qsnde(f,e&&*e?e:"type"))ecl();return)
 qzw(f);I dq=amdiagq;amdiagq=1;A r=m==1?K1("{z.pg x}",v):m==0?K1("{z.ps x}",v):(mr(v),au);amdiagq=dq;   //a client's error is its answer, not printed here, as q
 I(m==1,I(r,I k=qsnd(f,r,2);I(k<0,qsnde(f,"type"))mr(r))E(CO C*e=errtext();qsnde(f,e&&*e?e:"error");ecl()))E(I(r,mr(r))E(qerrp()))qzw(0);}
Z V qacc(V){I fd=accept(qls,0,0);I(fd<0,return)I(fd>=QH,close(fd);return)C c[1024];N n=0;W(n<SZ c,I(read(fd,c+n,1)!=1,close(fd);return)I(!c[n],break)n++)
 UC v=3;I(!qwr(fd,&v,1),close(fd);return)setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,(I[]){1},4);qhk[fd]=2;A r=K1("{z.po x}",ai(fd));I(r,mr(r))E(ecl())}
//an error in a timer or an async message: one line on the console, as q's ('2026.10.13T13:35:54.899 type), not the diagnostic
Z V qerrp(V){CO C*e=errtext();I(e&&*e==39,e++)C b[96];struct timeval tv;gettimeofday(&tv,0);time_t t=tv.tv_sec;struct tm m;localtime_r(&t,&m);
 I n=snprintf(b,SZ b,"'%04d.%02d.%02dT%02d:%02d:%02d.%03d ",m.tm_year+1900,m.tm_mon+1,m.tm_mday,m.tm_hour,m.tm_min,m.tm_sec,(I)(tv.tv_usec/1000));
 N k=0;I(e,W(e[k]&&e[k]!=10,k++))I(write(1,b,(N)n)<0||write(1,e?e:"error",e?k:5)<0||write(1,"\n",1)<0,{})ecl();}
Z V qtick(V){L t=qms();I(!qtp||t<qtn,return)qtn=t+qtp;I dq=amdiagq;amdiagq=1;A r=K1("{z.ts x}",antp((t-946684800000ll)*1000000));amdiagq=dq;I(r,mr(r))E(qerrp())}
// one wait: up to ms (-1 none) for the terminal (when s), the port, the clients, the timer; 1 when the terminal is ready
Z I qpoll(B s,I ms){ST pollfd p[QH+2];I n=0;I(s,p[n].fd=0;p[n].events=POLLIN;n++)I(qls>=0,p[n].fd=qls;p[n].events=POLLIN;n++)
 F(QH,I(qhk[i]==1||qhk[i]==2,p[n].fd=(I)i;p[n].events=POLLIN;n++))
 I(qtp,L d=qtn-qms();d=d<0?0:d;ms=ms<0||d<ms?(I)d:ms)
 I k=poll(p,(nfds_t)n,ms);I(k<0,return s)qtick();I in=0;
 F(n,I(p[i].revents,I f=p[i].fd;I(s&&!f,in=1)J(f==qls,qacc())E(qserve(f))))return in;}
I qactive(V)_(qls>=0||qtp>0)
V qidle(V){I(!qactive(),return)W(!qpoll(1,-1));}   //before a read of the terminal: serve until it has a key
V qloop(V){W(qactive(),qpoll(0,-1));}               //after a script: serve while there is a port or a timer
// h x: to a connection, sync (wait for the answer, serving the async messages that come first), neg[h] x async;
// h[] the next message's value; to a log, append x's items. et where h is no handle
//q's 1 x and 2 x write text to stdout and stderr, -1 x and -2 x with a newline after each line (a list of strings: a line
//each), giving the handle back; 0 x is x's value here
Z A qcon1(I h,A y/*1*/)_(I f=h<0?-h:h;B nl=h<0;UC t=_t(y);
 P(t==tc,C c=(C)_v(y);I(!qwr(f,&c,1)||nl&&!qwr(f,"\n",1),return eo0())ai(h))
 P(t==tC,I k=qwr(f,_V(y),_n(y))&&(!nl||qwr(f,"\n",1));y(k?ai(h):eo0()))
 P(t==tA,U n=_n(y);F(n,A e=_A(y)[i];P(_tP(e)||_t(e)-tC,y(et0()))I(!qwr(f,_V(e),_n(e))||nl&&!qwr(f,"\n",1),return y(eo0())))y(ai(h)))
 P(!_tP(y)&&!_n(y),I(nl&&!qwr(f,"\n",1),return y(eo0()))y(ai(h)))et(y))
A qcall(I h,A y/*1*/){I(h==1||h==2||h==-1||h==-2,return qcon1(h,y);)I(!h,P(_tP(y)||_t(y)-tC&&_t(y)-tA,et(y))P(_t(y)==tC,val(y))return K1("{qvalue x}",y);)B a=h<0;I f=a?-h:h;P(!qok(f),et(y))
 I(qhk[f]==3,P(_tP(y)||_t(y)>=tM,et(y))U n=_N(y);F(n,A e=ii(y,i);A b=qser(e,0);mr(e);P(!b,y(0))I k=qwr(f,_V(b)+8,_n(b)-8);mr(b);P(!k,y(eo0())))return y(au);)
 I(a,I k=qsnd(f,y,0);mr(y);P(k<=0,k<0?0:(qdrop(f),eo0()))return au;)
 B w=y==au;I(!w,I k=qsnd(f,y,1);mr(y);P(k<=0,k<0?0:(qdrop(f),eo0())))
 W(1,UC m;A v=qrcv(f,&m);P(m==255,I(!v,qdrop(f))v?v:eo0())P(!v,0)P(m==2||w,v)I dq=amdiagq;amdiagq=1;A r=K1("{z.ps x}",v);amdiagq=dq;I(r,mr(r))E(qerrp()))
 return 0;}
// hopen: an int port (this host), `:host:port[:user:pass] (`::port this host), or a file `:path, a log to append to
Z I qcon(S hs,S ps,S up){ST addrinfo h,*r=0;MS(&h,0,SZ h);h.ai_family=AF_INET;h.ai_socktype=SOCK_STREAM;I(getaddrinfo(*hs?hs:"127.0.0.1",ps,&h,&r)||!r,return -1)
 I f=socket(AF_INET,SOCK_STREAM,0);I(f<0,freeaddrinfo(r);return -1)I c=connect(f,r->ai_addr,r->ai_addrlen);freeaddrinfo(r);I(c<0,close(f);return -1)
 I(f>=QH,close(f);return -1)setsockopt(f,IPPROTO_TCP,TCP_NODELAY,(I[]){1},4);N n=SL(up);C b[600];I(n>500,n=500)MC(b,up,n);b[n]=3;b[n+1]=0;
 UC v;I(!qwr(f,b,n+2)||!qrd(f,&v,1),close(f);return -1)qhk[f]=1;return f;}
A qhopen(A x/*1*/)_(P(_tz(x),L p=gl(x);C ps[24];snprintf(ps,SZ ps,"%lld",(long long)p);I f=qcon("",ps,"");P(f<0,eo0())ai(f))
 P(!(_ts(x)||_t(x)==tC),et(x))A s=_ts(x)?str(x):x;s=str0(s);S t=_V(s);I(*t==':',t++)
 C hb[256],pb[32],ubuf[512];hb[0]=pb[0]=ubuf[0]=0;S c=strchr(t,':');S e=c?c+1:0;B net=c&&C09(*e);   //host:port.. when a port follows the first colon
 I(net,N hl=MIN((N)(c-t),SZ hb-1);MC(hb,t,hl);hb[hl]=0;S q=e;W(C09(*q),q++)N pl=MIN((N)(q-e),SZ pb-1);MC(pb,e,pl);pb[pl]=0;I(*q==':',snprintf(ubuf,SZ ubuf,"%s",q+1))
   mr(s);I f=qcon(hb,pb,ubuf);P(f<0,eo0())return ai(f);)
 I f=open(t,O_WRONLY|O_APPEND|O_CREAT,0666);mr(s);P(f<0||f>=QH,I(f>=0,close(f))eo0())qhk[f]=3;ai(f))
A qexit(A x/*1*/)_(L c=_tz(x)?gl(x):0;A r=K1("{z.exit x}",az(c));I(r,mr(r))E(ecl())exit((I)c);au)   //exit n: .z.exit, then gone, as q
A qhclose(A x/*1*/)_(P(!_tz(x),et(x))I f=(I)gl(x);P(!qok(f),ed0())qhk[f]=0;close(f);au)   //no .z.pc: we closed it
// \p n: listen on n (0: stop); \p: the port
A qport(S s)_(W(*s==' ',s++)P(!*s,ai(qlp))L p=pl((S*)&s);I(qls>=0,close(qls);qls=-1;qlp=0)P(!p,au)
 I f=socket(AF_INET,SOCK_STREAM,0);P(f<0,eo0())setsockopt(f,SOL_SOCKET,SO_REUSEADDR,(I[]){1},4);ST sockaddr_in a;MS(&a,0,SZ a);a.sin_family=AF_INET;a.sin_addr.s_addr=INADDR_ANY;
 a.sin_port=htons((UH)p);P(bind(f,(ST sockaddr*)&a,SZ a)<0||listen(f,64)<0,close(f);eo0())qls=f;qlp=(I)p;au)
// \t n: .z.ts every n ms (0: stop); \t: the period
A qtimer(L n)_(qtp=n>0?n:0;qtn=qms()+qtp;au)
L qtperiod(V)_(qtp)
#else
V qidle(V){}V qloop(V){}I qactive(V)_(0)A qcall(I h,A y)_(et(y))A qhopen(A x)_(en(x))A qhclose(A x)_(en(x))A qport(S s)_(en0())A qtimer(L n)_(en0())L qtperiod(V)_(0)
Z I qwr(I f,CO V*p,N n){return 0;}Z A qfile(S p){return en0();}
#endif

/* ---- q's logs ------------------------------------------------------------------------------------------------------
 * A log is q's file of an empty general list (ff 01, then 0 0 and a count of 0) and each message after it, as its bytes
 * (no message header): what `:f set () writes and what a log handle appends. -11!f replays it: each message's value, as
 * q's -11! (value of a list applies its first item to the rest), its count returned; -11!(n;f) the first n; -11!(-2;f)
 * only counts the whole messages. A message cut short at the end stops the replay there */
Z A qpath(A x/*1*/)_(A s=_ts(x)?str(x):x;P(_t(s)-tC,et(s))s=str0(s);P(!s,0)I(*(S)_V(s)==':',A t=aCz((S)_V(s)+1);mr(s);s=str0(t))s)
A qhdel(A x/*1*/)_(A s=N(qpath(x));I r=unlink(_V(s));mr(s);P(r,eo0())au)   //hdel: a file deleted
A qlog0(A x/*1*/)_(A s=N(qpath(x));I f=open(_V(s),O_WRONLY|O_CREAT|O_TRUNC,0666);mr(s);P(f<0,eo0())UC h[8]={0xff,1,0,0,0,0,0,0};I k=qwr(f,h,8);close(f);k?au:eo0())
A qreplay(A x/*1*/){L n=-1;A p=x;I(!_tP(x)&&_t(x)==tA&&_n(x)==2,n=gl(ii(x,0));p=ii(x,1);mr(x))A s=N(qpath(p));A b=qfile(_V(s));mr(s);P(!b,0)
 P(_t(b)-tC,mr(b);et0())CO UC*q=_V(b);N m=_n(b);P(m<8||q[0]!=0xff||q[1]!=1,mr(b);err0("badtail"))QR r={q,m,8,0};L k=0;
 W(r.i<m&&(n<0||k<n),A v=qdec(&r,0);I(!v||r.bad,I(v,mr(v))I(r.bad==4,mr(b);return 0)break)
  I(n==-2,mr(v);k++;continue)A e=K1("{qvalue x}",v);P(!e,mr(b);(A)0)mr(e);k++)
 mr(b);return az(k);}
//`tzo: the local time's offset from UTC in seconds, for q's local clocks .z.P .z.N .z.D .z.T .z.Z (2.9.0: they were UTC)
A qtzo(A x/*1*/)_(mr(x);time_t t=time(0);struct tm m;P(!localtime_r(&t,&m),az(0))az((L)m.tm_gmtoff))
