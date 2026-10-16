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
#include"parallel.h"
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
#include<sys/file.h>
#include<stdio.h>
#include<unistd.h>
#include<ftw.h>
#include<dirent.h>
A mfw(I,W,W);I tjk(A);A tjn(A);

/* 2.9.1: durable writes. A file is fsynced before it's renamed in, and the directories it went into after, so a crash
 * or a power cut leaves the old file or the new one, never a hole where the data was. A directory that isn't there yet
 * (a new partition, a new table, a new database) is written whole under a hidden name next to it, .name.amber-tmp,
 * and renamed in at the end: a crash leaves the database as it was, plus that hidden directory, which readers skip and
 * the next write of it clears. A table that is there and written whole (its .d with it) is staged the same way and
 * swapped in: renamed to .name.amber-old, the stage renamed in, the old one removed. A crash between the two renames
 * leaves the stage complete and no table, and `drc puts it in (hdb.k runs it on loading and before writing, under the
 * database's lock). Where a directory can't be renamed (a file in it mapped, on Windows), the files go in one by one
 * as before. AMBER_FSYNC=0 skips the syncs (scratch data, benchmarks), never the renames. */
Z I dsy=-1;
Z B dsyn(V){if(dsy<0){CO C*e=getenv("AMBER_FSYNC");dsy=!(e&&e[0]=='0'&&!e[1]);}return (B)dsy;}
Z I dfsy(I f){return dsyn()?fsync(f):0;}
Z V dfsd(CO C*d){if(!dsyn())return;I f=open(*d?d:".",O_RDONLY);if(f>=0){fsync(f);close(f);}}   //a directory's entries. Best effort: not every system syncs a directory
/* Files synced one after another wait for the disk once each; synced at once from a few threads, the file system
 * flushes them together (a 1M-row partition: 2.4x main's write time one by one, see wcolN for what it is now) */
TD ST{I*f;N n;I nt,bad;}DSJ;
Z V dsy1(V*c_,int t){DSJ*c=c_;for(N i=(N)t;i<c->n;i+=(N)c->nt)if(c->f[i]>=0&&fsync(c->f[i]))__atomic_store_n(&c->bad,1,__ATOMIC_RELAXED);}
Z I dsyall(I*f,N n){if(!dsyn()||!n)return 0;DSJ c={f,n,(I)(n<16?n:16),0};par_run(c.nt,dsy1,&c);return c.bad;}
Z I drm1(CO C*p,CO ST stat*s,I t,ST FTW*w){(V)s;(V)t;(V)w;return remove(p);}
Z V drmr(CO C*p){struct stat st;if(!lstat(p,&st))nftw(p,drm1,16,FTW_DEPTH|FTW_PHYS);}   //rm -r
// the topmost directory above file p that isn't there, into r: 1 when there is one, 0 when p's directory is there
Z B dmiss(CO C*p,C*r){N l=SL(p);for(N k=1;k<l;k++)if(p[k]=='/'&&p[k-1]!='/'){MC(r,p,k);r[k]=0;struct stat st;
  if(stat(r,&st))return errno==ENOENT;}return 0;}
Z V dstg(CO C*r,C*s,N cap,CO C*k){CO C*b=strrchr(r,'/');I pl=b?(I)(b-r+1):0;snprintf(s,cap,"%.*s.%s.amber-%s",pl,r,b?b+1:r,k);}   //r's stage (k tmp) or old name
Z V dpar(CO C*p,C*d,N cap){CO C*b=strrchr(p,'/');I pl=b?(I)(b-p):0;snprintf(d,cap,"%.*s",pl,p);}   //p's directory, "" for .
// a list of distinct strings
TD ST{C**v;N n,c;}DL;
Z I dlix(DL*l,CO C*s){for(N i=0;i<l->n;i++)if(!strcmp(l->v[i],s))return (I)i;return -1;}
Z B dladd(DL*l,CO C*s){if(dlix(l,s)>=0)return 1;if(l->n==l->c){N c=l->c?2*l->c:8;C**v=realloc(l->v,c*SZ(C*));if(!v)return 0;l->v=v;l->c=c;}
  C*d=strdup(s);if(!d)return 0;l->v[l->n++]=d;return 1;}
Z V dlfree(DL*l){for(N i=0;i<l->n;i++)free(l->v[i]);free(l->v);l->v=0;l->n=l->c=0;}
Z V dfsdl(DL*l){if(!dsyn()||!l->n)return;I*f=malloc(l->n*SZ(I));if(!f){for(N i=0;i<l->n;i++)dfsd(l->v[i]);return;}   //every directory in l, best effort
 for(N i=0;i<l->n;i++)f[i]=open(*l->v[i]?l->v[i]:".",O_RDONLY);dsyall(f,l->n);for(N i=0;i<l->n;i++)if(f[i]>=0)close(f[i]);free(f);}


