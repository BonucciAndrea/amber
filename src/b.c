#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
#define C2(x,a...) case x:C(a)
#define C6(u,v,w,x,y,z,a...) case u:case v:case w:case x:case y:case z:{a;break;}
#define C16(x,a...) case x:case x+1:case x+2:case x+3:case x+4:case x+5:case x+6:case x+7:case x+8:case x+9:case x+10:case x+11:case x+12:case x+13:case x+14:case x+15:{a;break;}
#define C32(x,a...) C16(x,C16(x+16,a))
#define U(x,a...) I(!(x),a;goto l)
#define OFF 4 //offset of constants in a function object
#define n1 -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
#define p1  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
enum {bu,  bv=32,bs=64,bg=80,bd=96,ba=112,bp,bm,bM,bx,bX,by,bY,bG,bS,bl,bL,bz,bj,bo,bP,bV,bc};      //opcodes
Z CO C di[]={                     [ba]= 1, 1, 3, 3, 3, 3, 3, 3, 2, 2, 1, 1, 1, 1, 0, 0, 2, 0},      //extra bytes after opcode (2-byte global/var index)
ds[]={[bv]=n1,n1,n1,   p1,   p1,  [ba]= 1, 1,-1,-1,-1,-1,-1,-1, 1,-1, 1, 0,-1, 0, 1,-1, 0, 1},      //stack size delta
ks[]={                            [ba]=-1,-1, 0, 0, 0, 0, 0, 0, 0, 0,-1, 1, 0, 0, 0, 0, 0, 0};      //stack size delta (coefficient for the next byte)
#define BG (b+=2,b[-2]|(U)b[-1]<<8)                                                                 //read a 2-byte little-endian global/var index
// amber 2.4: tail calls. A call whose result is the function's result (the next opcode is the
// end, maybe after jumps out of a $[..]) to a lambda that takes exactly that many arguments and
// fits in the current frame reuses the frame: no C recursion, no depth. {$[x;o x-1;0]}1000000.
Z B tlc(CO UC*b)_(W(*b==bj,b+=2+b[1])!*b)                                                           //is the code at b just the way out?
// amber 2.3: the 'noupdate raise for the global-store opcodes below, kept out of line
// and cold: inlined into run() it cost ~8% on a loop of small vector ops (register
// allocation for the whole dispatch loop), though the check itself never fires.
// A failed indexed assignment (a[i]:v, a[i]+:v) used to lose the variable: d4 amends it in place
// and its result, 0 on an error, was stored back. So check first what can be checked without
// touching it: for a list, each int index in range, level by level, and where every level above
// the last is a single index, a list value of the last level's count. 0: go ahead (d4 decides the rest); 1: 'index; 2: 'length;
// 3: 'type. Dicts and tables with symbol keys are checked by ixkd, a level at a time like a list.
Z L ixe(A q,U j){UC t=_t(q);CO V*d=_V(q);return t==tL?((CO L*)d)[j]:t==tI?((CO I*)d)[j]:t==tH?((CO H*)d)[j]:t==tG?((CO G*)d)[j]:t==tE?*(CO L*)d+(L)j:((CO UC*)d)[j>>3]>>(j&7)&1;}//item j of an int list, without boxing it
Z I ixkd(A x,A y,U k,A z,B asg);
Z B ixone(A y,U k){if(!_tA(y))return 1;F(k,A q=_A(y)[i];if(q==au||!_tt(q))return 0)return 1;}   //levels 0..k-1 each a single index, so the value at level k is z itself
NI I ixck(A x,A y,U k,A z,B asg){I r=0;if(x&&k<_N(y)&&!_tP(x)&&(_t(x)==tm||_t(x)==tM))return ixkd(x,y,k,z,asg);
 if(x&&y!=au&&k<_N(y)&&(_t(x)!=ts&&LH(ti,_t(x),ts)||LH(tdt,_t(x),tnp)))return 3;   //an index into a number, char or temporal atom (:: as the index is the whole value); a symbol atom names a global, which the assignment amends
 if(!x||k>=_N(y)||_tP(x)||!_tT(x))return 0;U n=_N(x);B one=_N(y)==1;
 if(!_tP(y)&&LH(tE,_t(y),tL)){L v=ixe(y,k);if(v<0||v>=(L)n)return 1;if(one)return 0;A w=ii(x,(U)v);r=ixck(w,y,k+1,z,asg);mr(w);return r;}   //y an int list: an int index per level
 A q=_tA(y)?_R(_A(y)[k]):ii(y,k);
 if(q==au){if(!one)for(U j=0;j<n&&!r;j++){A v=ii(x,j);r=ixck(v,y,k+1,z,asg);mr(v);}}
 else if(_tz(q)){L v=gl_(q);if(v<0||v>=(L)n)r=1;else if(!one){A w=ii(x,(U)v);r=ixck(w,y,k+1,z,asg);mr(w);}}
 else if(!_tP(q)&&LH(tE,_t(q),tL)){U m=_N(q);
  if(one&&m&&_t(q)!=tB){L lo=_t(q)==tE?ixe(q,0):minfZ(WL,q),hi=_t(q)==tE?ixe(q,m-1):maxfZ(NL,q);if(lo<0||hi>=(L)n)r=1;}   //one level: the vector min/max, not a type switch per item
  else for(U j=0;j<m&&!r;j++){L v=ixe(q,j);if(v<0||v>=(L)n)r=1;else if(!one){A w=ii(x,(U)v);r=ixck(w,y,k+1,z,asg);mr(w);}}
  if(!r&&(one||k+1==_N(y)&&ixone(y,k))&&!_tP(z)&&_tT(z)&&_N(z)!=m)r=2;}
 else if(_t(q)==ts||_t(q)==tS&&_N(q))r=3;   //a symbol indexes no list
 mr(q);return r;}
