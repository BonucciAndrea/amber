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
#define bF (bu+31) //Amber 2.5 (exp): fused float tree, bF j c p (the unused monad slot 31: no opcode moves)
Z CO C di[]={[bF]=3,              [ba]= 1, 1, 3, 3, 3, 3, 3, 3, 2, 2, 1, 1, 1, 1, 0, 0, 2, 0},      //Extra bytes after opcode (2-byte global/var index)
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
 if(!x||k>=_N(y)||_tP(x)||!_tT(x))return 0;U n=_N(x);B one=_N(y)==1,sd=_t(x)==tS&&k&&k+1<_N(y);   //sd: a symbol list below the first level is data, and its items symbols, so a level below them is 'type
 #define IXD(w) (sd?3:ixck(w,y,k+1,z,asg))
 if(!_tP(y)&&LH(tE,_t(y),tL)){L v=ixe(y,k);if(v<0||v>=(L)n)return 1;if(one)return 0;A w=ii(x,(U)v);r=IXD(w);mr(w);return r;}   //y an int list: an int index per level
 A q=_tA(y)?_R(_A(y)[k]):ii(y,k);
 B dn=k+1<_N(y);   //a level below to check: else no item is built
 if(q==au){if(dn)for(U j=0;j<n&&!r;j++){A v=ii(x,j);r=IXD(v);mr(v);}}   //(sd: 'type, as there is an item)
 else if(_tz(q)){L v=gl_(q);if(v<0||v>=(L)n)r=1;else if(dn){A w=ii(x,(U)v);r=IXD(w);mr(w);}}
 else if(!_tP(q)&&LH(tE,_t(q),tL)){U m=_N(q);
  if(one&&m&&_t(q)!=tB){L lo=_t(q)==tE?ixe(q,0):minfZ(WL,q),hi=_t(q)==tE?ixe(q,m-1):maxfZ(NL,q);if(lo<0||hi>=(L)n)r=1;}   //one level: the vector min/max, not a type switch per item
  else{for(U j=0;j<m&&!r;j++){L v=ixe(q,j);if(v<0||v>=(L)n)r=1;else if(dn&&!sd){A w=ii(x,(U)v);r=ixck(w,y,k+1,z,asg);mr(w);}}I(!r&&dn&&sd&&m,r=3)}   //every index in range first, as without sd
  if(!r&&(one||k+1==_N(y)&&ixone(y,k))&&!_tP(z)&&_tT(z)&&_N(z)!=m)r=2;}
 else if(_t(q)==ts||_t(q)==tS&&_N(q))r=3;   //a symbol indexes no list
 mr(q);return r;}
#undef IXD
//a dict or table (x) at level k, below where ixwk went (a list of indices, an elided level or a list of keys above,
//or the walk stopped short): at the last level a list of keys needs a list value of its count, and a table's column,
//assigned with : (asg), one of its row count (any other verb gets the whole column, so only its result has to fit);
//a table's row is an int in range; an elided level on a dict is every key, as when indexing, so every value is
//indexed. A key or a list of keys above the last level is not looked up, and nothing below it is checked: the
//assignment finds it, and the check would find it a second time
Z I ixkd(A x,A y,U k,A z,B asg){A ks=_x(x),vs=_y(x);B last=k+1>=_N(y),tb=_t(x)==tM,own=!_tA(y);I r=0;L rows=0;
 if(_t(ks)!=tS||tb&&!_N(vs))return 0;   //symbol keys only (every table's), and a table with columns
 A q=own?ii(y,k):_A(y)[k];B zlist=!_tP(z)&&_tT(z);   //q: borrowed from y, or made from a typed y
 if(last&&!tb){if(_t(q)==tS&&zlist&&_N(z)!=_N(q)&&ixone(y,k))r=2;I(own,mr(q))return r;}   //a dict's last level: any key may be set; a list of keys needs a value of its count
 if(tb)rows=_N(_A(vs)[0]);
 if(q==au){if(!last&&!tb)for(U j=0;j<_N(vs)&&!r;j++){A v=ii(vs,j);r=ixck(v,y,k+1,z,asg);mr(v);}}   //an elided level on a dict is every key (a8), so every value is indexed
 else if(_t(q)==ts){
  if(last){if(tb&&asg&&zlist&&_N(z)!=(U)rows&&ixone(y,k)){r=2;I(_N(ks)==1&&!fI(_I(ks),1,_v(q)),r=0)}}}   //a table's column needs the row count (for :), unless it is the only column, which may take another
 else if(_t(q)==tS);   //a list of keys: not looked up above the last level, nor a table's list of columns at it
 else if(tb&&_tz(q)){L v=gl_(q);if(v<0||v>=rows)r=1;else if(k+2==_N(y)&&_tA(y)&&_t(_A(y)[k+1])==tS){A w=ii(x,(U)v);r=ixck(w,y,k+1,z,asg);mr(w);}}   //a row: built only for a list of columns at the
 else if(tb&&!_tP(q)&&LH(tE,_t(q),tL)){U m=_N(q);for(U i=0;i<m&&!r;i++){L v=ixe(q,i);if(v<0||v>=rows)r=1;}}   //last level (its count); else there is nothing more to check in it. Rows: their range
 I(own,mr(q))return r;}
