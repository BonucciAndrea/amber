#include<dlfcn.h> // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#include<sys/socket.h>
#include<sys/wait.h>
#include<netinet/in.h>
#include<netinet/tcp.h>
#include<fcntl.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/time.h>
#undef __USE_EXTERN_INLINES
#include<sys/stat.h>
#include<sys/mman.h>
#include<dirent.h>
#include"a.h"
Z U addr(S*p)_(S s=*p;P(!*s,0x0100007f)UC v[4];F(4,I(i,P(*s-'.',ed0())s++)v[i]=pu(&s);P(v[i]>255,ed0()))*p=s;*(U*)v)
Z I skt(U h,UH p)_(I f=socket(AF_INET,SOCK_STREAM,0);P(f<0,eo0())I v=setsockopt(f,IPPROTO_TCP,TCP_NODELAY,(I[]){1},4);P(v<0,eo0())
ST sockaddr_in a;a.sin_family=AF_INET;a.sin_addr.s_addr=h;a.sin_port=(UH)(p<<8|p>>8);P(connect(f,(ST sockaddr*)&a,SZ a)<0,eo0())f)
Z I osf(S s,L fl)_(P(!strchr(s,':'),I f=open(s,fl,0666);P(f<3/*fbsd*/,eo0())f)U h=addr(&s);P(*s-':',ed0())s++;W p=pu(&s);P(*s,ed0())skt(h,p))
Z I o(A x/*1*/,I fl)_(Xz(gl(x))Xs(xv?osf(su(xv),fl):1)XC(x=str0(x);I v;Mx(v=osf(xV,fl));v)et(x))
Z I fm(I f)_(ST stat s;fstat(f,&s)<0?0:s.st_mode)                                                                                                                 // get file mode
Z A frd(I f,N i,N n)_(P(i||n+1,en0())DIR*a=fdopendir(f);P(!a,ei0())A x=emp(tC);ST dirent*e;W((e=readdir(a)),S s=e->d_name;x=apc(cts(x,s,SL(s)),10))closedir(a);x) // read dir
Z A frS(I f,N n)_(C b[1024];A x=emp(tC);W(n,I k=read(f,b,MIN(SZ b,n));P(k<0,eo(x))n-=k;x=cts(x,b,k);P(k-SZ b,x))x)                                                // read stream (only length)
Z A frs(I f,N i,N n)_(I(i&&lseek(f,i,SEEK_CUR)<0,mr(N(frS(f,i))))frS(f,n))                                                                                        // read stream (offset too)
Z A frm(I f,N i,N n)_(L m=lseek(f,0,SEEK_END);P(m<0,eo0())n=MIN(n,MAX(0,m-i));n?mf(f,i,n):emp(tC))                                                                // read through mmap
Z A fr(A x/*1*/,N i,N n)_(Xz(frs(gl(x),i,n))I f=N(o(x,O_RDONLY));P(f<3,frs(f,i,n))I m=fm(f);x=(S_ISDIR(m)?frd:S_ISREG(m)?frm:frs)(f,i,n);close(f);x)              // read
Z A fws(I f,S s,N n)_(W(n>0,L k=write(f,s,n);P(k<0,eo0())P(!k,au)s+=k;n-=k)au)                                                                                    // write stream
Z A fwm(I f,S s,N n)_(ftruncate(f,n);V*p=mmap(0,n,PROT_READ|PROT_WRITE,MAP_SHARED,f,0);MC(p,s,n);munmap(p,n);au)                                                  // write through mmap
Z X2(fw,Ril(I f=gl_(x);My(x=(f<3||!S_ISREG(fm(f))?fws:fwm)(f,yV,yn))x)R_(I f=N(o(xR,O_RDWR|O_CREAT|O_TRUNC));A z=v1c(ai(f),y);I(f>2,close(f))z))                   // write
ZN A dle()_(C*e=dlerror();I(e,os(e);os("\n"))eo0())
A1(opn,Xz(x)ai(N(o(x,O_RDWR|O_CREAT))))                                                                                     // <s
A cls(L n)_(close(n);au)                                                                                                    // >i
A1(u0c,spl(N(u1c(x))))                                                                                                      // 0:x
X1(u1c,RA(P(xn-2,el(x))P(!_tZ(xy),et(x))P(_n(xy)-2,el(x))A y=kv(&x);N i=gl(ii(y,0)),n=gl(ii(y,1));fr(x,i,n))R_(fr(x,0,-1))) // 1:x
A1(u2c,en(x))                                                                                                               // 2:x
Y2(v0c,RA(v0c(x,N(jc(10,y))))RC(v1c(x,apc(y,10)))R_(et(y)))                                                                 // x 0:y
Y2(v1c,RC(fw(x,y))R_(et(y)))                                                                                                // x 1:y
A2(v2c,P(!xts||!ytA,et(y))P(yn-2,el(y))P(!_ts(yx)||!_ti(yy),et(y))I k=_v(yy);y(0);P(!k||k>8u,ed0())                         // x 2:y
 V*l=dlopen(su(xv),RTLD_LAZY);P(!l,dle())V*f=dlsym(l,su(_v(yx)));P(!f,dle())ax(f,k))