#define DHB 4096u
#define DPB 4200   //a path's buffer: the path, a stage name in it and .tmp
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
Z I dopen(CO C*path,C*tmp){snprintf(tmp,DPB,"%s.tmp",path);I f=open(tmp,O_WRONLY|O_CREAT|O_TRUNC,0644);
 if(f<0&&errno==ENOENT&&!dmkp(tmp))f=open(tmp,O_WRONLY|O_CREAT|O_TRUNC,0644);return f;}
Z I dwtq(A x,CO C*path,C*tmp,I*fo){   //1 written, 0 failed (error raised), -1 not a kind q has: Amber's file then
 P(_t(x)!=tA||_n(x)!=4,-1)A*a=_A(x);A v=a[1];L dn=gl_(a[2]);P(_tP(v),-1)UC t=_t(v);W n=_n(v);
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
 I bad=dwall(f,b,m)||(!fo&&dfsy(f));free(b);
 if(bad||!fo)bad|=close(f);
 if(bad){unlink(tmp);eo0();return 0;}
 if(fo)*fo=f;return 1;}
// One quadruple x to path.tmp: 1 when written; 0 when not, with the error raised and no .tmp left of it. Synced and
// closed, or with fo left open in *fo for the caller to sync with the rest.
Z B dwt(A x,CO C*path,C*tmp,I*fo){
 if(dqf){I r=dwtq(x,path,tmp,fo);if(r>=0)return (B)r;}
 P(_t(x)!=tA||_n(x)!=4,et0(),0)
 A*a=_A(x);A v=a[1];L dn=gl_(a[2]),atr=gl_(a[3]);
 UC t=_t(v),kind,typ=t,att=0;W cnt,nb;CO V*src;A body=0;
 if(dn>=0){P(_tP(v)||!(t==tG||t==tH||t==tI||t==tL),et0(),0)
  if(t!=tI){body=cI(_R(v));P(!body,0)v=body;}   //find gives the narrowest int that holds the indexes
  kind=KENUM;typ=tI;cnt=_n(v);nb=cnt*4;src=_V(v);att=(UC)(atr>=0&&atr<5?atr:0);}
 else if(!_tP(v)&&dflat(t)){kind=KFLAT;cnt=_n(v);nb=dbytes(t,cnt);src=_V(v);att=_at(v);}
 else{I k=_tP(v)?0:tjk(v);
  if(TQ(k)){kind=KTIME;typ=(UC)k;body=tjn(v);cnt=_n(body);nb=cnt*8;src=_V(body);}
  else{kind=KSER;typ=0;body=ser8(_R(v));P(!body,0)cnt=_n(body);nb=cnt;src=_V(body);}}
 C hdr[DHB];MS(hdr,0,DHB);DHdr*h=(DHdr*)hdr;MC(h->mg,"AMBC",4);h->ver=1;h->kind=kind;h->typ=(UC)TXC(typ);h->att=att;h->cnt=cnt;h->nb=nb;h->dom=dn>=0?(W)dn:0;
 I f=dopen(path,tmp);
 if(f<0){if(body)mr(body);eo0();return 0;}
 I bad=dwall(f,hdr,DHB)||(nb&&dwall(f,src,nb))||(!fo&&dfsy(f));if(bad||!fo)bad|=close(f);
 if(body)mr(body);
 if(bad){unlink(tmp);eo0();return 0;}
 if(fo)*fo=f;return 1;}
// two quadruples naming one path would share its .tmp file, and one of them would be lost: 'domain before any write
Z B dwdup(A x){U n=_n(x);F(n,A a=_A(x)[i];P(_t(a)!=tA||_n(a)!=4,0)A p=*_A(a);P(_t(p)!=tC,0)
 for(U j=0;j<i;j++){A q=*_A(_A(x)[j]);if(_n(q)==_n(p)&&!memcmp(_V(q),_V(p),_n(p)))return 1;})return 0;}
/* Every file goes to its .tmp, synced, and only once all are written are they renamed, in the list's order: a splayed
 * table is written so (the sym file first, .d last), and a write that fails before the renames leaves the old table as
 * it was (#94 Q12). A file whose directory isn't there yet goes into that directory's stage (see the top), renamed in
 * once the files and the directories in it are synced. Then the directories the renames changed are synced. */
