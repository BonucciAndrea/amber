#include"a.h" // ngn/k, (c) 2019-2024 ngn, GNU AGPLv3 - https://codeberg.org/ngn/k/raw/branch/master/LICENSE
#include<stdarg.h>
#include<stdio.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<netinet/in.h>
#include<netinet/tcp.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/time.h>
#undef __USE_EXTERN_INLINES
#include<sys/stat.h>
I pg=4096;//pagesize

//__builtin_ia32_ldmxcsr(__builtin_ia32_stmxcsr()|1<<6|1<<15); //daz,ftz
#ifndef shared
#ifndef ldstatic
 I main(I n,S*a)_(kinit();kargs(n,a);I r=0;I(n<2,repl())J(!bsl(a[1]),r=1;epr(0))Q(bsm(""));r)
#endif
#endif

#if defined(wasm)
 I pipe(I v[2])_(-1)
 C*getcwd(C*s,N n)_((V*)0)
 I js_in(V*,N);V js_out(CO V*,N),js_log(CO V*),*js_alloc(N),js_time(I*,long*),js_exit(I);
 Z ST{C*a,p[16];N n;}s[8]={{.a=""},{.a=""},//s:storage,
  #include"o/w/fs.h"
 };Z ST{C i;N o;}d[8]={{.i=1},{.i=1},{.i=1}};Z CO I ns=L(s),nd=L(d);//d:fd table
 #define FI P((U)f>=nd||!d[f].i,EBADF)I i=d[f].i;//validate fd "f" and get inode "i"
 I open(S p,I v,...)_(I m=SL(p);P(m>=SZ s[0].p,ENAMETOOLONG)I i=0;W(i<ns&&strcmp(s[i].p,p),i++)
  I(i>=ns,P(O_CREAT&~v,ENOENT)i=0;W(i<ns&&s[i].a,i++)P(i>=ns,ENOSPC)s[i].a="";s[i].n=0;MC(s[i].p,p,m))
  I f=0;W(f<nd&&d[f].i,f++)P(f>=nd,EMFILE)d[f].i=i;d[f].o=0;f)
 I close(I f)_(FI d[f].i=0;0)
 I read(I f,V*a,N n)_(FI P(i==1,js_in(a,n))I o=d[f].o;n=MAX(0,MIN(n,s[i].n-o));MC(a,s[i].a+o,n);d[f].o+=n;n)
 I write(I f,CO V*a,N n)_(FI;P(i==1,js_out(a,n);n)
  I m=d[f].o+n;I(m>s[i].n,C*b=js_alloc(m);MC(b,s[i].a,n);s[i].a=b;s[i].n=m)MC(s[i].a+d[f].o,a,n);n)
 off_t lseek(I f,off_t o,I w)_(FI;o=w==SEEK_CUR?o+d[f].o:w==SEEK_END?o+s[i].n:w==SEEK_SET?o:-1;P(o<0,EINVAL)d[f].o=o)
 I fstat(I f,ST stat*r)_(FI;I n=s[i].n;
  *r=(TY(*r)){.st_ino=i,.st_mode=S_IFCHR,.st_nlink=1,.st_size=n,.st_blksize=512,.st_blocks=n+511>>9};0)
 V*mmap(V*a,N n,I pr,I fl,I f,off_t o)_(I(!a,a=js_alloc(n))P(f<0,a)P(f>=nd||!d[f].i,(V*)-1)I i=d[f].i;MC(a,s[i].a+o,n);a)
 I munmap(I f,I n)_(0)
 I gettimeofday(ST timeval*a,V*b)_(js_time((V*)&a->tv_sec,(V*)&a->tv_usec);0)
 V exit(I v){js_exit(v);}
 I dup2(I f,I v)_(-1)
 I execve(S p,C*CO*a,C*CO*e)_(-1)
 I fork()_(-1)
 I socket(I i,I j,I k)_(-1)
 I setsockopt(I f,I l,I s,CO V*v,socklen_t n)_(-1)
 I connect(I f,CO ST sockaddr*s,socklen_t n)_(-1)
 I chdir(S p)_(-1)
 I ftruncate(I f,off_t o)_(-1)
 I wait4(I i,I*l,I o,ST rusage*u)_(-1)
 long sysconf(I i)_(i==_SC_PAGESIZE?4096:-1)
 V*dlopen(S s,I i)_((V*)0)
 V*dlsym(V*p,S s)_((V*)0)
 I dlclose(V*p)_(0)
 C*dlerror()_("wasm")

 #include<dirent.h>
 DIR*fdopendir(I f)_((V*)0)
 ST dirent*readdir(DIR*x)_((V*)0)
 I closedir(DIR*x)_(0)

 V*memcpy (V*x,CO V*y,N n)_(C*p=x  ;S q=y  ;F(n,*p++=*q++)x)
 V*memrcpy(V*x,CO V*y,N n)_(C*p=x+n;S q=y+n;F(n,*--p=*--q)x)
 V*memmove(V*x,CO V*y,N n)_((y<x&&x<y+n?memrcpy:memcpy)(x,y,n))
 V*memset(V*x,I v,N n)_(C*p=x;F(n,*p++=v);x)
 V*memchr(CO V*x,I v,N n)_(S s=x;F(n,P(s[i]==v,(V*)(s+i)))(V*)0)
 V*memmem(CO V*x,N m,CO V*y,N n)_(S p=x,q=y;F((L)m-(L)n+1,P(!memcmp(p+i,q,n),(V*)(p+i)))(V*)0)
 I memcmp(CO V*x,CO V*y,N n)_(S s=x,t=y;F(n,I v=*s++-*t++;P(v,v))0)
 N strlen(S s)_(S p=s;W(1,W v=~*(W*)(V*)p;v&=v>>1;v&=v>>2;v&=v>>4;v&=0x0101010101010101ll;B(v)p+=8)W(*p,p++)p-s)
 C*strchr(CO C*s,I v)_(W(1,P(*s==v,(V*)s)P(!*s++,(V*)0))(V*)0)
 C*strstr(CO C*p,CO C*q)_(memmem(p,SL(p),q,SL(q)))
 I strcmp(S p,S q)_(W(*p&&*p==*q,p++;q++)*p-*q)
#else
 U js_eval(C*s,U m,C*r,U n)_(0)
#endif

#if !defined(__GNU_SOURCE)
 C*strchrnul(S s,I v)_(C*p=strchr(s,v);p?p:(V*)s+SL(s))
#endif