Z A rda(I f)_(A x=aC(256-HD);L m=0,k;W((k=read(f,xV+m,xn-m))>0,m+=k;I(m+1000000>xn&&2*m>xn,A y=aC(2*xn+HD);MC(yV,xV,m);x=x(y)))close(f);AN(m,x))
Z I lC(A x)_(XA(F(xn,P(_t(xa)-tC,0))1)0)//list of strings?
A1(frk,P(!xtA||xn-2,et(x))A y=kv(&x);P(!lC(x)||!ytC,y(ed(x)))x=Ny(e1f(str0,x));S a[xn+1];F(xn,a[i]=_V(xa))a[xn]=0;I p[4];pipe(p);pipe(p+2);I pid=fork();
 P(!pid,dup2(*p,0);dup2(p[3],1);F(4,close(p[i]))exit(execve(*a,(C**)a,(C*CO*)env));0)close(*p);close(p[3]);N(v1c(ai(p[1]),x(y)));close(p[1]);A x=rda(p[2]);wait4(pid,0,0,0);x)
// amber: fork-based parallel-each.  x=(f;y): apply f to each item of y across
// AMBER_THREADS worker processes (default: online CPU count, see peachCPUs()
// below).  Each worker computes its slice,
// serialises the result (-8!, binary; was `k text before 1.9.3) and writes it
// down a pipe; the parent reads each (rda), deserialises (-9!) and concatenates.  Falls back to serial each for
// tiny inputs or AMBER_THREADS<2.  Correct because  (. `k v) ~ v  for all v.
// amber: default worker count = actual online CPU count (sysconf), not a
// hardcoded guess -- previously this always fell back to a fixed 4 regardless
// of the host, so on a 1-2 core box peach would oversubscribe (fork more
// workers than there are cores, making it *slower* than serial f'y through
// pure context-switch/fork overhead) and on a 16+ core box it would leave
// most cores idle. src/parallel.c's par_thread_count() already solves this
// correctly for the SIMD/vector engine (online_cpus() via
// sysconf(_SC_NPROCESSORS_ONLN)); this mirrors that same fix for peach so the
// two parallel primitives in Amber agree on what "auto" means. AMBER_THREADS
// still overrides explicitly, unchanged.
Z I peachCPUs(){
#if defined(_SC_NPROCESSORS_ONLN)
 L n=sysconf(_SC_NPROCESSORS_ONLN);if(n>0)return(I)n;
#endif
 return 4;//last-resort fallback if sysconf itself is unavailable
}
Z I peachNW(){S*e=env;I n=-1;if(e)while(*e){S p=*e++;if(!strncmp(p,"AMBER_THREADS=",14)){n=0;S q=p+14;while(*q>='0'&&*q<='9')n=n*10+(*q++-'0');if(n<1)n=peachCPUs();break;}}return n<0?peachCPUs():n;}
Z A eachR(A f,A y,U lo,U hi){U m=hi-lo;A u=aA0(m|!m);for(U i=0;i<m;i++){A v=_1(f,ii(y,lo+i));if(!v){mr(u);return 0;}u=psh(u,v);}return sqz(u);}
A peachC(A x){P(_t(x)-tA||_n(x)-2,et(x))A fn=ii(x,0),dat=ii(x,1);U n=_N(dat);I nw=peachNW();if(nw>64)nw=64;
 if(nw<2||n<2){A r=eachR(fn,dat,0,n);mr(fn);mr(dat);return x(r);}
 // fork-based parallelism needs a real pipe()/fork(); neither exists in the
 // wasm sandbox (src/0.c's wasm branch stubs both to always return -1
 // there), and this loop never used to check for that, so pipe()/fork()
 // failing left pr[]/pid[] holding uninitialized fds that the read/wait
 // loop below then used -- a wasm trap (uncatchable from K), not a clean
 // error. One throwaway probe pipe() up front detects the unsupported
 // environment and falls back to the plain serial eachR pass instead,
 // which is exactly what std.k's own peach wrapper comment already
 // documents as the intended behavior ("this build is single-threaded, so
 // peach evaluates sequentially") -- this just makes the C side actually
 // honor that instead of assuming fork() always succeeds. No effect on a
 // real fork()-capable build: the probe pipe costs two fds, immediately
 // closed, once per peach call.
 {I pp[2];if(pipe(pp)<0){A r=eachR(fn,dat,0,n);mr(fn);mr(dat);return x(r);}close(pp[0]);close(pp[1]);}
 if((U)nw>n)nw=n;I pr[128],pid[64];U base=n/nw,rem=n%nw,lo=0;
 for(I w=0;w<nw;w++){U hi=lo+base+((U)w<rem);pipe(pr+2*w);pid[w]=fork();
  if(!pid[w]){close(pr[2*w]);A r=eachR(fn,dat,lo,hi);if(!r)_exit(3);
   /* amber 1.9.3: ship the chunk as BINARY (-8!) instead of `k text. The text
    * path formatted every element and the parent reparsed it, which cost more
    * than the work being parallelised on small-payload maps and could not
    * represent attributes or nested empties faithfully at all. ser8 fails
    * closed: a chunk that cannot be encoded exits non-zero rather than
    * writing a short buffer the parent would misparse. */
   A b=ser8(r);if(!b)_exit(4);
   v1c(ai(pr[2*w+1]),b);close(pr[2*w+1]);_exit(0);}
  close(pr[2*w+1]);lo=hi;}
 /* Parent collection. Three bugs lived in the one line this replaces:
  *   1. LEAK. `out=cat(out,part)` -- cat() is A2(cat,cat11(xR,y)): it bumps
  *      x's refcount and hands that extra reference to cat11. The caller's own
  *      reference to the OLD out was therefore never released, so every chunk
  *      past the first leaked a whole accumulated result vector. Worse, the
  *      surviving refcount made MINE(out) false inside aa(), so the append
  *      could not grow in place and reallocated the whole accumulator every
  *      time -- O(total^2) copying on top of the leak. Calling cat11 directly,
  *      which OWNS both arguments, fixes both at once: nothing is left holding
  *      a stray reference, and out is uniquely owned so the append extends in
  *      place. (Bumping and then releasing -- cat() plus mr(old) -- plugs the
  *      leak but keeps the refcount at 2 during the call, so it would still
  *      reallocate every chunk.)
  *   2. EXIT STATUS IGNORED. wait4(pid,0,0,0) discarded the child's status, so
  *      a worker that died on a signal or exited non-zero (eachR returning 0,
  *      or ser8 failing) was indistinguishable from success: the parent just
  *      saw a short/empty pipe and silently produced a WRONG result. Status is
  *      now inspected and any failure becomes a clean K error.
  *   3. UNVALIDATED DECODE. val(rda(...)) assumed the pipe held a parseable
  *      value; des9 returns 0 on a truncated or corrupt buffer and that is now
  *      treated as a worker failure rather than dereferenced.
  * Every child is still waited for even after a failure is detected, so no
  * zombie is left behind and no pipe fd is orphaned (rda() closes its own). */
 A out=0;I bad=0;
 for(I w=0;w<nw;w++){
  A raw=rda(pr[2*w]);                 /* rda() closes the read fd itself */
  A part=raw?des9(raw):0;             /* des9() consumes raw            */
  I st=0;wait4(pid[w],&st,0,0);
  I ok=part&&WIFEXITED(st)&&!WEXITSTATUS(st)&&!WIFSIGNALED(st);
  I(!ok,bad=1;I(part,mr(part))continue)
  I(!out,out=part)
  E(out=cat11(out,part);)   /* cat11 owns BOTH: no stray ref, grows in place */
 }
 mr(fn);mr(dat);
 I(bad||!out,I(out,mr(out))return x(err0("worker error in peach")))
 return x(sqz(out));}