//The check and then the assignment (d4) each looked up every key and column on the way down, so where every level of y
//is one index that is there -- an int in range of a list or of a table's rows, or a key of a dict of symbol keys or a
//column of a table, which may also be one to add -- ixwk walks y down x once, which checks it, and records in kd what
//each level is (0 list, 1 key, 2 column, 3 row, 4 key or 5 column to add above the last level, 7 or 8 when only an index
//into a list follows, 6 a row then its column, 9 a list of keys above the last level)
//and in ix its place (for 6, the column's in the next; for 9, the find of the keys), and ixst (a.c) assigns there without
//looking anything up again: from a row on, and from a key to add above the last level, as d4 and a8 do it from there;
//from a list of keys, as a8 does with its find (dam). Where the walk stops short (a list of ints, an elided level, an
//index into something else, or below the first level a symbol or a symbol list, which names a global), the check (ixck)
//goes on from that level, and the assignment is made from there (3, d8), or by d4 at the first level. Below where the walk
//went, the check looks no key up (ixkd): each key is found once. >0: the levels walked, for ixst; 0: d4; <0: -(ixck's error)
U tcc(A,A);
//A dict whose values are not a list is one an amend has taken them out of (ixst, or a8), and is amending: a symbol at a
//level below names a global, amended by name, which names this variable again (v:`a`b!(`v;1 2); v[`a;`b]:9). Refused
//('type) before it is touched, as the amend failed on it, which then freed it, and the caller again. gl: x is a global's
//value, assigned in run (ixca): where the walk meets a symbol below the first level, 512, so d4 assigns it (ixca);
//and +256 where an amend by name can follow ixst's places (a key to add or a list of keys, or the walk stopped short
//above the last level), so that ixca keeps the global's place while ixst assigns it
I ixwk(A x,A y,A z,B asg,B gl,UC*kd,L*ix){if(!x)return 0;if(_t(x)==tm&&!_tMT(_y(x)))return -3;UC ry=_t(y);B at=ry>tm,ty=LH(tE,ry,tL);U m=at?1:_N(y),k=0;I a=-1;   //a: the first level with a key to add
 if(y==au||!m||m>8||!at&&!ty&&ry!=tA&&ry!=tS)return -ixck(x,y,0,z,asg);A v=x;
 for(;k<m;k++){B last=k+1==m;A s=at?y:ty?0:_tA(y)?_A(y)[k]:ii(y,k),w=0;L j=ty?ixe(y,k):_tz(s)?gl_(s):-1;UC t=_tP(v)?0:_t(v);   //s: borrowed, or a packed symbol
  if(t&&t<tM){   //a list; a symbol list below the first level too, as data
   if(last&&!ty&&!_tP(s)&&LH(tE,_t(s),tL)&&_t(s)!=tB){U c=_N(s);I r=0;   //a list of ints at the last level, as ixck checks it: in range, and a list value of its count
    if(c){L lo=_t(s)==tE?ixe(s,0):minfZ(WL,s),hi=_t(s)==tE?ixe(s,c-1):maxfZ(NL,s);I(lo<0||hi>=(L)_N(v),r=-1)}
    I(!r&&!_tP(z)&&_tT(z)&&_N(z)!=c,r=-2)I(v!=x,mr(v))P(r,r)P(a>=0,a+1|gl<<8)P(!k,0)kd[k]=3;ix[k]=-1;return (I)k+1;}   //d8 there (3; no row: ix -1)
   if(j<0||j>=(L)_N(v))break;P(!last&&k&&t==tS,I(v!=x,mr(v))-3)kd[k]=0;ix[k]=j;I(!last,w=ii(v,(U)j))}   //its items are symbols: a level below one is 'type
  else if((t==tm||t==tM)&&_t(_x(v))==tS&&(t==tm||_N(_y(v)))){A ks=_x(v),vs=_y(v);
   if(!ty&&_t(s)==ts&&a<0){A f=fnd(ks,_R(s));j=gl_(f);mr(f);
    if(j<0||j>=(L)_N(ks)){j=_N(ks);if(!last){kd[k]=t==tm?4:5;ix[k]=j;I(a<0,a=(I)k)   //a key not there, which the assignment adds: above the last level with the
      if(t==tM||!_N(vs)){I(v!=x,mr(v))return a+1|gl<<8;}w=ii(vs,0);}}   //first value nulled, so the check goes on in the first value (a table's: nothing more to check)
    if(!w){if(t==tM&&last&&asg&&!_tP(z)&&_tT(z)&&_N(z)!=_N(_A(vs)[0])&&!(_N(ks)==1&&!j))break;   //a column assigned with : needs the row count
     kd[k]=t==tm?1:2;ix[k]=j;I(!last,w=ii(vs,(U)j))}}
   else if(t==tM&&(ty||_tz(s))){if(j<0||j>=(L)_N(_A(vs)[0])||!last&&!(k+2==m&&_tA(y)&&_t(_A(y)[k+1])==ts))break;kd[k]=3;ix[k]=j;
    if(!last){U c=tcc(v,_A(y)[k+1]);I(c,kd[k]=6;ix[k+1]=c-1)}k++;break;}   //a row then a column that d4 would amend on its own (tca, a.c): 6, which ixst does so
   else if(!last&&a<0&&!ty&&_t(s)==tS){A f=fnd(ks,_R(s));I r=0;B tb=t==tM;A e=k+2==m&&_tz(_A(y)[k+1])?_A(y)[k+1]:0;   //a list of keys above the last level: found
    P(_t(vs)==tM&&(k+2==m||_A(y)[k+1]!=au),I(v!=x,mr(v))kd[k]=9;ix[k]=(L)f;(I)(k+1|gl<<8))   //once, and its places (f) passed to ixst (9). Values that are dicts of the
    F(_N(s),L q=ixe(f,i);B kin=q>=0&&q<(L)_N(ks);A u=_tA(vs)&&(kin||!tb&&_N(vs))?_A(vs)[kin?q:0]:0;   //same keys (held as a table): only an elided level in them is checked
     if(e&&u&&!_tP(u)&&_tT(u)){L g=gl_(e);I(g<0||g>=(L)_N(u),r=1)}   //one int into a list below: its range, as ixck checks it, without taking the list
     else I(kin||!tb&&_N(vs),u=ii(vs,kin?(U)q:0);r=ixck(u,y,k+1,z,asg);mr(u))I(r,break))
    I(v!=x,mr(v))P(r,mr(f);-r)kd[k]=9;ix[k]=(L)f;return (I)(k+1|gl<<8);}   //as ixkd checks it: a missing key above the last level adds it, the first value nulled
   else if(t==tM&&s==au){I(v!=x,mr(v))P(a>=0,a+1|gl<<8)P(!k,0)kd[k]=3;ix[k]=-1;return (I)(k+1|(gl&&k+1<m)<<8);}   //every row of a table: nothing to check (ixkd), so not called
   else break;}
  else break;
  if(last){k++;break;}if(v!=x)mr(v);v=w;}
 if(k==m||k&&(kd[k-1]==3||kd[k-1]==6)){I(v!=x,mr(v))P(a>=0&&k==m&&a+2==(I)m&&!kd[a+1],kd[a]+=3;a+2)return a<0?(I)k:a+1|gl<<8;}   //7, 8: a key to add, then one index into a list
 I r=ixck(v,y,k,z,asg);B sy=gl&&k&&_ts(v);I(v!=x,mr(v))P(r,-r)P(sy,512)P(a>=0,a+1|gl<<8)P(!k,0)kd[k]=3;ix[k]=-1;return (I)(k+1|(gl&&k+1<m)<<8);}   //stopped short below the first level: d8 from there (3)