Z B disd(CO C*p){struct stat st;return !stat(p,&st)&&S_ISDIR(st.st_mode);}
Z A wcolN(A x){U n=_n(x);P(dwdup(x),x(ed0()))
 C path[DPB],tmp[DPB],r[DPB],s[DPB];C**sp=calloc(n?n:1,SZ(C*));I*ri=calloc(n?n:1,SZ(I)),*fd=malloc((n?n:1)*SZ(I));
 P(!sp||!ri||!fd,free(sp);free(ri);free(fd);x(ez0()))F(n,fd[i]=-1)B kf=n<=256;   //kf: keep the files open, to sync them together
 DL rt={0},st={0},ds={0};B sw[64]={0};U w=0,k=0;B ok=1;   //roots, their stages, in step (sw: the root is there, a swap); directories to sync
 // a table written whole whose directory is there: its directory is a root to swap
 for(U i=0;ok&&i<n;i++){A a=_A(x)[i];if(_t(a)!=tA||_n(a)!=4){et0();ok=0;break;}
  if(!dpath(*_A(a),path,DPB-32)){dpe(*_A(a));ok=0;break;}
  N l=SL(path);if(l>3&&!strcmp(path+l-3,"/.d")&&rt.n<64){dpar(path,r,SZ r);if(disd(r)&&dlix(&rt,r)<0){dstg(r,s,SZ s,"tmp");drmr(s);
   if(!dladd(&rt,r)||!dladd(&st,s)){ez0();ok=0;break;}sw[rt.n-1]=1;}}}
 for(U i=0;ok&&i<n;i++){A a=_A(x)[i];dpath(*_A(a),path,DPB-32);I j=-1;
  if(dmiss(path,r)){j=dlix(&rt,r);if(j<0&&rt.n<64){dstg(r,s,SZ s,"tmp");drmr(s);if(!dladd(&rt,r)||!dladd(&st,s)){ez0();ok=0;break;}j=(I)rt.n-1;}}
  else{dpar(path,r,SZ r);j=dlix(&rt,r);}
  ri[i]=j;if(j>=0){snprintf(s,SZ s,"%s%s",st.v[j],path+SL(rt.v[j]));sp[i]=strdup(s);}else sp[i]=strdup(path);
  if(!sp[i]){ez0();ok=0;}}
 for(;ok&&w<n;w++)if(!dwt(_A(x)[w],sp[w],tmp,kf?fd+w:0)){ok=0;break;}   //w, k: how many written, renamed
 if(ok&&dsyall(fd,w)){errno=EIO;eo0();ok=0;}
 for(U i=0;i<n;i++)if(fd[i]>=0){if(close(fd[i])&&ok){eo0();ok=0;}fd[i]=-1;}
 for(;ok&&k<n;k++){snprintf(tmp,SZ tmp,"%s.tmp",sp[k]);if(rename(tmp,sp[k])){eo0();ok=0;break;}}
 /* the directories the renames changed are synced once, at the end: a file's own directory when it went straight in,
  * the stage's parent when a stage did. What the files and directories inside a stage hold is journaled before the
  * stage's rename on every journaling file system (ext4, XFS, APFS, NTFS), so it isn't synced on its own: two waits
  * for the disk a partition, not six. Where nothing is journaled, a crash can lose the newest write, never the old */
 if(ok){for(N j=0;ok&&j<rt.n;j++){C*R=rt.v[j],*T=st.v[j];
   if(!sw[j]){if(rename(T,R)){eo0();ok=0;break;}}
   else{dstg(R,s,SZ s,"old");drmr(s);
    if(!rename(R,s)){if(rename(T,R)){I e=errno;rename(s,R);errno=e;eo0();ok=0;break;}drmr(s);}
    else for(U i=0;i<n;i++)if(ri[i]==(I)j){dpath(*_A(_A(x)[i]),path,DPB-32);if(rename(sp[i],path)){eo0();ok=0;break;}}}   //one by one
   dpar(R,s,SZ s);dladd(&ds,s);}
  for(U i=0;i<n;i++)if(ri[i]<0){dpar(sp[i],s,SZ s);dladd(&ds,s);}
  dfsdl(&ds);}
 if(!ok){for(U i=k;i<w;i++){snprintf(tmp,SZ tmp,"%s.tmp",sp[i]);unlink(tmp);}}
 for(N j=0;j<st.n;j++)drmr(st.v[j]);   //a stage renamed in is gone; one the files went out of one by one is cleared
 for(U i=0;i<n;i++)free(sp[i]);free(sp);free(ri);free(fd);dlfree(&rt);dlfree(&st);dlfree(&ds);
 P(!ok,x(0))A z=aA(n);F(n,_A(z)[i]=_R(*_A(_A(x)[i])))mr(x);return z;}