// amber: window-join C kernel.  x=(qt;qcols;codes;w0;w1;gb;ge)  (marshalled by wj in amber.k)
//  qt    sorted long vector (ordering column; ascending within each group slice)
//  qcols list of numeric vectors (tF or tL) aligned to qt, one per aggregate
//  codes int vector, reducer per agg: 0=first 1=last 2=min 3=max 4=sum 5=avg 6=count
//  w0,w1 long window bounds per trade row (length nt)
//  gb,ge long group-slice [base,end) in q per trade row (length nt)
// returns list of nt-length result columns (tF for avg/float-source, tL otherwise).
// O(log g) range probe per row + one contiguous slice sweep; no per-row K objects.
A wjc(A x){
 P(_t(x)-tA||_n(x)-7,et(x))
 A*e=(A*)_V(x);
 P(!_n(e[1]),x(emp(tA)))
 // normalise all integer inputs to 64-bit long (columns/times/bounds may be squeezed to G/H/I widths)
 A QT=N(cL(_R(e[0]))),CD=N(cL(_R(e[2]))),W0A=N(cL(_R(e[3]))),W1A=N(cL(_R(e[4]))),GBA=N(cL(_R(e[5]))),GEA=N(cL(_R(e[6])));
 CO L*T=_V(QT),*W0=_V(W0A),*W1=_V(W1A),*GB=_V(GBA),*GE=_V(GEA),*cod=_V(CD);
 U nt=_n(W0A),na=_n(e[1]);
 A*QC=(A*)_V(e[1]);
 A res=aA(na);A*R=(A*)_V(res);
 for(U a=0;a<na;a++){
  A col=QC[a];I c=(I)cod[a];
  B isf=_t(col)==tF,flo=(c==5)||(isf&&c!=6);
  A out=flo?aF(nt):aL(nt);
  A colL=isf?0:N(cL(_R(col)));
  F*of=(F*)_V(out);L*ol=(L*)_V(out);
  CO F*pf=isf?(CO F*)_V(col):0;CO L*pl=isf?0:(CO L*)_V(colL);
  for(U i=0;i<nt;i++){
   L b=GB[i],en=GE[i];U lo,hi;
   if(b==NL||en==NL||en<=b){lo=0;hi=0;}
   else{lo=amlb(T,(U)b,(U)en,W0[i]);hi=amlb(T,(U)b,(U)en,W1[i]+1);}
   U m=hi-lo;
   if(c==6){ol[i]=(L)m;continue;}
   if(flo){F r;
    if(!m)r=c==2?WF:c==3?-WF:c==4?0.0:NF;
    else if(c==0)r=isf?pf[lo]:(F)pl[lo];
    else if(c==1)r=isf?pf[hi-1]:(F)pl[hi-1];
    else if(c==2){r=WF;for(U k=lo;k<hi;k++){F v=isf?pf[k]:(F)pl[k];if(v<r)r=v;}}
    else if(c==3){r=-WF;for(U k=lo;k<hi;k++){F v=isf?pf[k]:(F)pl[k];if(v>r)r=v;}}
    else if(c==4){r=0;for(U k=lo;k<hi;k++)r+=isf?pf[k]:(F)pl[k];}
    else{r=0;for(U k=lo;k<hi;k++)r+=isf?pf[k]:(F)pl[k];r/=m;}
    of[i]=r;
   }else{L r;
    if(!m)r=c==2?WL:c==3?-WL:c==4?0:NL;
    else if(c==0)r=pl[lo];
    else if(c==1)r=pl[hi-1];
    else if(c==2){r=pl[lo];for(U k=lo+1;k<hi;k++)if(pl[k]<r)r=pl[k];}
    else if(c==3){r=pl[lo];for(U k=lo+1;k<hi;k++)if(pl[k]>r)r=pl[k];}
    else{r=0;for(U k=lo;k<hi;k++)r+=pl[k];}
    ol[i]=r;
   }
  }
  if(colL)mr(colL);
  R[a]=out;
 }
 mr(QT);mr(CD);mr(W0A);mr(W1A);mr(GBA);mr(GEA);
 return x(res);
}
// ============ Braille & Unicode terminal charting ============
// Braille dot bit for sub-cell (sx in 0..1, sy in 0..3).  U+2800 + bitmask -> 2x4 pixel cell.
Z CO UC BRA[4][2]={{0x01,0x08},{0x02,0x10},{0x04,0x20},{0x40,0x80}};
Z V pxset(UC*RES c,I W,I H,I px,I py){if(px>=0&&py>=0&&px<2*W&&py<4*H)c[(py>>2)*W+(px>>1)]|=BRA[py&3][px&1];}
Z V bres(UC*c,I W,I H,I x0,I y0,I x1,I y1){I ax=x1-x0,ay=y1-y0,dx=ax<0?-ax:ax,sx=x0<x1?1:-1,dy=ay<0?ay:-ay,sy=y0<y1?1:-1,er=dx+dy;//Bresenham
 while(1){pxset(c,W,H,x0,y0);if(x0==x1&&y0==y1)break;I e2=2*er;if(e2>=dy){er+=dy;x0+=sx;}if(e2<=dx){er+=dx;y0+=sy;}}}