A ixst(A,A,CO UC*,CO L*,U,U,A,A,A*),ixv2(UC,A,A);
//the item at the places ixwk found, where each is one that is there -- an int into a list (0), a key or column there
//(1, 2: not one to add), a row then its column (6), or a table's row at the last level (3) -- so that the verb can go
//first, on it, and its result be assigned with :, and a verb that fails touches nothing. Else 0 (the amend goes as before).
//Borrowed from a general list (*o 0), else made, an atom or a row (*o 1): no count is touched on the way down x.
//Where the walk stopped at a table's row (3) with more levels below, or did not start (m 0: d4) at one at the first
//level (then kd[0] 3), the rest is read too, where each level below is one that is there (ixrr)
Z A ixrr(A,A,U,U);
NI __attribute__((cold)) A ixit(A x,A y,UC*kd,CO L*ix,U m,B*o){UC ry=_t(y);U my=ry>tm?1:_N(y);A v=x,w,c;B vo=0,wo;
 if(!m){P(ry!=tA||my<2||_t(x)!=tM||!_tz(*_A(y)),0)v=ixrr(x,y,0,my);I(v,kd[0]=3;*o=1)return v;}
 for(U k=0;k<m;k++){UC t=kd[k];L j=ix[k];   //c: the list the item is in
  c=!t?v:t<3?j<(L)_N(_x(v))?_y(v):0:t==6?_A(_y(v))[ix[k+1]]:t==3&&j>=0&&k+1==my&&_t(v)==tM&&(LH(tE,ry,tL)||(ry>tm?_tz(y):ry==tA&&_tz(_A(y)[k])))?v:0;   //3: a row,
  if(!c){w=t==3&&k+1==m&&ry==tA&&_t(v)==tM&&_tz(_A(y)[k])?ixrr(v,y,k,my):0;I(vo,mr(v))I(w,*o=1)return w;}   //or the rest below one
  wo=!_tA(c);w=wo?ii(c,(U)j):_A(c)[j];I(vo,I(!wo,w=_R(w);wo=1)mr(v))v=w;vo=wo;}   //not where the walk stopped short
 *o=vo;return v;}