A wcolT(A x){
 P(_t(x)==tA&&_n(x)&&_t(*_A(x))==tA,wcolN(x))
 A l=aA(1);_A(l)[0]=x;A r=wcolN(l);P(!r,0)A z=_R(*_A(r));mr(r);return z;}
A wcolqT(A x){dqf=1;A r=wcolT(x);dqf=0;return r;}   //`wcq: as `wcol, in q's files (dwtq)

/* `dlk dir: an exclusive lock on directory dir (made when it isn't there), held until `duk of what it gives back.
 * hdb.k holds it from reading a database's sym file to writing it back, so two processes adding symbols to one
 * database can't lose each other's names. flock is advisory and q doesn't take it; where a system can't lock a
 * directory it goes on unlocked, as before. */
A dlkT(A x){C p[4200];P(!dpath(x,p,4096-8),x(dpe(x)))N l=SL(p);p[l]='/';p[l+1]='x';p[l+2]=0;dmkp(p);p[l]=0;
 I f=open(p,O_RDONLY);P(f<0,x(eo0()))while(flock(f,LOCK_EX)&&errno==EINTR);mr(x);return ai(f);}
A dukT(A x){P(!_tz(x),x(et0()))I f=(I)gl(x);mr(x);P(f<3,et0())flock(f,LOCK_UN);close(f);return au;}
/* `drc dir: settles what a write that died left in dir (see the top): .name.amber-old with no name is a swap cut
 * between its renames, so the stage, complete by then, goes in (or the old one back when there's no stage); then
 * every .name.amber-old and .name.amber-tmp left is removed, and every file.tmp (a file a write died in before its
 * rename: q's \l would take it for a table). Run it holding the database's lock (hdb.k does). */
A drcT(A x){C p[DPB],a[DPB],b[DPB],c[DPB];P(!dpath(x,p,DPB-300),x(dpe(x)))mr(x);DIR*d=opendir(p);P(!d,au)DL o={0},t={0};ST dirent*e;
 while((e=readdir(d))){CO C*m=e->d_name;N l=SL(m);
  if(l>4&&!strcmp(m+l-4,".tmp")){snprintf(b,SZ b,"%s/%s",p,m);struct stat st;if(!lstat(b,&st)&&S_ISREG(st.st_mode))unlink(b);continue;}
  if(m[0]!='.'||l<12)continue;
  if(!strcmp(m+l-10,".amber-old"))dladd(&o,m);else if(!strcmp(m+l-10,".amber-tmp"))dladd(&t,m);}
 closedir(d);
 for(N i=0;i<o.n;i++){CO C*m=o.v[i];I l=(I)SL(m)-11;snprintf(a,SZ a,"%s/%.*s",p,l,m+1);snprintf(b,SZ b,"%s/%s",p,m);
  snprintf(c,SZ c,"%s/.%.*s.amber-tmp",p,l,m+1);struct stat st;
  if(lstat(a,&st)){if(!rename(c,a)||!rename(b,a))dfsd(p);}
  drmr(b);}
 for(N i=0;i<t.n;i++){snprintf(b,SZ b,"%s/%s",p,t.v[i]);drmr(b);}
 dlfree(&o);dlfree(&t);return au;}

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
Z B qdir(UC t,UC at,W w)_(at!=tB&&t!=4&&w==((W)1<<Tw[at])>>3)   //q's items are Amber's as they are (copied, not converted)
//n items of q's type t (1..19) at b, as an Amber list
Z A qcv(UC t,W n,CO UC*b){P(t>=20||!qtA[t],err0("nyi"))UC at=qtA[t];W w=qtW[t];P(n>0xffffffffull,ez0())P(!n,emp(at))A v=an((U)n,at);
 if(qdir(t,at,w)){MC(_V(v),b,(N)(n*w));return v;}
 if(at==tB){MS(_V(v),0,(n+7)>>3);UC*o=(UC*)_V(v);for(W i=0;i<n;i++)o[i>>3]|=(UC)((b[i]&1)<<(i&7));}
 else if(at==tF){F*o=(F*)_V(v);for(W i=0;i<n;i++){float g;MC(&g,b+4*i,4);o[i]=(F)g;}}
 else{L*o=(L*)_V(v);for(W i=0;i<n;i++){
  if(w==1)o[i]=b[i];
  else if(w==2){short s;MC(&s,b+2*i,2);o[i]=s==(short)0x8000?NL:s;}
  else{I s;MC(&s,b+4*i,4);o[i]=s==NIT?NL:s;}}}   //q's nulls (0Nh 0Ni) to Amber's 0N
 return v;}
