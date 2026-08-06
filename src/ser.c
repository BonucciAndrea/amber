#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
/* ser.c  -  Amber compact binary serializer:  -8!x (encode)  -9!y (decode).
 *
 * WHY THIS EXISTS.  Amber's only wire format was TEXT: `k rendered the value
 * and `. reparsed it.  peach shipped every worker's result that way, so a
 * multi-core map paid a full format-then-parse round trip per chunk, and any
 * value whose printed form does not reparse to itself (attributes, 0n/0w edge
 * cases, nested empties) could not survive the trip at all.  -8!/-9! is a
 * byte-exact, allocation-light encoding: one contiguous tC vector out, the
 * identical object back.
 *
 * FORMAT.  4-byte header "AMB" + version, then one recursive node:
 *
 *   u8 tag                        the K type enum value (_t(x))
 *   PACKED types (TP(t): ti tc tu tv tw tx tdt ttm) -- these live INSIDE the
 *     A word itself, not on the heap, so there is no header and no payload:
 *       i32 value                 (_v(x))
 *   ts (symbol atom)              u32 len + name bytes  -- see SYMBOLS below
 *   tA / tM / tm (ref-carrying)   u8 attr + u64 count + count recursive nodes
 *   tS (symbol vector)            u8 attr + u64 count + count (u32 len + name)
 *   everything else on the heap   u8 attr + u64 count + ((count<<Tw[t])+7>>3)
 *     (tE tB tG tH tI tL tF tC tl tf tnp)   raw payload bytes
 *
 * Because the trailing case copies the raw payload and stores Tw[t] implicitly
 * via the tag, bit vectors (tB, 1 bit/element) and the date/time widths come
 * out exact with no special cases, and nulls and infinities are preserved
 * because they ARE just their bit patterns (0N is 1<<63, 0w is the f64
 * infinity) -- nothing is routed through a decimal formatter that could round.
 *
 * ATTRIBUTES.  _at(x) (0=none, 1=`s#-sorted) rides in the attr byte for every
 * heap type, so a sorted column stays sorted across the wire and keeps the
 * O(log n) binary-search path in fnd() on the far side.
 *
 * SYMBOLS.  Symbol *ids* are process-local: su() resolves an id against this
 * process's intern table, and a forked peach worker can intern a symbol the
 * parent has never seen.  Shipping raw 32-bit ids would therefore decode to
 * the wrong name (or garbage) in the parent.  Every symbol is written as its
 * NAME and re-interned with us() on the way in, which is correct across
 * processes and across separate runs.
 *
 * NOT SUPPORTED: to/tp/tq/tr (lambdas, projections, compositions, derived
 * verbs).  Serialising a closure means serialising its captured environment
 * and its bytecode; that is a much larger feature than a data wire format and
 * nothing in peach needs it.  These raise a clean 'type rather than emitting
 * bytes that would not decode.
 *
 * SAFETY.  -9! is a parser for untrusted bytes, so every read is bounds
 * checked against the input length, the recursion is depth limited, and a
 * malformed or truncated buffer yields a clean 'domain error -- never a read
 * past the end and never a partially built object left un-freed.
 */
V *realloc(V*,N);
V free(V*);

#define SERMAGIC0 'A'
#define SERMAGIC1 'M'
#define SERMAGIC2 'B'
#define SERVER    1
#define SERDEPTH  256   /* recursion guard for both directions */

/* ---------- growable output buffer ---------------------------------------- */
typedef struct{UC*p;N n,cap;I bad;}WB;
Z V wput(WB*b,CO V*s,N k){
 I(b->bad,return)
 I(b->n+k>b->cap,N c=b->cap?b->cap:256;W(c<b->n+k,c+=c)UC*q=realloc(b->p,c);I(!q,b->bad=1;return)b->p=q;b->cap=c)
 MC(b->p+b->n,s,k);b->n+=k;}
Z V wu8 (WB*b,UC v)_(wput(b,&v,1))
Z V wu32(WB*b,U  v)_(wput(b,&v,4))
Z V wu64(WB*b,W  v)_(wput(b,&v,8))
Z V wsym(WB*b,U id){S s=su(id);U n=(U)SL(s);wu32(b,n);wput(b,s,n);}

Z V enc(WB*b,A x,I d);
Z V encv(WB*b,A x,I d){
 C t=_t(x);U n=_n(x);
 wu8(b,(UC)_at(x));wu64(b,n);
 /* An EMPTY general list still carries a type witness in slot 0 (see aA0 in
  * m.c: `()` is allocated with room for one element, count forced to 0, and
  * slot 0 set to an empty tC). mtc_ compares that witness -- it loops
  * `xn|!xn` times, i.e. at least once -- so `(-9!-8!())~()` is false unless
  * the witness travels too. Encode max(n,1) children for every ref-carrying
  * type, which is exactly the count mtc_ will look at. */
 I(t==tA||t==tM||t==tm,CO A*e=_A(x);U m=n|!n;F(m,enc(b,e[i],d+1))return)
 I(t==tS,CO I*e=(CO I*)_V(x);F(n,wsym(b,(U)e[i]))return)
 wput(b,_V(x),(N)((((W)n<<Tw[(I)t])+7)>>3));}