//from row y[k] of table x on, each level one index that is there, as ixwk walks it: an int in range of a list (a
//symbol list too, as data) or of a table's rows, a key of a dict of symbol keys, or a table's column.
//The item, made; else 0, and the amend goes as before. Nothing below names a global (a symbol is an atom, read no further)
Z __attribute__((cold)) A ixrr(A x,A y,U k,U my){L j=gl_(_A(y)[k]);P(!_N(_y(x))||j<0||j>=(L)_N(*_A(_y(x))),0)A v=ii(x,(U)j);
 for(k++;k<my;k++){A s=_A(y)[k],w=0;UC t=_tP(v)?0:_t(v);
  if(t&&t<tM){I(_tz(s)&&(j=gl_(s))>=0&&j<(L)_N(v),w=ii(v,(U)j))}
  else if((t==tm||t==tM)&&_t(_x(v))==tS){A vs=_y(v);
   if(_ts(s)){A f=fnd(_x(v),_R(s));j=gl_(f);mr(f);I(j>=0&&j<(L)_N(_x(v))&&(t==tM||_tMT(vs)),w=ii(vs,(U)j))}
   else I(t==tM&&_tz(s)&&_N(vs)&&(j=gl_(s))>=0&&j<(L)_N(*_A(vs)),w=ii(v,(U)j))}
  mr(v);P(!w,0)v=w;}
 return v;}