//a dict or table (x) at level k: a key that is missing is added by the assignment, so only at the last
//level is it fine; a key that is there goes a level down; at the last level a list of keys needs a
//list value of its count, and a table's column, assigned with : (asg), one of its row count (any other verb
//gets the whole column, so only its result has to fit); a table's row is an int in range
Z I ixkd(A x,A y,U k,A z,B asg){A ks=_x(x),vs=_y(x);B last=k+1>=_N(y),tb=_t(x)==tM,own=!_tA(y);I r=0;L rows=0;
 if(_t(ks)!=tS||tb&&!_N(vs))return 0;   //symbol keys only (every table's), and a table with columns
 A q=own?ii(y,k):_A(y)[k];B zlist=!_tP(z)&&_tT(z);   //q: borrowed from y, or made from a typed y
 if(last&&!tb){if(_t(q)==tS&&zlist&&_N(z)!=_N(q)&&ixone(y,k))r=2;I(own,mr(q))return r;}   //a dict's last level: any key may be set; a list of keys needs a value of its count
 if(tb)rows=_N(_A(vs)[0]);
 if(q==au){if(!last&&!tb&&_N(vs)){A v=ii(vs,0);r=ixck(v,y,k+1,z,asg);mr(v);}}   //an elided level adds the key :: with the first value nulled (as ngn/k), and indexes that: so only the first value's shape counts
 else if(_t(q)==ts){
  if(last){if(tb&&asg&&zlist&&_N(z)!=(U)rows&&ixone(y,k)){r=2;if(_N(ks)==1){A f=fnd(ks,_R(q));I(!gl_(f),r=0)mr(f);}}}   //a table's column needs the row count (for :), unless it is the only column, which may take another
  else{A f=fnd(ks,_R(q));L j=gl_(f);mr(f);B kin=j>=0&&j<(L)_N(ks);if(kin||!tb&&_N(vs)){A w=ii(vs,kin?(U)j:0);r=ixck(w,y,k+1,z,asg);mr(w);}}}   //a missing key is added with the first value nulled (as ngn/k), so that is what is indexed
 else if(_t(q)==tS){U m=_N(q);
  if(!last){A f=fnd(ks,_R(q));for(U i=0;i<m&&!r;i++){L j=ixe(f,i);B kin=j>=0&&j<(L)_N(ks);if(kin||!tb&&_N(vs)){A w=ii(vs,kin?(U)j:0);r=ixck(w,y,k+1,z,asg);mr(w);}}mr(f);}}   //a table's list of columns at the last level is not checked
 else if(tb&&_tz(q)){L v=gl_(q);if(v<0||v>=rows)r=1;else if(!last&&!(k+2==_N(y)&&_tA(y)&&_t(_A(y)[k+1])==ts)){A w=ii(x,(U)v);r=ixck(w,y,k+1,z,asg);mr(w);}}   //a row then one column name: nothing to check, so the row is not built
 else if(tb&&!_tP(q)&&LH(tE,_t(q),tL)){U m=_N(q);B dn=!last&&!(k+2==_N(y)&&_tA(y)&&_t(_A(y)[k+1])==ts);for(U i=0;i<m&&!r;i++){L v=ixe(q,i);if(v<0||v>=rows)r=1;else if(dn){A w=ii(x,(U)v);r=ixck(w,y,k+1,z,asg);mr(w);}}}   //rows then one column name: only the rows are checked, so no row is built
 I(own,mr(q))return r;}
