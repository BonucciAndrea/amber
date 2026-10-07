/* pread, strdup, struct stat and dirent are POSIX, hidden under a strict -std=c99 unless asked for before the
 * first system header (CI builds every file that way; clang makes the implicit declarations errors). */
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
#include"a.h" // Amber - GNU AGPLv3 - see LICENSE and NOTICE
/* dsk.c  -  on-disk columns (2.7): `wcol writes one, `rcol maps one back.
 *
 * A column file is one 4096-byte header page and then the data, so the data starts on a page boundary
 * and a flat column maps straight into memory with no copy (mfw in m.c puts Amber's header in the page
 * in front of it). A 10M-row float column loads in the time of one mmap; pages come in as they are read.
 *
 * HEADER (little-endian, the rest of the page is zero)
 *   0  "AMBC"      magic
 *   4  u8 version  1
 *   5  u8 kind     0 flat: the raw payload of a B G H I L F C vector
 *                  1 enum: int32 indexes into the database's sym file (a symbol column)
 *                  2 ser:  any other value as -8! bytes (nested lists, dicts, atoms, ...)
 *                  3 time: a list of date, time or timestamp atoms as int64 values
 *   6  u8 type     Amber type of the vector (kind 0), or of the atoms (kind 3)
 *   7  u8 attr     0 none, 1 `s, 2 `u, 3 `p, 4 `g  (trusted on the way back, as q does)
 *   8  u64 count   items
 *   16 u64 bytes   payload bytes after the header
 *   24 u64 domain  kind 1: how many syms the sym file had when this was written
 *
 * Files are written to name.tmp and renamed over the old one, so a reader that has the old file mapped
 * keeps its old data and never sees half a file. A map is private: an in-place write in Amber copies
 * the page, it never reaches the disk.
 */
#if !defined(wasm)
#include<fcntl.h>
#include<errno.h>
#include<stdlib.h>
#include<sys/stat.h>
#include<stdio.h>
A mfw(I,W,W);I tjk(A);A tjn(A);

#define DHB 4096u
enum{KFLAT,KENUM,KSER,KTIME};
typedef struct{C mg[4];UC ver,kind,typ,att;W cnt,nb,dom;}DHdr;   //32 bytes, then zeros up to DHB

Z B dflat(UC t){return t==tB||t==tG||t==tH||t==tI||t==tL||t==tF||t==tC;}
Z W dbytes(UC t,W n){return ((n<<Tw[t])+7)>>3;}
// a path out of a char vector, NUL-terminated; 0 when it does not fit
Z B dpath(A p,C*b,N cap){P(_t(p)!=tC||!_n(p)||_n(p)>=cap,0)MC(b,_V(p),_n(p));b[_n(p)]=0;return 1;}
// mkdir -p of every directory above the file
Z I dmkp(C*p){for(C*s=p+1;*s;s++)if(*s=='/'){*s=0;I r=mkdir(p,0777);*s='/';if(r&&errno!=EEXIST)return -1;}return 0;}
Z I dwall(I f,CO V*b,W n){CO C*p=b;while(n){W k0=n>((W)1<<30)?((W)1<<30):n;ssize_t k=write(f,p,(N)k0);if(k<0&&errno==EINTR)continue;if(k<=0)return -1;p+=k;n-=(W)k;}return 0;}
Z I drall(I f,V*b,W n,W off){C*p=b;while(n){W k0=n>((W)1<<30)?((W)1<<30):n;ssize_t k=pread(f,p,(N)k0,(off_t)off);if(k<0&&errno==EINTR)continue;if(k<=0)return -1;p+=k;n-=(W)k;off+=(W)k;}return 0;}