Z A rcqv(I f,UC t,UC u,W n,W off){P(t>=20||!qtA[t],err0("nyi"))UC at=qtA[t];W w=qtW[t],nb=n*w;P(n>0xffffffffull,ez0())
 if(qdir(t,at,w)){A v=an((U)n,at);P(nb&&drall(f,_V(v),nb,off),mr(v);eo0())_at(v)=u<5?u:0;return v;}
 UC*b=malloc(nb?nb:1);P(!b,ez0())if(nb&&drall(f,b,nb,off)){free(b);return eo0();}
 A v=qcv(t,n,b);free(b);P(!v,0)_at(v)=u<5?u:0;return v;}
/* 2.9.1: a nested column (strings, or lists of one type), as q 4 writes it. col has the page of header with t 77 at
 * 0xff2, then n int64s: where each item starts in the data of col# (past its own page of header). An offset's top byte
 * says how the item is kept there: 1 short (a byte 0x80+t-1 -- an atom's comes below 0x80 -- then a list's count in a
 * byte, then the data), 0 as q holds a vector (m a t u, r, the count at 8, the data from 16; t 77 with no items: ()) */
Z A qitem(CO UC*d,W dn,L e){W o=(W)e&0xffffffffffffffull;UC k=(UC)((W)e>>56);P(o>=dn,err0("format"))
 if(k==1){UC c=d[o];I t=(I)c-0x7f;
  if(t<0){P(-t>=20||!qtW[-t]||o+1+qtW[-t]>dn,err0("nyi"))A v=N(qcv((UC)-t,1,d+o+1));A a=ii(v,0);mr(v);return a;}   //an atom
  P(t>=20||!qtW[t]||o+2>dn,err0("nyi"))W n=d[o+1];P(n>=0x80,err0("nyi"))P(o+2+n*qtW[t]>dn,err0("format"))return qcv((UC)t,n,d+o+2);}
 P(k,err0("nyi"))P(o+16>dn,err0("format"))UC t=d[o+2];W n;MC(&n,d+o+8,8);
 I(t==77||!t,P(n,err0("nyi"))return emp(tA))
 P(t>=20||!qtW[t],err0("nyi"))P(n>(dn-o-16)/qtW[t],err0("format"))return qcv(t,n,d+o+16);}
Z A rcqn(I f,W n,CO C*path){C p[4100];N l=SL(path);P(l+2>SZ p,eo0())MC(p,path,l);p[l]='#';p[l+1]=0;
 I g=open(p,O_RDONLY);P(g<0,eo0())struct stat st;I(fstat(g,&st)||(W)st.st_size<0x1000,close(g);return err0("format"))
 W dn=(W)st.st_size-0x1000;UC*d=malloc(dn?dn:1);L*ix=malloc(n?n*8:8);
 I(!d||!ix,free(d);free(ix);close(g);return ez0())
 I((dn&&drall(g,d,dn,0x1000))||(n&&drall(f,ix,n*8,0x1000)),free(d);free(ix);close(g);return eo0())close(g);
 A r=an((U)n,tA);F(n,_A(r)[i]=au)F(n,A y=qitem(d,dn,ix[i]);I(!y,mr(r);r=0;break)_A(r)[i]=y)free(d);free(ix);return r;}
Z A rcq(I f,W sz,CO UC*m,A dom,CO C*path){
 if(m[0]==0xfe)return rcqv(f,m[2],m[3],sz>=16?(sz-16)/(qtW[m[2]<20?m[2]:0]?qtW[m[2]<20?m[2]:0]:1):0,16);
 if(m[0]==0xfd){UC k[16];P(sz<0x1000||drall(f,k,16,0xff0),err0("format"))UC t=k[2],u=k[3];W n;MC(&n,k+8,8);if(!n&&sz>0x1000)n=(sz-0x1000)/(t&&t<20&&qtW[t]?qtW[t]:8);   //a count of 0 here too at times: the size says it
  P(n>0xffffffffull||0x1000+n*8>sz,err0("format"))
  if(t==77)return rcqn(f,n,path);   //a nested column: its items in col# (2.9.1)
  if(t<20)return t?rcqv(f,t,u,n,0x1000):err0("nyi");
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
  if(!drall(f,m,4,0)&&(m[0]==0xfe&&m[1]==0x20||m[0]==0xfd&&m[1]==0x20||m[0]==0xff&&m[1]==1)){A r=rcq(f,(W)st.st_size,m,dom,path);close(f);mr(x);return r;}}
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
A dlkT(A x){return x(en0());}
A dukT(A x){return x(en0());}
A drcT(A x){return x(en0());}
#endif