//an amend at y where every level is one index that is there (an int in range of a list or of a table's rows, a key of a dict
//of symbol keys, a column of a table): its verb can go first and the result be assigned, so a verb that fails touches nothing.
//ixwk walks y down x: the levels (at most 8; 0 for any other index), what each is in kd (list, key, column, row) and its place,
//and in *q the item at the end. ixst then assigns r there, as a8 does with :, but without going down again.
U ixwk(A x,A y,UC*kd,L*ix,A*q){B at=_tt(y),ty=!at&&LH(tE,_t(y),tL);U m=at?1:_N(y);if(!m||m>8||!at&&!ty&&!_tA(y)&&_t(y)!=tS)return 0;A v=x;
 for(U k=0;k<m;k++){A w=0,s=at?y:ty?0:_tA(y)?_A(y)[k]:ii(y,k);L j=ty?ixe(y,k):_tz(s)?gl_(s):-1;UC t=0;   //s: borrowed, or a packed symbol
  if(!_tP(v)&&_tT(v)){if(j>=0&&j<(L)_N(v))w=ii(v,(U)j);}
  else if(!_tP(v)&&(_t(v)==tm||_t(v)==tM)&&_t(_x(v))==tS){A ks=_x(v),vs=_y(v);
   if(!ty&&_t(s)==ts){A f=fnd(ks,_R(s));j=gl_(f);mr(f);if(j>=0&&j<(L)_N(ks))w=ii(vs,(U)j),t=_t(v)==tM?2:1;}
   else if(_t(v)==tM&&_N(vs)&&j>=0&&j<(L)_N(_A(vs)[0]))w=ii(v,(U)j),t=3;}
  if(v!=x)mr(v);if(!w)return 0;kd[k]=t;ix[k]=j;v=w;}
 *q=v;return m;}
//x[y]f:z, x the variable at p: checked first, and where ixwk finds the item, the verb (dyads[d]) goes first and ixst (a.c)
//assigns its result, so a verb that fails leaves the variable as it was. The result, or 0 on an error. Out of run, which it would slow.
//A verb that sets the variable (a function item, by @: or .:) leaves it as it set it, and the assignment's value (in *v, for by)
//is then the verb's result, not the new value at y (indexing that could call a function again)
A ixst(A,A,CO UC*,CO L*,U,U,A);
Z NI A ixas(A*p,A x,A y,UC d,A z,A*v){I e=ixck(x,y,0,z,!d);if(e)return e==1?ei0():e==2?el0():et0();UC kd[8];L ix[8];A q;U m=d?ixwk(x,y,kd,ix,&q):0;
 if(!m)return *p=d4(x,y,av+d,z);RC_INC(x);A r=v2[d](q,_R(z));mr(q);   //x (not packed: ixwk went into it) held across the verb: a function it calls (@:, .:) can set the variable, which would free x
 if(*p!=x){mr(x);P(!r,0)I(v,*v=r)E(mr(r));return *p;}   //it did: the variable keeps what the function set it to
 (V)RC_DECV(x);if(!r)return 0;return *p=ixst(x,y,kd,ix,0,m,r);}   //the variable still holds x