Z C*ebr(C*p,UC b){U cp=0x2800+b;*p++=0xE2;*p++=0x80|(cp>>6&0x3F);*p++=0x80|(cp&0x3F);return p;}//emit braille char
Z C*elab(C*p,F v,I w){C b[40];L db;MC(&db,&v,8);C*e=sf(b,db);I ll=e-b;I(ll>w,ll=w)F(w-ll,*p++=' ')F(ll,*p++=b[i])return p;}//right-justified label
// plot: x = numeric vector, or (series;W;H).  Returns a multi-line UTF-8 braille line chart.
A plotC(A x){
 A dat;I W=70,H=15;
 if(_t(x)==tA){dat=N(ii(x,0));if(_n(x)>1)W=(I)gl(N(ii(x,1)));if(_n(x)>2)H=(I)gl(N(ii(x,2)));}else dat=_R(x);
 mr(x);A ser=N(cF(dat));U n=_n(ser);
 if(!n){mr(ser);return aCz("(empty)\n");}
 if(W<10)W=10;if(W>200)W=200;if(H<3)H=3;if(H>60)H=60;I PW=2*W,PH=4*H;
 CO F*d=(CO F*)_V(ser);F mn=d[0],mx=d[0];F(n,I(d[i]<mn,mn=d[i])I(d[i]>mx,mx=d[i]))F rng=mx-mn;I(rng<=0,rng=1)
 UC cells[12000];MS(cells,0,W*H);I ppx=0,ppy=0;
 F(n,I px=n>1?(I)((F)i*(PW-1)/(n-1)+.5):PW/2,py=(I)((PH-1)*(1.-(d[i]-mn)/rng)+.5);I(px>PW-1,px=PW-1)I(py>PH-1,py=PH-1)I(py<0,py=0)I(i,bres(cells,W,H,ppx,ppy,px,py))E(pxset(cells,W,H,px,py))ppx=px;ppy=py)
 A out=aC(H*(16+W*3)+64);C*p=(C*)_V(out);
 F(H,p=elab(p,i==0?mx:i==H-1?mn:mx-rng*i/(H-1),8);*p++=' ';*p++=0xE2;*p++=0x94;*p++=0x82;/*│*/
  Fj(W,UC b=cells[i*W+j];I(b,p=ebr(p,b))E(*p++=' '))*p++='\n')
 mr(ser);return AN(p-(C*)_V(out),out);}