Z V enc(WB*b,A x,I d){
 I(b->bad,return)
 I(d>SERDEPTH,b->bad=2;return)
 C t=_t(x);
 I(t==to||t==tp||t==tq||t==tr,b->bad=2;return)   /* functions: unsupported */
 wu8(b,(UC)t);
 I(t==ts,wsym(b,(U)_v(x));return)
 I(TP(t),wu32(b,(U)_v(x));return)
 encv(b,x,d);}

A ser8(A x/*1*/){
 WB b={0,0,0,0};
 UC h[4]={SERMAGIC0,SERMAGIC1,SERMAGIC2,SERVER};
 wput(&b,h,4);enc(&b,x,0);
 I(b.bad,I(b.p,free(b.p))mr(x);return b.bad==2?et0():eo0())
 A r=aV(tC,(U)b.n,b.p);
 free(b.p);mr(x);return r;}

/* ---------- bounds-checked input ------------------------------------------ */
typedef struct{CO UC*p;N n,i;I bad;}RB;
Z CO V*rtake(RB*b,N k){I(b->bad||k>b->n-b->i,b->bad=1;return 0)CO UC*q=b->p+b->i;b->i+=k;return q;}
Z UC ru8 (RB*b){CO V*q=rtake(b,1);I(!q,return 0)UC v;MC(&v,q,1);return v;}
Z U  ru32(RB*b){CO V*q=rtake(b,4);I(!q,return 0)U  v;MC(&v,q,4);return v;}
Z W  ru64(RB*b){CO V*q=rtake(b,8);I(!q,return 0)W  v;MC(&v,q,8);return v;}
Z U  rsym(RB*b){U n=ru32(b);I(b->bad,return 0)CO V*q=rtake(b,n);I(!q,return 0)
 C t[256];I(n>=SZ t,b->bad=1;return 0)MC(t,q,n);t[n]=0;return us(t);}

Z A dec(RB*b,I d);
Z A dec(RB*b,I d){
 I(b->bad||d>SERDEPTH,b->bad=1;return 0)
 UC t=ru8(b);I(b->bad,return 0)
 I(t<tA||t>=tn,b->bad=1;return 0)
 I(t==to||t==tp||t==tq||t==tr,b->bad=1;return 0)
 I(t==ts,U id=rsym(b);I(b->bad,return 0)return as(id))
 I(TP(t),U v=ru32(b);I(b->bad,return 0)return Lt(t)|(U)(I)v)
 UC at=ru8(b);W n=ru64(b);I(b->bad,return 0)
 /* a count that cannot fit the remaining bytes is corrupt: reject BEFORE
  * allocating, so a hostile length field cannot drive a huge allocation. */
 I(n>(W)0xffffffffu,b->bad=1;return 0)
 I(t==tA||t==tM||t==tm,
   W m=n|!n;                                      /* slot 0 witness, see enc */
   I(m>(W)(b->n-b->i),b->bad=1;return 0)          /* >=1 byte per child */
   A x=an((U)m,t);A*e=(A*)_V(x);W k=0;
   W(k<m,A c=dec(b,d+1);I(!c,mrn((U)k,e);AN(0,x);mr(x);b->bad=1;return 0)e[k++]=c)
   AN((U)n,x);_at(x)=at;return x)
 I(t==tS,
   I(n>(W)(b->n-b->i),b->bad=1;return 0)          /* >=4 bytes per symbol */
   A x=an((U)n,tS);I*e=(I*)_V(x);
   F((U)n,U id=rsym(b);I(b->bad,mr(x);return 0)e[i]=(I)id)
   _at(x)=at;return x)
 N nb=(N)((((W)n<<Tw[t])+7)>>3);
 CO V*q=rtake(b,nb);I(!q,return 0)
 A x=an((U)n,t);MC(_V(x),q,nb);_at(x)=at;return x;}

A des9(A y/*1*/){
 /* NOTE: I(c,a) is `if(c){a;}` -- it does NOT return. Every early exit here
  * needs an explicit `return`, or a rejected header falls through and
  * dereferences the NULL rtake() just handed back. */
 P(_t(y)-tC,mr(y);et0())
 RB b={_V(y),(N)_n(y),0,0};
 CO UC*hp=rtake(&b,4);
 I(!hp,mr(y);return ed0())
 I(hp[0]-SERMAGIC0||hp[1]-SERMAGIC1||hp[2]-SERMAGIC2||hp[3]-SERVER,mr(y);return ed0())
 A r=dec(&b,0);
 I(!r||b.bad||b.i!=b.n,I(r,mr(r))mr(y);return ed0())
 mr(y);return r;}