Z NI __attribute__((cold)) V noupd(A*s){mr(*s);*s=err0("noupdate");}
AX(run,Q(xto)Z AM_TLS_IE I d;P(++d>2048,es8(a,n))/*d: per-thread VM recursion depth (peach workers run the VM concurrently)*/P(n-xk,er8(a,n))UC*b=_V(xy),c,nl=_n(xA[3]);A own=0,l[nl+*b++],*s=l+L(l);MS(l,0,SZ l);I(n,MC(l,a,8*n))//virtual machine
 W((c=*b++),S(c,                                                                                    //          |BYTES |          STACK        |         EFFECT
  C32(bu,U(*s=v1[c-bu](*s)))                                                                        //monad     |bu+m  |.. x -> monads[m][x]   |
  C32(bv,A x=*s++;U(*s=x(v2[c-bv](x,*s))))                                                          //dyad      |bv+d  |.. y x -> dyads[d][x;y]|
  C16(bs,A*p=l+c%16;I(*p,mr(*p))*p=*s++)                                                            //set local |bs+i  |.. x -> ..             |locals[i]:x
  C16(bg,A*p=l+c%16,x=*p;U(*--s=x)xR)                                                               //get local |bg+i  |.. -> .. locals[i]     |
  C16(bd,A*p=l+c%16,x=*p;U(*--s=x)*p=0)                                                             //del local |bd+i  |.. -> .. locals[i]     |locals[i]:NULL (freed)
  C2(ba,bp,UC m=*b;A f=*s;                                                                          //apply|proj|ba,n  |.. z y x -> .. x[y;z]  |
   I(c==ba&&tlc(b+1)&&s+m+1==l+L(l)&&_t(f)==to&&_k(f)==m&&_n(_A(f)[3])+*(UC*)_V(_A(f)[1])<=L(l),        //tail call: the frame becomes f's
    A t[8];MC(t,s+1,8*m);*s=0;MS(s+1,0,8*m);F(nl,A y=l[i];I(y,mr(y)))MS(l,0,SZ l);I(own,mr(own))
    own=x=f;nl=_n(xA[3]);b=(UC*)_V(xy)+1;MC(l,t,8*m);s=l+L(l))
   E(b++;UC n=m;A x=f;s+=n;U(*s=x((c==ba?_8:prj)(x,s-n+1,n)))))
  C6(bm,bM,bx,bX,by,bY,A*p=(c&1?gv:l)+BG,x=*p;I(__builtin_expect(c&1&&ray_rc_sync,0),noupd(s);goto l)  //          |      |                       |
   U(x,*s=ev(*s))A y=*s++;                                                                           //          |      |                       |
   I(c==bm||c==bM,y=v2[*b++](x,y);U(y,*--s=0)*p=x(y))                                               //mod asgn  |bm,i,d|.. x -> ..             |vars[i]:dyads[d][vars[i];x]
   E(A v=0;x=ixas(p,x,y,*b++,*s,c>=by?&v:0);mr(*s);I(c==bx||c==bX,mr(y);U(x,*s=0)s++)                    //ind asgn  |bx,i,d|.. z y -> ..           |vars[i]:  .[vars[i];y;dyads[d];z]
                                    E(U(x,*s=y(0))U(*s=v?y(v):dot(x,y)))))                          //ind asgn  |by,i,d|.. z y -> .. r         |vars[i]:r:.[vars[i];y;dyads[d];z]
  C(bG,A x=*--s=gv[BG];U(x,ev0())xR)                                                                //get global|bG,i,i|.. -> .. globals[i]    |
  // amber 2.3: a peach dispatch runs this VM on several threads at once, and a
  // global store there raced on gv[] (two workers releasing the same old value
  // is a double free). As in q, a parallel worker may read globals but not set
  // them: 'noupdate. Serial each, and peach with one lane, are unaffected.
  C(bS,A*p=gv+BG;I(__builtin_expect(ray_rc_sync,0),noupd(s);goto l)A x=*s++,y=*p;*p=y?y(x):x)              //set global|bS,i,i|.. x -> ..             |globals[i]:x
  C(bl,UC n=*b++;s+=n-1;*s=sqz(aV(tA,n,s-n+1)))                                                     //list      |bl,n  |.. y x -> .. (x;y)     |
  C(bL,UC n=*b++;A x=*s;U(xtt||xN==n,*s=el(x))F(n,*--s=ii(x,n-1-i)))                                //unlist    |bL,n  |.. x -> .. x[0] x[1]   |
  C(bj,UC n=*b++;b+=n)                                                                              //jump      |bj,n  |.. x -> ..             |PC+:n
  C(bz,UC n=*b++;b+=n*!tru(*s++))                                                                   //branch    |bz,n  |.. x -> ..             |if x is falsy, PC+:n
  C(bo,*--s=xR)                                                                                     //recur     |bo    |.. -> .. o             |o is the current lambda
  C(bP,mr(*s++))                                                                                    //pop       |bP    |.. x -> ..             |
  C(bV,UC i=*b++;U(*s=v2[*b++](xA[i+OFF],*s)))                                                      //const dyad|bV,i,d|.. x -> .. r           |r:dyads[d][consts[i];x]
  D(*--s=_R(xA[c-bc+OFF]))))                                                                        //const     |bc+i  |.. -> .. consts[i]     |
 l:d--;A u=*s;MS(l+nl,0,s-l-nl+1<<3);F(L(l),A x=l[i];I(x,mr(x)))I(!u,eS(xx,(UC)_C(xz)[(C*)b-1-_C(xy)]))I(own,mr(own))u)