// `wcol(path;value;domain;attr): write one column file. domain -1 picks the kind from the value; domain>=0
// says the value is an int vector of indexes into a sym file of that many names (kind 1), and attr is the
// symbol column's attribute. Returns the path.
// `wcol of a list of such quadruples writes them all or none: each to its .tmp file, and only once every one is
// written are they renamed, in the list's order. A splayed table is written so (the sym file first, .d last), and a
// set that fails before the renames leaves the old table as it was (#94 Q12). Returns the paths. A path named twice is
// 'domain. A reader during the renames can still see some files old and some new.
// One quadruple x to its .tmp file: 1 when written; 0 when not, with the error raised and no .tmp left of it.
Z B dwt(A x,C*path,C*tmp){
 P(_t(x)!=tA||_n(x)!=4,et0(),0)
 A*a=_A(x);A pth=a[0],v=a[1];L dn=gl_(a[2]),atr=gl_(a[3]);
 P(!dpath(pth,path,4096-8),et0(),0)
 UC t=_t(v),kind,typ=t,att=0;W cnt,nb;CO V*src;A body=0;
 if(dn>=0){P(_tP(v)||!(t==tG||t==tH||t==tI||t==tL),et0(),0)
  if(t!=tI){body=cI(_R(v));P(!body,0)v=body;}   //find gives the narrowest int that holds the indexes
  kind=KENUM;typ=tI;cnt=_n(v);nb=cnt*4;src=_V(v);att=(UC)(atr>=0&&atr<5?atr:0);}
 else if(!_tP(v)&&dflat(t)){kind=KFLAT;cnt=_n(v);nb=dbytes(t,cnt);src=_V(v);att=_at(v);}
 else{I k=_tP(v)?0:tjk(v);
  if(k==tdt||k==ttm||k==tnp){kind=KTIME;typ=(UC)k;body=tjn(v);cnt=_n(body);nb=cnt*8;src=_V(body);}
  else{kind=KSER;typ=0;body=ser8(_R(v));P(!body,0)cnt=_n(body);nb=cnt;src=_V(body);}}
 C hdr[DHB];MS(hdr,0,DHB);DHdr*h=(DHdr*)hdr;MC(h->mg,"AMBC",4);h->ver=1;h->kind=kind;h->typ=typ;h->att=att;h->cnt=cnt;h->nb=nb;h->dom=dn>=0?(W)dn:0;
 snprintf(tmp,4100,"%s.tmp",path);
 I f=open(tmp,O_WRONLY|O_CREAT|O_TRUNC,0644);
 if(f<0&&errno==ENOENT&&!dmkp(tmp))f=open(tmp,O_WRONLY|O_CREAT|O_TRUNC,0644);
 if(f<0){if(body)mr(body);eo0();return 0;}
 I bad=dwall(f,hdr,DHB)||(nb&&dwall(f,src,nb));bad|=close(f);
 if(body)mr(body);
 if(bad){unlink(tmp);eo0();return 0;}
 return 1;}
// the names of quadruple x, written already, and the unlink of .tmp files i to j-1 of a list
Z V dwnm(A x,C*path,C*tmp){dpath(_A(x)[0],path,4096-8);snprintf(tmp,4100,"%s.tmp",path);}
Z V dwun(A x,U i,U j,C*path,C*tmp){for(;i<j;i++){dwnm(_A(x)[i],path,tmp);unlink(tmp);}}
// two quadruples naming one path would share its .tmp file, and one of them would be lost: 'domain before any write
Z B dwdup(A x){U n=_n(x);F(n,A a=_A(x)[i];P(_t(a)!=tA||_n(a)!=4,0)A p=*_A(a);P(_t(p)!=tC,0)
 for(U j=0;j<i;j++){A q=*_A(_A(x)[j]);if(_n(q)==_n(p)&&!memcmp(_V(q),_V(p),_n(p)))return 1;})return 0;}
Z A wcolN(A x){U n=_n(x);C path[4096],tmp[4100];P(dwdup(x),x(ed0()))
 for(U i=0;i<n;i++)if(!dwt(_A(x)[i],path,tmp)){dwun(x,0,i,path,tmp);return x(0);}
 for(U i=0;i<n;i++){dwnm(_A(x)[i],path,tmp);if(rename(tmp,path)){dwun(x,i,n,path,tmp);return x(eo0());}}
 A r=aA(n);F(n,_A(r)[i]=_R(*_A(_A(x)[i])))mr(x);return r;}
A wcolT(A x){
 P(_t(x)==tA&&_n(x)&&_t(*_A(x))==tA,wcolN(x))
 C path[4096],tmp[4100];P(!dwt(x,path,tmp),x(0))
 if(rename(tmp,path)){unlink(tmp);return x(eo0());}
 A r=_R(*_A(x));mr(x);return r;}

// the payload of a column file as a vector of type t and count n: mapped when it can be, read otherwise
Z A dload(I f,UC t,W n,W nb){
 P(!n,an(0,t))
 A v=mfw(f,DHB,nb);
 if(v){_T(v)=t;_n(v)=(U)n;_at(v)=0;return v;}
 v=an((U)n,t);P(drall(f,_V(v),nb,DHB),mr(v);eo0())return v;}