//x[y]f:z (f dyads[d]) in run, the variable at p: ixca checks it (ixwk) and assigns it (by ixst where ixwk walked y, else
//as before), in one call, so what ixwk found stays in its frame: in run's frame it changed how run's whole loop was
//compiled, and made every bytecode a few percent slower; kept per thread between two calls, it cost a lookup each.
//Where ixit reads the item, the verb goes first and ixst assigns its result with :, so a verb that fails leaves the
//variable as it was. f#y, f_y, f@y and f.y call f, which may be an item of x: there the variable is whole while f runs,
//and held, so that f can set it (a global): if f then fails, the variable keeps what f set; if not, the assignment is
//made on the variable as it was, as before. Elsewhere f could see or set the variable while ixst has items out of it,
//so # _ @ and . go as before: ixck checks, and d4 assigns; a global held (ixfh) where an item they call can be a
//function (ixhr), so that d4 amends a copy of it, and the variable is then as where ixit reads the item. g f:y (bM)
//holds g so too (bmh), for the verb calls g itself.
//A global (g) is run's variable while it is assigned, which an amend by name (d8, a.c) in the assignment meets: a symbol
//below names a global, which may name the variable again, directly or through other globals (v:(1;(2;`v)); v[1;1;1;0]:9).
//So it is assigned by d4 where the walk meets a symbol (ixwk), held where a symbol on the way can name it (ixhr), and
//the global's place is kept, innermost first, in ixgs while it is assigned by d4 (m), or by ixst where an amend by name
//can follow it (ixwk's +256). An amend by name, or run's of a global, while one is assigned by d4 is made as before (by
//d4), and one of a global that is being assigned in place (by ixst, which has items out of it, or by d4, not held: h)
//is refused ('type) before it is touched, so the assignment fails: where it was held, the variable is kept. Held by d4
//from the first level, or for a function # _ @ or . calls (ixfh's h), the global is d4's own copy, so an amend of it by
//name is made, on the variable as it was. Only one thread (no peach) assigns a global
Z struct ixg{A*p;struct ixg*n;B m,h;}*ixgs;
I ixgn(A*p){I r=0;for(struct ixg*q=ixgs;q;q=q->n){if(q->p==p&&!q->m)return 2;r|=q->m;}return r;}   //2: refused; 1: by d4
I ixgh(A*p){for(struct ixg*q=ixgs;q;q=q->n)if(q->p==p)return 1;return 0;}   //is p's global being assigned?
I ixgi(A*p){for(struct ixg*q=ixgs;q;q=q->n)if(q->p==p&&!q->h)return 1;return 0;}   //in place (not held, h)?
Z A ixas(A x,A y,A z,UC d,CO UC*kd,CO L*ix,I m,A*u)_(m?ixst(x,y,kd,ix,0,(U)m,av+d,z,u):_tA(y)&&_n(y)==1?a4(x,*_A(y),av+d,z):d4(x,y,av+d,z))   //one level: what d4 does, without taking it out of y
Z NI A ixag(A*p,A x,A y,A z,UC d,CO UC*kd,CO L*ix,I m,B h){struct ixg e={p,ixgs,!m,h};ixgs=&e;x=ixas(x,y,z,d,kd,ix,m,0);ixgs=e.n;return x;}   //a global, kept in ixgs while assigned
//d4 on a global held, so that it amends its own copy, where ixhr finds it can be set or amended on the way: f by d4 (a
//list of indices, an elided level, a key to add, ..), where a function item that sets the variable (or amends it) then
//frees nothing the amend uses, and the variable is as where ixit reads the item first (h); where the walk met a symbol
//(ixwk's 512) that can name the variable, whose amend by name is refused (d8, by ixgi) but for f (h): the amend fails,
//and the variable is kept; and from the first level (a list of indices, an elided level: h), as d4 did it before. o:
//its own reference, so that where an amend by name of it is made (h) and fails, which leaves the global ::, it is put back
Z NI __attribute__((cold)) I ixfh(A*p,A x,A y,A z,UC d,B h){A o=_R(x);x=ixag(p,_R(x),y,z,d,0,0,0,h);P(!x,I(*p==au,*p=o)E(mr(o))4)I(*p,mr(*p))*p=x;mr(o);return 0;}
//can d4, amending x at y from level k (of m), call a function -- an item that f calls (# _ @: the item, .: anything in
//it, e), here or in a global that a symbol on the way names -- or amend p's global by name, through such symbols? Then
//p's global is held (ixfh), else d4 amends it in place, as before. Only the items at y are looked at (every one for an
//elided level, or an index that is not ints or keys). t: x is where an amend starts, whose symbol list's items name
//globals. Where unsure -- a table with a general or symbol column, a dict that is not keys!list, a function above the
//last level, a chain of more than 32 names, an item more than 64 levels deep (ixfd: . takes at most 8 index levels, so
//.: calls no function below that, and 1 only costs a copy) -- 1. The walk is O(the items at y; for .: all of each),
//where d4 may touch one path in each: to measure in the timing pass
A*gq(A);Z I ixfd(A x,I l){UC t=_t(x);P(_tU(x)||l>64,1)P(t==tA,({I r=0;F(_N(x),I(ixfd(_A(x)[i],l+1),r=1;break))r;}))return (t==tm||t==tM)&&(ixfd(_x(x),l+1)||ixfd(_y(x),l+1));}
Z I ixhr(A*p,A x,A y,U k,U m,B f,B e,B t,I l){if(k==m)return f&&(e?ixfd(x,0):_tU(x));UC vt=_t(x);
 if(vt==ts){A*q=gq(x);return q==p||q&&*q&&(l>32||ixhr(p,*q,y,k,m,f,e,1,l+1));}   //a name: the amend goes on in its global
 if(!f&&k+1==m||_tP(x)&&!_tU(x))return 0;if(_tU(x)||vt==tM&&(f||k+2<m)&&({I r=0;A c=_y(x);F(_N(c),I(_t(_A(c)[i])==tA||_t(_A(c)[i])==tS,r=1))r;}))return 1;
 B dc=vt==tm;A v=dc?_y(x):x;P(vt>tm||vt==tM,0)P(dc&&(!_tT(_x(x))||!_tT(v)),1)UC u=_t(v);P(u!=tA&&!(u==tS&&(t||dc)),0)   //items that are data
 UC ry=_t(y);U n=_N(v);L j=0;A s=0,g=0;B a=0;I r=0;   //the items at y[k]: every one (a), g's (an int list), or j
 if(ry>tm)s=y;else if(ry==tA)s=_A(y)[k];else if(LH(tE,ry,tL))j=ixe(y,k);else if(ry==tS)s=ii(y,k);else a=1;
 if(!a&&(s==au||s&&_tA(s)))a=1;else if(!a&&dc)I(_t(_x(x))==tS&&s&&(_ts(s)||_t(s)==tS),g=fnd(_x(x),_R(s));I(!_tT(g),j=gl_(g);mr(g);g=0))E(a=1)
 else if(s&&_tz(s))j=gl_(s);else if(s)I(_tZ(s),g=_R(s))E(a=1);
 U c=a?n:g?_N(g):1;for(U i=0;i<c&&!r;i++){L q=a?(L)i:g?ixe(g,i):j;I(q>=0&&q<(L)n,r=ixhr(p,u==tA?_A(v)[q]:ii(v,(U)q),y,k+1,m,f,e,0,l))}
 I(g,mr(g))return r;}