#define Nr(a...) {I r_=cr(a);P(r_-OK,r_);}                                                          //compile rvalue; return on error
#define Nl(a...) {I r_=cl(a);P(r_-OK,r_);}                                                          //compile lvalue; return on error
#define OK -1                                                                                       //returned by cl() and cr() on success
#define MB 2048                                                                                     //max bytecode size
#define M(a) {b[nb]=a;m[nb]=o;nb+=nb<MB-1;}                                                         //append byte
#define MG(a) {U ig_=(a);M(ig_&255)M(ig_>>8)}                                                       //append a 2-byte little-endian global/var index
Z A u;Z UC b[MB],m[MB];Z I lu[16],nb,nl,l[16],cr(A,B);                                              //u:lambda(src;b:bytes;m:map;l:locals;consts..)  lu:last usages
ZN I li(I v)_(U i=fI(l,nl,v);P(i==nl,-1)lu[i]=nb;i)                                                 //index of a local variable (returns -1 if not found)
Z B cm(A x/*0*/){X(Rv(!xv)Ru(1)RS(P(xn-1,0)S s=su(*xI);U n=SL(s);n&&s[n-1]==':')R_(0))}             //is x a valid modifier? i.e. :: or primitive monad or symbol ending with ":"
// amber 2.3: two constants share a pool slot only if IDENTICAL. ~ calls -0.0 and
// 0.0 (and any two NaNs) one value, so deduplicating on ~ turned {1%x*-0.0} into
// 0w whenever a 0.0 literal came first. Floats must agree bit for bit, down lists.
Z B cid(A x,A y)_(P(!mtc_(x,y),0)UC t=_t(x);P(t==tf||t==tF,_t(y)==t&&!memcmp(_V(x),_V(y),(N)(t==tf?1:_n(x))*SZ(F)))
 P((t==tA||t==tm||t==tM)&&_t(y)==t,U m=_n(x)|!_n(x);F(m,P(!cid(_A(x)[i],_A(y)[i]),0))1)1)
Z V cc(A x/*0*/,I o){U n=un,i=OFF;W(i<n&&!cid(x,ua),i++)I(i>=n,PSH(u,xR))M(i+bc-OFF)}              //append a "load constant" instruction
Z B lim;//cr: an expression exceeds a limit of the bytecode ('limit rather than 'compile)
Z I cl(A x,A y/*00*/,B r){Q(cm(xx))I v=_v(xx),o=xo;                                                 //compile lvalue (x:assignmentNode,y:tree,r:wantResult)
 Y(R_(o)
   RS(I(yn==1,I w=*yI,i=li(w);P(xx==av&&nl,I(i<0,i=nl;P(i>15,lim=1;o)l[nl++]=w;lu[i]=nb)M(bs+i)I(r,M(bg+i))OK)P(i>=0,M(bm)MG(i)M(v)I(r,M(bg+i))OK))
      U i=gi(y);M(v?bM:bS)MG(i)I(v,M(v))I(r,M(bG)MG(i))OK)
   RA(I n=yn-1;P(!n,o)P(n>8u,lim=1;o)A z=yx;P(z==MKL&&(xx==av||_t(xx)==tu),M(bL)M(n)F(n,Nl(x,yA[i+1],0))I(r,P(xx-av,o))E(M(bP))OK)
      ZS(F(n,Nr(yA[n-i],1))M(bl)M(n)I i=zn-1?-1:li(*zI);I(i>=0,M(r?by:bx))E(i=gi(z);M(r?bY:bX))MG(i)M(v)OK)o))}
// ---- amber 2.1: idiom fusion ---------------------------------------------
// A handful of expression SHAPES are compiled to one fused primitive instead
// of a chain of primitives that materialise intermediates. Every fused entry
// point reproduces the unfused semantics exactly (it falls back to the very
// same primitives when the operands are not the flat vectors it handles), so
// this is a pure rewrite of the bytecode, invisible to the program:
//   +/x*y  +/x=y  +/x<y  +/x>y    ->  fredC   (no product / mask vector)
//   x@&m                          ->  cmprC   (no index vector)
//   a+s*b  a-s*b  (s a literal)   ->  fmaC    (one pass, no s*b vector)
//   x@<x   x@>x                   ->  srtC / srtdC (sort by value, no grade)
Z B fnode(A x,A v)_(x!=GAP&&!_tU(x))                                             //an operand node (not a gap, not a bare verb)
Z B numlit(A x)_(_tz(x)||_tf(x))                                                //a literal number in the tree
// amber 2.3: `k_v` with k a literal int and v a plain variable -> v, else 0
Z A shv(A t,L k)_(P(!_tA(t)||_n(t)!=3||_A(t)[0]!=UND,0)A l=_A(t)[1],v=_A(t)[2];P(!_tz(l)||gl_(l)!=k||!_tS(v),0)v)
// (1_v) and ((-1)_v) of the SAME variable, either way round: -1 none, else flip
Z I shpair(A oa,A ob)_(A u=shv(oa,1),w=shv(ob,-1);P(u&&w&&mtc_(u,w),0)u=shv(oa,-1);w=shv(ob,1);P(u&&w&&mtc_(u,w),1)-1)
// amber 2.3: is m a comparison node `c OP k`? -> op (0 <, 1 >, 2 =), c and k set;
// else -1. A literal on the LEFT is moved right (`50<x` is `x>50`) so that it
// reaches the scalar form of the kernel; a literal has no side effect, so the
// order in which the two operands are then evaluated cannot matter.
Z I cmpn(A m,A*c,A*k)_(P(!_tA(m)||_n(m)!=3,-1)A d=_A(m)[0];I op=d==LTN?0:d==GTN?1:d==EQL?2:-1;P(op<0,-1)
 A p=_A(m)[1],q=_A(m)[2];P(!fnode(p,0)||!fnode(q,0),-1)I(numlit(p)&&!numlit(q),SW(p,q)I(op<2,op^=1))*c=p;*k=q;op)