// candle: x = (open;high;low;close) 4 numeric vectors.  Box wicks + block bodies + ANSI colour.
A candleC(A x){
 P(_t(x)-tA||_n(x)-4,et(x))
 A o=N(cF(N(ii(x,0)))),h=N(cF(N(ii(x,1)))),l=N(cF(N(ii(x,2)))),c=N(cF(N(ii(x,3))));mr(x);
 U n=_n(o);I H=15;if(!n){mr(o);mr(h);mr(l);mr(c);return aCz("(empty)\n");}
 CO F*O=_V(o),*Hi=_V(h),*Lo=_V(l),*Cl=_V(c);
 F mn=Lo[0],mx=Hi[0];F(n,I(Lo[i]<mn,mn=Lo[i])I(Hi[i]>mx,mx=Hi[i]))F rng=mx-mn;I(rng<=0,rng=1)
 A out=aC(H*(9+n*24)+64);C*p=(C*)_V(out);
 F(H,I r=i;p=elab(p,i==0?mx:i==H-1?mn:mx-rng*i/(H-1),8);*p++=0xE2;*p++=0x94;*p++=0x82;
  Fj(n,I rh=(I)((H-1)*(1.-(Hi[j]-mn)/rng)+.5),rl=(I)((H-1)*(1.-(Lo[j]-mn)/rng)+.5),bt=O[j]>Cl[j]?O[j]:Cl[j],bb=O[j]<Cl[j]?O[j]:Cl[j];
   I rbt=(I)((H-1)*(1.-(bt-mn)/rng)+.5),rbb=(I)((H-1)*(1.-(bb-mn)/rng)+.5);B up=Cl[j]>=O[j];
   S col=up?"\033[32m":"\033[31m";
   I(r>=rbt&&r<=rbb,MC(p,col,5);p+=5;*p++=0xE2;*p++=0x96;*p++=0x88;/*█*/MC(p,"\033[0m",4);p+=4)
   J(r>=rh&&r<=rl,MC(p,col,5);p+=5;*p++=0xE2;*p++=0x94;*p++=0x82;/*│*/MC(p,"\033[0m",4);p+=4)
   E(*p++=' ')*p++=' ')*p++='\n')
 mr(o);mr(h);mr(l);mr(c);return AN(p-(C*)_V(out),out);}
L now()_(ST timeval t;gettimeofday(&t,0);1000000ll*t.tv_sec+t.tv_usec)
