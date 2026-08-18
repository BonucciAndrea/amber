/* ---- portability preamble: MUST precede every system header in this TU ----
 * strdup(3) is POSIX, not ISO C. Under `cc -std=c99` __STRICT_ANSI__ is defined,
 * glibc drops _DEFAULT_SOURCE, and <string.h> stops declaring it -- which is
 * precisely the "Compile every TU under strict -std=c99" CI failure:
 *   error: call to undeclared function 'strdup'; ISO C99 and later do not
 *   support implicit function declarations [-Wimplicit-function-declaration]
 * Under C99 that is not merely a diagnostic: the implicit `int strdup()` is
 * then assigned to a `const char *`, so the returned pointer is truncated to
 * 32 bits on LP64 -- a real bug, not just a warning.
 *
 * _POSIX_C_SOURCE >= 200809L is what actually exposes strdup. Defining it alone
 * would put Darwin's headers into STRICT POSIX mode and hide the BSD
 * extensions, so _DARWIN_C_SOURCE goes back in beside it; _GNU_SOURCE and
 * _DEFAULT_SOURCE do the equivalent job on glibc/musl. All four are purely
 * ADDITIVE -- they only ever unhide declarations -- and this is the same
 * preamble src/a.c, src/arena.c and src/trace.c already carry.
 *
 * These must sit above the FIRST #include of the translation unit, not merely
 * above <string.h>: any system header may pull in <features.h> first and latch
 * the mode for the whole compilation. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include<stdlib.h> // Amber - Apache Arrow C Data Interface - GNU AGPLv3 - see LICENSE and NOTICE
#include<string.h>
#include"a.h"
// Zero-dependency Arrow C Data Interface: interop with PyArrow/Polars/DuckDB over the
// stable C ABI without linking libarrow.  Export is zero-copy (child buffers alias Amber
// vector payloads; the release callback drops the Amber refcount).  Import copies (Amber's
// inline 32-byte object header precludes aliasing a foreign buffer as a native vector).

// ---- release callbacks ----------------------------------------------------
Z V relSchemaChild(struct ArrowSchema*s){if(!s->release)return;free((V*)s->name);s->release=0;}
Z V relSchema(struct ArrowSchema*s){if(!s->release)return;for(L i=0;i<s->n_children;i++)if(s->children[i]){if(s->children[i]->release)s->children[i]->release(s->children[i]);free(s->children[i]);}free(s->children);s->release=0;}
Z V relArrayChild(struct ArrowArray*a){if(!a->release)return;if(a->n_buffers==3){free((V*)a->buffers[1]);free((V*)a->buffers[2]);}free((V*)a->buffers);a->release=0;}//3 buffers = utf8 (malloc'd offsets+data)
// top-level array release: return each aliased Amber column via mr (refcount drop)
Z V relArray(struct ArrowArray*a){if(!a->release)return;for(L i=0;i<a->n_children;i++)if(a->children[i]){if(a->children[i]->release)a->children[i]->release(a->children[i]);free(a->children[i]);}A*h=a->private_data;if(h){for(L i=0;i<a->n_children;i++)mr(h[i]);free(h);}free((V*)a->buffers);free(a->children);a->release=0;}

// Amber vector type -> Arrow format code (numeric primitives, zero-copy layout)
Z S afmt(UC t){S(t,R(tB,"b")R(tG,"C")R(tH,"s")R(tI,"i")R(tL,"l")R(tF,"g")R_("l"))}

// arrow.export: x=(names; cols) -> (schemaAddr; arrayAddr) as 64-bit ints.
A arrowExport(A x){
 P(_t(x)-tA||_n(x)-2,et(x))
 A names=((A*)_V(x))[0],cols=((A*)_V(x))[1];
 L nc=_n(cols);A*colv=(A*)_V(cols);CO I*nmv=(CO I*)_V(names);L nrows=0;
 struct ArrowSchema*sc=calloc(1,sizeof*sc);
 struct ArrowArray*ar=calloc(1,sizeof*ar);
 sc->format="+s";sc->n_children=nc;sc->children=calloc(nc?nc:1,sizeof(V*));sc->release=relSchema;
 ar->n_children=nc;ar->n_buffers=1;ar->buffers=calloc(1,sizeof(V*));ar->children=calloc(nc?nc:1,sizeof(V*));ar->release=relArray;
 A*held=calloc(nc?nc:1,sizeof(A));ar->private_data=held;
 for(L i=0;i<nc;i++){A col=_t(colv[i])==tE?gZ(_R(colv[i])):_R(colv[i]);//materialise lazy ranges to a flat buffer
  L rn=(L)_n(col);if(!i)nrows=rn;
  struct ArrowSchema*cs=calloc(1,sizeof*cs);
  struct ArrowArray*ca=calloc(1,sizeof*ca);
  cs->name=strdup(su(nmv[i]));cs->release=relSchemaChild;ca->length=rn;ca->release=relArrayChild;
  if(_t(col)==tS){cs->format="u";CO I*ids=(CO I*)_V(col);L tot=0;F(rn,tot+=strlen(su(ids[i])))I*offs=malloc((rn+1)*4);C*dt=malloc(tot?tot:1);offs[0]=0;L pos=0;F(rn,S s=su(ids[i]);L l=strlen(s);MC(dt+pos,s,l);pos+=l;offs[i+1]=(I)pos)ca->n_buffers=3;ca->buffers=calloc(3,sizeof(V*));ca->buffers[0]=0;ca->buffers[1]=offs;ca->buffers[2]=dt;}//utf8
  else{cs->format=afmt(_t(col));ca->n_buffers=2;ca->buffers=calloc(2,sizeof(V*));ca->buffers[0]=0;ca->buffers[1]=_V(col);}//numeric zero-copy
  held[i]=col;sc->children[i]=cs;ar->children[i]=ca;}
 ar->length=nrows;
 L addr[2]={(L)sc,(L)ar};
 return x(aV(tL,2,addr));}

// build one Amber column from an Arrow child buffer (copy); apply validity bitmap as nulls
Z A mkcol(S fmt,CO V*data,CO UC*valid,L n){A c;C f=fmt?*fmt:'l';
 S(f,C('i',c=aI(n);I(data,MC(_V(c),data,4*n)))
     C('l',c=aL(n);I(data,MC(_V(c),data,8*n)))
     C('g',c=aF(n);I(data,MC(_V(c),data,8*n)))
     C('s',c=an(n,tH);I(data,MC(_V(c),data,2*n)))
     C('b',c=an(n,tG);I(data,MC(_V(c),data,n)))
     C('C',c=an(n,tG);I(data,MC(_V(c),data,n)))
     D(c=aL(n);I(data,MC(_V(c),data,8*n))))
 I(valid,F(n,I(!(valid[i>>3]>>(i&7)&1),S(f,C('i',((I*)_V(c))[i]=1<<31)C('l',((L*)_V(c))[i]=NL)C('g',((F*)_V(c))[i]=NF)D()))))
 return c;}

// arrow.import: x=(schemaAddr; arrayAddr) -> (names; cols).  Copies buffers, then releases the Arrow structs.
A arrowImport(A x){
 P(_n(x)-2,el(x))
 A lv=N(cL(_R(x)));L pa=((CO L*)_V(lv))[0],pb=((CO L*)_V(lv))[1];mr(lv);
 struct ArrowSchema*sc=(V*)pa;struct ArrowArray*ar=(V*)pb;
 L nc=sc->n_children,nrows=ar->length;
 A nms=aS(nc),cols=aA(nc);A*cv=(A*)_V(cols);I*nv=(I*)_V(nms);
 for(L i=0;i<nc;i++){struct ArrowSchema*cs=sc->children[i];struct ArrowArray*ca=ar->children[i];
  nv[i]=us(cs->name?cs->name:"");
  if(cs->format&&*cs->format=='u'){CO I*offs=(CO I*)ca->buffers[1];CO C*dt=(CO C*)ca->buffers[2];A s=aS(nrows);I*sv=(I*)_V(s);
   F(nrows,C tmp[256];L l=offs[i+1]-offs[i];I(l>255,l=255)MC(tmp,dt+offs[i],l);tmp[l]=0;sv[i]=us(tmp))cv[i]=s;}//utf8 -> symbols
  else{CO UC*valid=ca->n_buffers>1?(CO UC*)ca->buffers[0]:0;CO V*data=ca->n_buffers>1?ca->buffers[1]:0;cv[i]=mkcol((S)cs->format,data,valid,nrows);}}
 if(ar->release)ar->release(ar);
 if(sc->release)sc->release(sc);
 free(ar);free(sc);
 A r[2]={nms,cols};return x(aV(tA,2,r));}