Z I fus(A x,B r){U n=xn;A y=xx;I o=xo;                                                //OK when it emitted fused code, -2 when not an idiom, else the error
 I(n==2&&((_tA(y)&&_n(y)==2&&_A(y)[0]==aw+1&&_A(y)[1]==ADD)||(!_tP(y)&&_t(y)==tr&&_E(y)==1&&_n(y)==1&&_A(y)[0]==ADD)),A z=xy;// +/ (folded by cf() into a tr constant, or still a node) applied to a dyad node
  // +/(1_v) CMP ((-1)_v) : count neighbour comparisons in one read of v (amber 2.3)
  I(_tA(z)&&_n(z)==3,A d=_A(z)[0];I(d==EQL||d==LTN||d==GTN,I fl=shpair(_A(z)[1],_A(z)[2]);
   I(fl>=0,cc(au,xo);Nr(shv(_A(z)[1],fl?-1:1),1)cc(ai(300+_v(d)+16*fl),xo);cc(FUS1,xo);M(ba)M(3)I(!r,M(bP))return OK;)))
  I(_tA(z)&&_n(z)==3,A d=_A(z)[0];I(d==MUL||d==EQL||d==LTN||d==GTN,A oa=_A(z)[1],ob=_A(z)[2];
   I(fnode(oa,0)&&fnode(ob,0),Nr(ob,1)Nr(oa,1)cc(ai(_v(d)),xo);cc(FUS1,xo);M(ba)M(3)I(!r,M(bP))return OK;))
   // +/x@&m : sum of the masked elements, no compressed vector
   I(d==AP1,A oa=_A(z)[1],w=_A(z)[2];I(_tA(w)&&_n(w)==2&&_A(w)[0]==WHR&&fnode(oa,0)&&fnode(_A(w)[1],0),
    // amber 2.2: and when the thing being masked is itself `a +- s*b` with a
    // literal scalar, the arithmetic joins the same pass -- otherwise the FMA
    // rule below fires on the operand and materialises a full-width vector that
    // only this sum ever reads. Tried before the plain form so the narrower
    // shape wins; if it does not match, nothing has been emitted yet.
    I(_tA(oa)&&_n(oa)==3&&(_A(oa)[0]==ADD||_A(oa)[0]==SUB),A zm=_A(oa)[2],aa=_A(oa)[1];
     I(_tA(zm)&&_n(zm)==3&&_A(zm)[0]==MUL,A p=_A(zm)[1],q=_A(zm)[2];
      A sc=numlit(p)?p:numlit(q)?q:0,ob=sc==p?q:p;
      I(sc&&fnode(aa,0)&&fnode(ob,0)&&!numlit(ob),
       // amber 2.3: the mask a comparison too -- +/(a+-s*b)@&(c OP k), six arguments
       A c_=0,k_=0;I op_=cmpn(_A(w)[1],&c_,&k_);
       I(op_>=0,Nr(k_,1)Nr(c_,1)Nr(ob,1)Nr(sc,1)Nr(aa,1)cc(ai((_A(oa)[0]==SUB)+2*op_),xo);cc(FUS3,xo);M(ba)M(6)I(!r,M(bP))return OK;)
       Nr(_A(w)[1],1)Nr(ob,1)Nr(sc,1)Nr(aa,1)cc(ai(_A(oa)[0]==SUB),xo);cc(FUS3,xo);M(ba)M(5)I(!r,M(bP))return OK;)))
    // amber 2.3: +/x@&(c OP k) -- the mask computed in the summing loop
    A c_=0,k_=0;I op_=cmpn(_A(w)[1],&c_,&k_);
    I(op_>=0,Nr(k_,1)Nr(c_,1)Nr(oa,1)cc(ai(28+op_),xo);cc(FUS1,xo);M(ba)M(4)I(!r,M(bP))return OK;)
    Nr(_A(w)[1],1)Nr(oa,1)cc(ai(18),xo);cc(FUS1,xo);M(ba)M(3)I(!r,M(bP))return OK;))))
 // _x%y : floor division, one exact integer pass for integer data (amber 2.3)
 I(n==2&&y==FLR,A z=xy;I(_tA(z)&&_n(z)==3&&_A(z)[0]==DVD&&fnode(_A(z)[1],0)&&fnode(_A(z)[2],0),
   Nr(_A(z)[2],1)Nr(_A(z)[1],1)cc(ai(40),xo);cc(FUS1,xo);M(ba)M(3)I(!r,M(bP))return OK;))
 // #'=x : count per group in one pass, no index lists (amber 2.2)
 I(n==2&&((_tA(y)&&_n(y)==2&&_A(y)[0]==aw&&_A(y)[1]==LEN)||(!_tP(y)&&_t(y)==tr&&_E(y)==0&&_n(y)==1&&_A(y)[0]==LEN)),A z=xy;
  I(_tA(z)&&_n(z)==2&&_A(z)[0]==GRP&&fnode(_A(z)[1],0),cc(au,xo);Nr(_A(z)[1],1)cc(ai(19),xo);cc(FUS1,xo);M(ba)M(3)I(!r,M(bP))return OK;))
 // &(x OP y) : positions of a comparison without the mask (amber 2.2)
 I(n==2&&y==WHR,A z=xy;I(_tA(z)&&_n(z)==3,A d=_A(z)[0];I(d==EQL||d==LTN||d==GTN,A oa=_A(z)[1],ob=_A(z)[2];
   I(fnode(oa,0)&&fnode(ob,0),Nr(ob,1)Nr(oa,1)cc(ai(100+_v(d)),xo);cc(FUS1,xo);M(ba)M(3)I(!r,M(bP))return OK;))))
 I(n==3&&y==AP1,A oa=xy,z=xz;
  // s@&(x OP y) : compress by a comparison without the mask (amber 2.2)
  I(_tA(z)&&_n(z)==2&&_A(z)[0]==WHR&&fnode(oa,0),A w=_A(z)[1];I(_tA(w)&&_n(w)==3,A d=_A(w)[0];I(d==EQL||d==LTN||d==GTN,A p=_A(w)[1],q=_A(w)[2];
    I(fnode(p,0)&&fnode(q,0),Nr(q,1)Nr(p,1)Nr(oa,1)cc(ai(100+_v(d)),xo);cc(FUS2,xo);M(ba)M(4)I(!r,M(bP))return OK;))))
  I(_tA(z)&&_n(z)==2&&_A(z)[0]==WHR&&fnode(oa,0)&&fnode(_A(z)[1],0),Nr(_A(z)[1],1)Nr(oa,1)M(bv+27)I(!r,M(bP))return OK;)   // x@&m
  I(_tA(z)&&_n(z)==2&&(_A(z)[0]==ASC||_A(z)[0]==DSC)&&_tS(oa)&&_tS(_A(z)[1])&&mtc_(oa,_A(z)[1]),Nr(oa,1)M(bu+(_A(z)[0]==ASC?27:28))I(!r,M(bP))return OK;))// x@<x
 // (1_v) OP ((-1)_v) and the flipped form: neighbour-wise dyad, v read once (amber 2.3)
 I(n==3&&(y==ADD||y==SUB||y==MUL||y==DVD||y==MNM||y==MXM||y==LTN||y==GTN||y==EQL),I fl=shpair(xy,xz);
  I(fl>=0,cc(au,xo);Nr(shv(xy,fl?-1:1),1)cc(ai(200+_v(y)+16*fl),xo);cc(FUS1,xo);M(ba)M(3)I(!r,M(bP))return OK;))
 I(n==3&&(y==ADD||y==SUB),A oa=xy,z=xz;
  I(_tA(z)&&_n(z)==3&&_A(z)[0]==MUL,A p=_A(z)[1],q=_A(z)[2];A sc=numlit(p)?p:numlit(q)?q:0,ob=sc==p?q:p;
   I(sc&&fnode(oa,0)&&fnode(ob,0)&&!numlit(ob),Nr(ob,1)Nr(sc,1)Nr(oa,1)cc(ai(y==SUB),xo);cc(FUS2,xo);M(ba)M(4)I(!r,M(bP))return OK;)))
 return -2;}
