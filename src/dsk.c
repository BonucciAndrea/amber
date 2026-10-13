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
 *   5  u8 kind     0 flat: the raw payload of a B G H I L F C vector, or of a temporal one (2.9.0)
 *                  1 enum: int32 indexes into the database's sym file (a symbol column)
 *                  2 ser:  any other value as `ser bytes (nested lists, dicts, atoms, ...)
 *                  3 time: a list of date, time or timestamp atoms as int64 values (before 2.9.0; loads as a typed list)
 *   6  u8 type     Amber type of the vector (kind 0), or of the atoms (kind 3), numbered as before 2.9 (a.h TXC)
 *   7  u8 attr     0 none, 1 `s, 2 `u, 3 `p, 4 `g  (trusted on the way back, as q does; any other byte is 'format)
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

Z B dflat(UC t){return t==tB||t==tG||t==tH||t==tI||t==tL||t==tF||t==tC||TQV(t);}   //a temporal list too (2.9.0): its numbers, as they are
Z W dbytes(UC t,W n){return ((n<<Tw[t])+7)>>3;}
// a path out of a char vector, NUL-terminated; 0 when it does not fit
Z B dpath(A p,C*b,N cap){P(_t(p)!=tC||!_n(p)||_n(p)>=cap||pnul(p),0)MC(b,_V(p),_n(p));b[_n(p)]=0;return 1;}
Z A dpe(A p){return pnul(p)?ed0():et0();}   // why dpath refused p: a NUL in it is 'domain (#94 Q16), else 'type
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
/* 2.9.0: `wcq writes q's own files (what rcq reads), for a database kdb+ made, so q can still read it after Amber
 * wrote to it (an rdb's eod[] .Q.dpft into a q hdb): a flat vector behind fe 20 t 0 and 12 zeros; a symbol column as
 * an enum, int64 indexes after a page whose header names the domain sym; a symbol list (the sym file, .d) as ff 01 0b 0
 * and its names. q writes 0 for every count, and so does this. What q has no file for goes in Amber's own (dwt) */
Z CO UC qtA[20]={0,tB,0,0,tL,tL,tL,tL,tF,tF,tC,0,tNP,tMO,tDT,0,tSP,tMN,tSC,tTM};
Z CO UC qtW[20]={0,1,0,0,1,2,4,8,4,8,1,0,8,4,4,0,8,4,4,4};
Z B dqf;Z CO UC qtO[tSP+1]={[tB]=1,[tG]=7,[tH]=7,[tI]=7,[tL]=7,[tF]=9,[tC]=10,[tDT]=14,[tTM]=19,[tNP]=12,[tMN]=17,[tSC]=18,[tMO]=13,[tSP]=16};
Z I dopen(C*path,C*tmp){snprintf(tmp,4100,"%s.tmp",path);I f=open(tmp,O_WRONLY|O_CREAT|O_TRUNC,0644);
 if(f<0&&errno==ENOENT&&!dmkp(tmp))f=open(tmp,O_WRONLY|O_CREAT|O_TRUNC,0644);return f;}
Z I dwtq(A x,C*path,C*tmp){   //1 written, 0 failed (error raised), -1 not a kind q has: Amber's file then
 P(_t(x)!=tA||_n(x)!=4,-1)A*a=_A(x);A v=a[1];L dn=gl_(a[2]);P(_tP(v),-1)UC t=_t(v);W n=_n(v);
 P(!dpath(a[0],path,4096-8),dpe(a[0]),0)
 UC*b;W m,o;
 if(dn>=0){P(!(t==tG||t==tH||t==tI||t==tL),-1)o=4096;m=o+n*8;b=calloc(1,m?m:1);P(!b,ez0(),0)
  b[0]=0xfd;b[1]=0x20;MC(b+16,"sym",3);b[0xff0]=0xfd;b[0xff2]=20;L*d=(L*)(b+o);
  for(W i=0;i<n;i++)d[i]=t==tL?_L(v)[i]:t==tI?_I(v)[i]:t==tH?_H(v)[i]:_G(v)[i];}
 else if(t==tS){m=8;for(W i=0;i<n;i++)m+=SL(su((U)_I(v)[i]))+1;b=calloc(1,m);P(!b,ez0(),0)b[0]=0xff;b[1]=1;b[2]=11;o=8;
  for(W i=0;i<n;i++){S s=su((U)_I(v)[i]);N l=SL(s);MC(b+o,s,l);o+=l+1;}}
 else{P(t>tSP||!qtO[t],-1)UC q=qtO[t];W w=qtW[q];m=16+n*w;b=calloc(1,m);P(!b,ez0(),0)b[0]=0xfe;b[1]=0x20;b[2]=q;UC*d=b+16;
  if(t==tB)for(W i=0;i<n;i++)d[i]=(UC)(((UC*)_V(v))[i>>3]>>(i&7)&1);
  else if(q==7&&t!=tL){L*e=(L*)d;for(W i=0;i<n;i++)e[i]=t==tI?_I(v)[i]:t==tH?_H(v)[i]:_G(v)[i];}
  else MC(d,_V(v),(N)(n*w));}
 I f=dopen(path,tmp);if(f<0){free(b);eo0();return 0;}
 I bad=dwall(f,b,m);bad|=close(f);free(b);
 if(bad){unlink(tmp);eo0();return 0;}
 return 1;}
// One quadruple x to its .tmp file: 1 when written; 0 when not, with the error raised and no .tmp left of it.
Z B dwt(A x,C*path,C*tmp){
 if(dqf){I r=dwtq(x,path,tmp);if(r>=0)return (B)r;}
 P(_t(x)!=tA||_n(x)!=4,et0(),0)
 A*a=_A(x);A pth=a[0],v=a[1];L dn=gl_(a[2]),atr=gl_(a[3]);
 P(!dpath(pth,path,4096-8),dpe(pth),0)
 UC t=_t(v),kind,typ=t,att=0;W cnt,nb;CO V*src;A body=0;
 if(dn>=0){P(_tP(v)||!(t==tG||t==tH||t==tI||t==tL),et0(),0)
  if(t!=tI){body=cI(_R(v));P(!body,0)v=body;}   //find gives the narrowest int that holds the indexes
  kind=KENUM;typ=tI;cnt=_n(v);nb=cnt*4;src=_V(v);att=(UC)(atr>=0&&atr<5?atr:0);}
 else if(!_tP(v)&&dflat(t)){kind=KFLAT;cnt=_n(v);nb=dbytes(t,cnt);src=_V(v);att=_at(v);}
 else{I k=_tP(v)?0:tjk(v);
  if(TQ(k)){kind=KTIME;typ=(UC)k;body=tjn(v);cnt=_n(body);nb=cnt*8;src=_V(body);}
  else{kind=KSER;typ=0;body=ser8(_R(v));P(!body,0)cnt=_n(body);nb=cnt;src=_V(body);}}
 C hdr[DHB];MS(hdr,0,DHB);DHdr*h=(DHdr*)hdr;MC(h->mg,"AMBC",4);h->ver=1;h->kind=kind;h->typ=(UC)TXC(typ);h->att=att;h->cnt=cnt;h->nb=nb;h->dom=dn>=0?(W)dn:0;
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
A wcolqT(A x){dqf=1;A r=wcolT(x);dqf=0;return r;}   //`wcq: as `wcol, in q's files (dwtq)

// the payload of a column file as a vector of type t and count n: mapped when it can be, read otherwise
Z A dload(I f,UC t,W n,W nb){
 P(!n,an(0,t))
 A v=mfw(f,DHB,nb);
 if(v){_T(v)=t;_n(v)=(U)n;_at(v)=0;return v;}
 v=an((U)n,t);P(drall(f,_V(v),nb,DHB),mr(v);eo0())return v;}

/* 2.9.0: kdb+'s own files, so `\l db` reads a database q wrote. Three layouts:
 *   fe 20 t u ...   a flat vector: 16 bytes of header, then the items (q leaves the count at 8 as 0 at times: the size says it)
 *   fd 20 ...       a page of header (the enum's domain name at 16), q's vector header at 0xff0 (t at 0xff2, u at 0xff3,
 *                   the count at 0xff8), the items from 0x1000: an enum's are int64 indexes into the sym file
 *   ff 01 t ...     a value as -8! writes it without the message header: the sym file (a symbol list, read to its end:
 *                   q does not keep its count up to date as it appends) and .d
 * q's types map to Amber's: byte short int long to its ints, real to float, the temporals as they are (same numbers) */
Z A rcqv(I f,UC t,UC u,W n,W off){P(t>=20||!qtA[t],err0("nyi"))UC at=qtA[t];W w=qtW[t],nb=n*w;P(n>0xffffffffull,ez0())
 if(at!=tB&&t!=4&&w==((W)1<<Tw[at])>>3){A v=an((U)n,at);P(nb&&drall(f,_V(v),nb,off),mr(v);eo0())_at(v)=u<5?u:0;return v;}
 UC*b=malloc(nb?nb:1);P(!b,ez0())if(nb&&drall(f,b,nb,off)){free(b);return eo0();}
 A v=an((U)n,at);
 if(at==tB){MS(_V(v),0,(n+7)>>3);UC*o=(UC*)_V(v);for(W i=0;i<n;i++)o[i>>3]|=(UC)((b[i]&1)<<(i&7));}
 else if(at==tF){F*o=(F*)_V(v);for(W i=0;i<n;i++){float g;MC(&g,b+4*i,4);o[i]=(F)g;}}
 else{L*o=(L*)_V(v);for(W i=0;i<n;i++){
  if(w==1)o[i]=b[i];
  else if(w==2){short s;MC(&s,b+2*i,2);o[i]=s==(short)0x8000?NL:s;}
  else{I s;MC(&s,b+4*i,4);o[i]=s==NIT?NL:s;}}}   //q's nulls (0Nh 0Ni) to Amber's 0N
 free(b);_at(v)=u<5?u:0;return v;}
Z A rcq(I f,W sz,CO UC*m,A dom){
 if(m[0]==0xfe)return rcqv(f,m[2],m[3],sz>=16?(sz-16)/(qtW[m[2]<20?m[2]:0]?qtW[m[2]<20?m[2]:0]:1):0,16);
 if(m[0]==0xfd){UC k[16];P(sz<0x1000||drall(f,k,16,0xff0),err0("format"))UC t=k[2],u=k[3];W n;MC(&n,k+8,8);if(!n&&sz>0x1000)n=(sz-0x1000)/(t&&t<20&&qtW[t]?qtW[t]:8);   //a count of 0 here too at times: the size says it
  P(n>0xffffffffull||0x1000+n*8>sz,err0("format"))
  if(t<20)return t?rcqv(f,t,u,n,0x1000):err0("nyi");   //a nested column (strings) keeps its items in a col# file: not yet
  L*ix=malloc(n?n*8:8);P(!ix,ez0())if(n&&drall(f,ix,n*8,0x1000)){free(ix);return eo0();}
  A r;if(!dom){r=an((U)n,tL);MC(_V(r),ix,(N)n*8);}
  else{U dn=_n(dom);CO I*sy=(CO I*)_V(dom);r=an((U)n,tS);I*o=(I*)_V(r);
   for(W i=0;i<n;i++){if(ix[i]<0||(W)ix[i]>=dn){free(ix);mr(r);return err0("sym");}o[i]=sy[ix[i]];}}
  free(ix);_at(r)=u<5?u:0;return r;}
 // ff 01: the whole file
 UC*b=malloc(sz+8);P(!b,ez0())if(drall(f,b+8,sz,0)){free(b);return eo0();}
 if(sz>=8&&b[10]==11){W i=16,k=0;for(W j=i;j<sz+8;j++)k+=!b[j];A r=an((U)k,tS);I*o=(I*)_V(r);W c=0;   //names to the end
  while(i<sz+8&&c<k){C*s=(C*)b+i;N l=strlen(s);o[c++]=(I)us(s);i+=l+1;}free(b);return r;}
 // anything else: q's bytes behind a message header, to qdes
 A y=an((U)(sz-2+8),tC);UC*p=(UC*)_V(y);p[0]=1;p[1]=0;p[2]=0;p[3]=0;I len=(I)(sz-2+8);MC(p+4,&len,4);MC(p+8,b+10,(N)(sz-2));free(b);return qdes(y);}

// `rcol path or `rcol(path;syms): one column file back. A symbol column needs the database's syms (a sym
// vector); without them its indexes come back as ints. A file q wrote reads too (rcq above)
A rcolT(A x){
 A pth=x,dom=0;
 if(_t(x)==tA&&_n(x)==2){pth=_A(x)[0];dom=_A(x)[1];P(_t(dom)!=tS,x(et0()))}
 C path[4096];P(!dpath(pth,path,SZ path),x(dpe(pth)))
 I f=open(path,O_RDONLY);P(f<0,x(eo0()))
 struct stat st;DHdr h;I e=fstat(f,&st);
 if(!e&&S_ISREG(st.st_mode)&&st.st_size>=8){UC m[4];
  if(!drall(f,m,4,0)&&(m[0]==0xfe&&m[1]==0x20||m[0]==0xfd&&m[1]==0x20||m[0]==0xff&&m[1]==1)){A r=rcq(f,(W)st.st_size,m,dom);close(f);mr(x);return r;}}
 // a file that opens but is too short for the header is damage, 'format (#94 Q14); 'io is a failed open, stat or
 // read, and a directory
 if(e||(W)st.st_size<DHB||drall(f,&h,SZ h,0)){close(f);return x(!e&&S_ISREG(st.st_mode)&&(W)st.st_size<DHB?err0("format"):eo0());}
 // an attribute byte that names none of the four is damage too: copied into _at, `at read "\0supg" past its end
 h.typ=(UC)TIC(h.typ);   //numbered as before 2.9 (a.h TXC)
 B ok=!memcmp(h.mg,"AMBC",4)&&h.ver==1&&h.kind<=KTIME&&h.att<5&&(W)st.st_size>=DHB+h.nb;
 if(ok)S(h.kind,
  C(KFLAT,ok=h.typ<tn&&dflat(h.typ)&&h.nb==dbytes(h.typ,h.cnt))
  C(KENUM,ok=h.nb==h.cnt*4)
  C(KSER, ok=h.nb==h.cnt)
  C(KTIME,ok=TQ(h.typ)&&h.nb==h.cnt*8))
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
    r=an(n,TQL(h.typ));   //a list of the kind (2.9.0; it was a general list of atoms)
    if(TQN(h.typ)){MC(_V(r),q,(N)n*8);}
    else{I*o=(I*)_V(r);for(U i=0;i<n;i++)o[i]=q[i]==NL?NIT:(I)q[i];}   //a null as tjn wrote it (NL), back to its kind's
    mr(b)))
 out:
 close(f);mr(x);return r;}

// `fsz path: the size of a file, -1 when there is nothing there, -2 for a directory. Other failures are
// 'io: hdb.k must never take a sym file it could not read for a missing one and write a new one over it.
A fszT(A x){C path[4096];P(!dpath(x,path,SZ path),x(dpe(x)))struct stat st;
 if(stat(path,&st))return x(errno==ENOENT||errno==ENOTDIR?al(-1):eo0());
 return x(al(S_ISDIR(st.st_mode)?-2:(L)st.st_size));}

// `ldir path: (directories;files) in a directory, each a list of names in byte order, . and .. left out
#include<dirent.h>
Z I dcmp(CO V*a,CO V*b){return strcmp(*(C*CO*)a,*(C*CO*)b);}
A ldirT(A x){C path[4096];P(!dpath(x,path,SZ path-260),x(dpe(x)))DIR*d=opendir(path);P(!d,x(eo0()))
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
A wcolqT(A x){return x(en0());}
A fszT(A x){return x(en0());}
A ldirT(A x){return x(en0());}
#endif