Z I ixho(A*p,A x,A y,B f,UC d)_(ixhr(p,x,y,0,y==au?0:_t(y)>tm?1:_N(y),f,d==19,1,0))   //from the first level
Z NI I ixcv(A*p,A x,A y,A z,UC d,B g,UC*kd,L*ix,I m,I n){B f=d==14||d==15||d==18||d==19,o;A q=0;
 if(!n){if(m>0&&m<256){I(f||kd[m-1]==3,q=ixit(x,y,kd,ix,(U)m,&o))}   //a row with more below (where the walk stopped, or did not start):
  else if(m?m>0&&kd[(m&255)-1]==3:_t(x)==tM){U w=m&255;q=ixit(x,y,kd,ix,w,&o);I(q,m=w?(I)w:1)}}   //no symbol is met, so no global is amended by name
 if(q){B h=f&&g;I(h,RC_INC(x))A r=ixv2(d,q,z);I(o,mr(q))   //f, or at a table's row: the verb first, on the item
  if(h&&*p!=x){P(!r,mr(x);4)I(*p,mr(*p))*p=x;}E(I(h,(V)RC_DECV(x))P(!r,4))   //f set the variable: if f failed, it keeps that; else
  B rw=kd[m-1]==3;A u=0;A w=ixst(x,y,kd,ix,0,(U)m,rw?av:0,r,rw?0:&u);I(rw,mr(r))P(!w&&u,*p=u;4)*p=w;return 0;}   //the assignment is made on x, as before (held). ixst takes r,
 //but for a row (d8); a result that does not fit (a column's count) is its error, and x is put back (u)
 I(f&&!n&&m>0&&kd[(m&255)-1]==9,mr((A)ix[(m&255)-1]))   //no item read: the find ixwk made for a list of keys (9) is not passed to ixst
 P(m<0,-m)I(f&&!n,m=_t(x)==tm&&!_tMT(_y(x))?-3:-ixck(x,y,0,z,!d))P(m<0,-m)   //ixwk's or ixck's error, and *p untouched;
 P(g&&!m&&ixho(p,x,y,f,d),ixfh(p,x,y,z,d,1))P(m>255||g&&!m,*p=ixag(p,x,y,z,d,kd,ix,m&255,0);0)A u=0;x=ixas(x,y,z,d,kd,ix,m,&u);P(!x&&u,*p=u;4)*p=x;return 0;}   //else 0; 4: the verb's error, and *p as it was (or as f set it)
Z NI I ixca(A*p,A x,A y,A z,UC d,B g){UC kd[8];L ix[8];I n=g&&ixgs?ixgn(p):0;P(n>1,3)
 I m=n?_t(x)==tm&&!_tMT(_y(x))?-3:-ixck(x,y,0,z,!d):ixwk(x,y,z,!d,g,kd,ix);B f=d==14||d==15||d==18||d==19;I(m==512,P(ixho(p,x,y,f,d),ixfh(p,x,y,z,d,f))m=0)P(d,ixcv(p,x,y,z,d,g,kd,ix,m,n))P(m<0,-m)   //a verb, which can fail: ixcv
 P(g&&!m&&ixho(p,x,y,0,0),ixfh(p,x,y,z,0,1))*p=m>255||g&&!m?ixag(p,x,y,z,0,kd,ix,m&255,0):ixas(x,y,z,0,kd,ix,m,0);return 0;}   //: assigns z, which ixwk checked fits