// `rcol path or `rcol(path;syms): one column file back. A symbol column needs the database's syms (a sym
// vector); without them its indexes come back as ints.
A rcolT(A x){
 A pth=x,dom=0;
 if(_t(x)==tA&&_n(x)==2){pth=_A(x)[0];dom=_A(x)[1];P(_t(dom)!=tS,x(et0()))}
 C path[4096];P(!dpath(pth,path,SZ path),x(et0()))
 I f=open(path,O_RDONLY);P(f<0,x(eo0()))
 struct stat st;DHdr h;I e=fstat(f,&st);
 // a file that opens but is too short for the header is damage, 'format (#94 Q14); 'io is a failed open, stat or
 // read, and a directory
 if(e||(W)st.st_size<DHB||drall(f,&h,SZ h,0)){close(f);return x(!e&&S_ISREG(st.st_mode)&&(W)st.st_size<DHB?err0("format"):eo0());}
 B ok=!memcmp(h.mg,"AMBC",4)&&h.ver==1&&h.kind<=KTIME&&(W)st.st_size>=DHB+h.nb;
 if(ok)S(h.kind,
  C(KFLAT,ok=h.typ<tn&&dflat(h.typ)&&h.nb==dbytes(h.typ,h.cnt))
  C(KENUM,ok=h.nb==h.cnt*4)
  C(KSER, ok=h.nb==h.cnt)
  C(KTIME,ok=(h.typ==tdt||h.typ==ttm||h.typ==tnp)&&h.nb==h.cnt*8))
 if(!ok){close(f);return x(err0("format"));}
 if(h.cnt>0xffffffffull){close(f);return x(ez0());}
 A r=0;U n=(U)h.cnt;
 S(h.kind,
  C(KFLAT,r=dload(f,h.typ,h.cnt,h.nb);if(r)_at(r)=h.att)
  C(KENUM,A iv=dload(f,tI,h.cnt,h.nb);
    if(!iv||!dom){r=iv;break;}
    U dn=_n(dom);CO I*ix=(CO I*)_V(iv);CO I*sy=(CO I*)_V(dom);
    // the sym file only grows, so every index the column was written with is below its count then
    if(h.dom>dn){mr(iv);r=err0("sym");break;}
    r=an(n,tS);I*o=(I*)_V(r);
    for(U i=0;i<n;i++){U k=(U)ix[i];if(k>=dn){mr(iv);mr(r);r=err0("sym");goto out;}o[i]=sy[k];}
    _at(r)=h.att;mr(iv))
  C(KSER,A b=dload(f,tC,h.cnt,h.nb);r=b?des9(b):0)
  C(KTIME,A b=dload(f,tL,h.cnt,h.nb);if(!b)break;CO L*q=(CO L*)_V(b);
    r=aA(n);A*o=_A(r);
    if(h.typ==tnp){for(U i=0;i<n;i++)o[i]=antp(q[i]);}
    else{for(U i=0;i<n;i++)o[i]=Lt(h.typ)|(U)(I)q[i];}
    mr(b)))
 out:
 close(f);mr(x);return r;}

// `fsz path: the size of a file, -1 when there is nothing there, -2 for a directory. Other failures are
// 'io: hdb.k must never take a sym file it could not read for a missing one and write a new one over it.
A fszT(A x){C path[4096];P(!dpath(x,path,SZ path),x(et0()))struct stat st;
 if(stat(path,&st))return x(errno==ENOENT||errno==ENOTDIR?al(-1):eo0());
 return x(al(S_ISDIR(st.st_mode)?-2:(L)st.st_size));}

// `ldir path: (directories;files) in a directory, each a list of names in byte order, . and .. left out
#include<dirent.h>
Z I dcmp(CO V*a,CO V*b){return strcmp(*(C*CO*)a,*(C*CO*)b);}
A ldirT(A x){C path[4096];P(!dpath(x,path,SZ path-260),x(et0()))DIR*d=opendir(path);P(!d,x(eo0()))
 N cap=64,n=0;C**nm=malloc(cap*SZ(C*));B ok=!!nm;struct dirent*e;
 while(ok&&(e=readdir(d))){if(!strcmp(e->d_name,".")||!strcmp(e->d_name,".."))continue;
  if(n==cap){C**q=realloc(nm,2*cap*SZ(C*));if(!q){ok=0;break;}nm=q;cap*=2;}
  nm[n]=strdup(e->d_name);if(!nm[n]){ok=0;break;}n++;}
 closedir(d);
 if(!ok){for(N i=0;i<n;i++)free(nm[i]);free(nm);return x(ez0());}
 qsort(nm,n,SZ(C*),dcmp);
 A ds=aA0(0),fs=aA0(0);N pl=strlen(path);path[pl++]='/';
 for(N i=0;i<n;i++){struct stat st;snprintf(path+pl,SZ path-pl,"%s",nm[i]);
  B dir=!stat(path,&st)&&S_ISDIR(st.st_mode);A s=aCz(nm[i]);
  if(dir)ds=psh(ds,s);else fs=psh(fs,s);free(nm[i]);}
 free(nm);return x(aA2(ds,fs));}
#else
A wcolT(A x){return x(en0());}
A rcolT(A x){return x(en0());}
A fszT(A x){return x(en0());}
A ldirT(A x){return x(en0());}
#endif