Z I cr(A x/*0*/,B r)_(I o=xo;                                                                       //compile rvalue (x:tree,r:wantResult)
 XS(I i=xn-1?-1:li(*xI);I(i>=0,M(bg+i))J(xn==1&&*xI=='o',M(bo))E(M(bG)MG(gi(x)))I(!r,M(bP))OK)       // x.y      variable (possibly qualified)
 P(!xtA||!xn,I(r,cc(x-GAP?x:au,o))OK)                                                               // 0        constant
 U n=xn;A y=xx;                                                                                     //
 P(y==GAP,F(n-1,Nr(xA[i+1],i==n-2&&r))OK)                                                           // [x;y]    block
 P(n==1,I(r,cc(y,o))OK)                                                                             // `a       quoted
 P(n==3&&cm(y)&&_tsSA(xy),
  YS(Nr(xz,1);Nr(xy,1);A z=enl(cS(drp(-1,str(ii(y,0)))));Nr(z,1);mr(z);M(ba)M(2)z=aA1(au);Nl(z,xy,r);z(0);OK)
  Nr(xz,1);Nl(x,xy,r);OK)// x[y]+:z     assignment
 P(n>3&&(y==av||y==DLR),n--;I p[n];A*a=xA;F(n&~1,Nr(*++a,1);M(i&1?bj:bz)p[i]=nb;M(0))               // :[x;y;z] cond
  Nr(n&1?*++a:au,1);F(n&~1,I d=(i&1?nb-1:p[i+1])-p[i];I(i&1,I j=(n&~1)-1;W(i<j&&d>255,d=p[j]-2-p[i];j-=2))P(d>255,lim=1;o)b[p[i]]=d)I(!r,M(bP))OK)
 I(n==2&&y==FIR,A z=xy;I(ztA&&zn==2,P(zx-REV<3u,Nr(zy,1);M(bu+zx-REV+LAS-au)I(!r,M(bP))OK)))        // *|x      recognized idioms
 {I f_=fus(x,r);P(f_!=-2,f_)}                                                                        // amber 2.1 fused idioms
 I p=0;F(n-1,A z=xA[n-1-i];I(z-GAP,Nr(z,1))E(p=1;cc(GAP,o)))I(p,Nr(xx,1);M(bp)M(n-1))               // x[y;]    projection
 J(y==MKL,n--;P(n>255u,lim=1;o)M(bl)M(n))                                                                 // (x;y)    list
 J(n==2&&ytu,M(bu+yv))                                                                              // +x       monad
 J(n==3&&ytv,I(!p&&!_tSA(xy),Q(b[nb-1]>=bc);I i=b[nb-1]-bc;b[nb-1]=bV;M(i)M(yv))E(M(bv+yv)))        // x+y      dyad
 E(P(n>9,lim=1;o)Nr(xx,1)M(ba)M(n-1))                                                                     // x[y]     application
 I(!r,M(bP))OK)