Z NI __attribute__((cold)) V noupd(A*s){mr(*s);*s=err0("noupdate");}
//g f:y, f a global that # _ @ or . calls (bM), which can set g: g held while f runs, as for g[i]f:y (ixfh); 0: f's error
Z NI __attribute__((cold)) I bmh(A*p,A x,A y,UC d){y=v2[d](_R(x),y);mr(x);P(!y,0)I(*p,mr(*p))*p=y;return 1;}
Z NI __attribute__((cold)) A ixer(I e)_(e==1?ei0():e==2?el0():e==3?et0():0)   //ixca's error (4: the verb's, already set)
A fzop(CO UC*,A*,A*);
// Amber 2.5 (exp): the VM's start pinned to 64 bytes. Its dispatch loop's speed depended on where the linker happened
// to put it: a change anywhere else (one constant in src/2.c, here) moved run() 16 bytes and made nbody 7% slower, and
// that alignment luck is the 5-8% nbody swing seen between builds since 2.3. Pinned, unrelated changes no longer move it.
A run(A,CO A*,U)__attribute__((aligned(64)));
AX(run,Q(xto)Z AM_TLS_IE I d;P(++d>2048,d--,es8(a,n))/*d: per-thread VM recursion depth (peach workers run the VM concurrently)*/P(n-xk,d--,er8(a,n))UC*b=_V(xy),c,nl=_n(xA[3]);A own=0,l[nl+*b++],*s=l+L(l);MS(l,0,SZ l);I(n,MC(l,a,8*n))//virtual machine
 W((c=*b++),S(c,                                                                                    //          |BYTES |          STACK        |         EFFECT
  C16(bu,U(*s=v1[c-bu](*s)))                                                                        //Monad     |bu+m  |.. x -> monads[m][x]   |
  case bu+16:case bu+17:case bu+18:case bu+19:case bu+20:case bu+21:case bu+22:case bu+23:
  case bu+24:case bu+25:case bu+26:case bu+27:case bu+28:case bu+29:case bu+30:{U(*s=v1[c-bu](*s))}break;
  C(bF,UC j=*b;A r=fzop(b,l,xA+OFF);b+=3;I(r,*--s=r;b+=j))                                          //Fused     |bF,j,c,p|.. -> .. r       |when fzop takes it: PC+:j
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
   I(c==bm||c==bM,UC d_=*b++;I(c==bM&&(d_-14<2u||d_-18<2u),U(bmh(p,x,y,d_),*--s=0))E(y=v2[d_](x,y);U(y,*--s=0)*p=x(y)))   //mod asgn  |bm,i,d|.. x -> ..             |vars[i]:dyads[d][vars[i];x]
   E(UC d_=*b++;I e_=ixca(p,x,y,*s,d_,c&1);x=e_?ixer(e_):*p;mr(*s);I(c==bx||c==bX,mr(y);U(x,*s=0)s++)        //ind asgn  |bx,i,d|.. z y -> ..           |vars[i]:  .[vars[i];y;dyads[d];z]
                                    E(U(x,*s=y(0))U(*s=dot(x,y)))))                                 //ind asgn  |by,i,d|.. z y -> .. r         |vars[i]:r:.[vars[i];y;dyads[d];z]
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
Z B cid(A x,A y)_(P(!mtc_(x,y),0)UC t=_t(x);P(_t(y)!=t,0)   //And of one type: a bit list was pooled with the equal int list (digest #36)
P(t==tf||t==tF,_t(y)==t&&!memcmp(_V(x),_V(y),(N)(t==tf?1:_n(x))*SZ(F)))
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
// ---- amber 2.5 (exp): fusion. An element-wise tree of + - * % & | (< > = at the root) over variables and number literals
// (two operations or more, or one under +/) compiles to  bF j c p  followed by the usual code for it. bF gives
// the program (constant c: its bytes, then the literals) to fzrun (src/2.c), which takes it only for float
// vectors and numbers and then jumps the j bytes of usual code; everything else falls through to that code.
// p is the rightmost local leaf: unless it holds a float vector bF does not even call fzrun, so scalar code
// pays one test. If the tree's usual code is longer than a jump reaches, the bF is taken back out.
Z B fzno,fzu;   //fzno: emit no fused code (the fallback being compiled, or the recompile after a limit); fzu: emitted some
// Is t a fusable tree? k counts operations, v variable leaves; r: t is the root, where < > = may stand too
Z B fzt(A t,I*k,I*v,B r){I(_tS(t),P(_n(t)!=1,0)++*v;return 1)P(numlit(t),1)P(!_tA(t)||_n(t)!=3,0)A d=_A(t)[0];
 P(d!=ADD&&d!=SUB&&d!=MUL&&d!=DVD&&d!=MNM&&d!=MXM&&!(r&&(d==LTN||d==GTN||d==EQL)),0)P(++*k>15,0)return fzt(_A(t)[1],k,v,0)&&fzt(_A(t)[2],k,v,0);}
Z B fzp(A t,UC*p,I*np,A*d,I*pk){P(*np>60,0)
 I(_tS(t),I w=*_I(t),i=fI(l,nl,w);P(i<nl,p[(*np)++]='l';p[(*np)++]=i;*pk=i;1)P(w=='o',0)U g=gi(t);
  p[(*np)++]='g';p[(*np)++]=g&255;p[(*np)++]=g>>8;return 1)
 I(numlit(t),P(_n(*d)>250,0)p[(*np)++]='k';p[(*np)++]=_n(*d);PSH(*d,_R(t));return 1)
 P(!fzp(_A(t)[1],p,np,d,pk)||!fzp(_A(t)[2],p,np,d,pk),0)A o=_A(t)[0];p[(*np)++]=o==ADD?'+':o==SUB?'-':o==MUL?'*':o==DVD?'%':o==MNM?'&':o==MXM?'|':o==LTN?'<':o==GTN?'>':'=';return 1;}
Z I fzc(A x)_(U n=un,i=OFF;W(i<n&&!cid(x,ua),i++)I(i>=n,PSH(u,xR))i-OFF)
Z I fz(A x,B r){P(fzno||!r,-2)U n=xn;A y=xx,t=x;I o=xo;UC fl=0;
 I(n==2&&((_tA(y)&&_n(y)==2&&_A(y)[0]==aw+1&&_A(y)[1]==ADD)||(!_tP(y)&&_t(y)==tr&&_E(y)==1&&_n(y)==1&&_A(y)[0]==ADD)),t=xy;fl=1)
 I k=0,v=0;P(!fzt(t,&k,&v,1)||!v||k<2-fl,-2)
 UC p[72];I np=1,pk=255;p[0]=fl;A d=aA(1);_A(d)[0]=au;P(!fzp(t,p,&np,&d,&pk),mr(d);-2)
 _A(d)[0]=aCn((S)p,np);I c=fzc(d);mr(d);P(c>255,-2)
 I st=nb;M(bF)M(0)M(c)M(pk)fzno=1;I r_=cr(x,1);fzno=0;P(r_-OK,r_)
 I len=nb-st-4;I(len>255,__builtin_memmove(b+st,b+st+4,len);__builtin_memmove(m+st,m+st+4,len);nb-=4;F(16,I(lu[i]>=st+4,lu[i]-=4))return OK)
 b[st+1]=len;fzu=1;return OK;}
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
 {I f_=fz(x,r);P(f_!=-2,f_)}                                                                         // Amber 2.5 (exp) float fusion
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
// Amber 2.5 (exp): one compile of the body: OK, a cr() error offset, -2 a size limit, -3 the global table full
Z I cpl1(A y,B s,I k,CO I*l0){nb=1;MS(lu,-1,SZ lu);MC(l,l0,OFF*k);nl=k;fzu=0;I r=cr(y,!s);P(r-OK,r)P(gfull,-3)
 I o=0;I(s,cc(au,o))P(un>255||nb>MB-2||nl>L(l)-2,-2)M(bu)P(nb>MB-2||un>255-bc+OFF,-2)
 F(nl,I j=lu[i];I(j>=0&&b[j]==bg,b[j]=bd))I sx=mxs(1,0);P(sx>255,-2)*b=sx;*m=-1;return OK;}
// Compiled with float fusion first; if that hits a limit of the bytecode and fused code was emitted, compiled
// again without it, so fusion never turns a lambda that compiled before into a 'limit
Z A3(cpl_,/*111*/I k=0,l0[16];I(z,k=zn;MC(l0,zV,OFF*k);z(0))y=Nx(cf(y));B s=shy(y);fzno=0;u=aA(OFF);ux=xR;uy=uz=uA[3]=au;I r=cpl1(y,s,k,l0);
 I(r!=OK&&fzu&&(r==-2||lim),mr(u);lim=0;fzno=1;u=aA(OFF);ux=xR;uy=uz=uA[3]=au;r=cpl1(y,s,k,l0);fzno=0)
 y(0);mr(x);P(r==-2,ez0();eS(ux,0);u(0))P(r!=OK,gfull||lim?(gfull=lim=0,ez0()):ec0();eS(ux,r>=0?r:0);u(0))
 uy=aCn(b,nb);uz=aCn(m,nb);uA[3]=aV(tS,nl,l);AK(k,AT(to,u)))
A3(cpl,P(!ray_rc_sync,cpl_(x,y,z))plk(1);A r=cpl_(x,y,z);plk(0);r)                                 //cpl_ under the peach parse lock (m.c plk)
#undef M