A1(qte,/*1*/xtS||xtA?aA1(x):x)                                                                      //quote
Z A2(c2,/*00*/P(xtw&&!ytSA,1)/*P(x==TIL&&ytZ&&yn<4,F(yn,P(gl(ii(y,i))>100u,0))1)*/0)                //constant folding
Z A3(c3,/*000*/P(ADD<=x&&x<=MUL&&ytzZ&&ztzZ&&(ytt||ztt||yn==zn)&&MAX(xN,yN)<101,1)0)                //constant folding
Z A1(cf,P(!xtA||!xn,x)P(xx==MKL,F(xn,A y=xa;YSA(x))qte(N(drp(1,x))))P(xn==2?c2(xx,xy):xn==3?c3(xx,xy,xz):0,qte(N(val(x))))A y=rsz(xn,au);F(xn,ya=cf(xa);xa=au;P(!ya,die("CF")))AO(xo,x(y)))
Z I mxs(I i,I s)_(I r=s;W(1,UC c=MIN(bc,b[i++]);r=MAX(r,s);P(!c,r)s+=ds[c]+ks[c]*b[i];i+=di[c]+(c==bj)*b[i];I(c==bz,r=MAX(r,mxs(i+b[i-1],s))))r)//max stack
Z B shy(A x/*0*/)_(!xtA?0:xn&&xx==GAP?shy(xA[xn-1]):xn==3&&cm(xx)&&_tSA(xy))                        //is last expr an assignment?
Z A3(cpl_,/*111*/nb=1;MS(lu,-1,SZ lu);I k=0;I(z,k=zn;MC(l,zV,OFF*k);z(0))nl=k;u=aA(OFF);y=Nx(cf(y));ux=x;uy=uz=uA[3]=au;B s=shy(y);I r=cr(y,!s);y(0);P(r-OK||gfull,gfull||lim?(gfull=lim=0,ez0()):ec0();eS(ux,r-OK?r:0);u(0))
 I o=0;I(s,cc(au,o))P(un>255||nb>MB-2||nl>L(l)-2,ez0();eS(ux,0);u(0))M(bu)P(nb>MB-2||un>255-bc+OFF,ez0();eS(ux,0);u(0))   //ez0 first, as above: eS marks the error ez0 makes (the other order read a null error)
 F(nl,I j=lu[i];I(j>=0&&b[j]==bg,b[j]=bd))I sx=mxs(1,0);P(sx>255,ez0();eS(ux,0);u(0))*b=sx;*m=-1;uy=aCn(b,nb);uz=aCn(m,nb);uA[3]=aV(tS,nl,l);AK(k,AT(to,u)))
A3(cpl,P(!ray_rc_sync,cpl_(x,y,z))plk(1);A r=cpl_(x,y,z);plk(0);r)                                 //cpl_ under the peach parse lock (m.c plk)
#undef M
