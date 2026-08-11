#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
typedef int32_t I; typedef int16_t H; typedef signed char G; typedef int64_t L; typedef uint64_t W; typedef unsigned U;
#define MIN(a,b) ((a)<(b)?(a):(b))
#define AMPARN 1000000u
#ifdef _OPENMP
 #define PR(x) _Pragma(#x)
 #define AMPFOR(...) PR(omp parallel for simd reduction(__VA_ARGS__) schedule(static))
 #define AMSIMD PR(omp simd)
#else
 #define AMPFOR(...)
 #define AMSIMD
#endif
/* candidate A: narrow types - plain loop, hard-branch threading */
#define NARROW(T,id) const T*restrict p=a;T r=id;\
 if(n>=AMPARN){AMPFOR(min:r) for(U i=0;i<n;i++)r=MIN(r,p[i]);}\
 else {for(U i=0;i<n;i++)r=MIN(r,p[i]);}\
 return (L)r;
/* candidate B: 64-bit - 4-way partials, hard-branch threading */
#define WIDE4(T,id) const T*restrict p=a;T a0=id,b0=id,c0=id,d0=id;U m=n&~(U)3;\
 if(n>=AMPARN){AMPFOR(min:a0,b0,c0,d0) for(U i=0;i<m;i+=4){a0=MIN(a0,p[i]);b0=MIN(b0,p[i+1]);c0=MIN(c0,p[i+2]);d0=MIN(d0,p[i+3]);}}\
 else {for(U i=0;i<m;i+=4){a0=MIN(a0,p[i]);b0=MIN(b0,p[i+1]);c0=MIN(c0,p[i+2]);d0=MIN(d0,p[i+3]);}}\
 T r=MIN(MIN(a0,b0),MIN(c0,d0));for(U i=m;i<n;i++)r=MIN(r,p[i]);return (L)r;
#define MULK(T) const T*restrict p=a;W a0=1,b0=1,c0=1,d0=1;U m=n&~(U)3;\
 if(n>=AMPARN){AMPFOR(*:a0,b0,c0,d0) for(U i=0;i<m;i+=4){a0*=(W)p[i];b0*=(W)p[i+1];c0*=(W)p[i+2];d0*=(W)p[i+3];}}\
 else {for(U i=0;i<m;i+=4){a0*=(W)p[i];b0*=(W)p[i+1];c0*=(W)p[i+2];d0*=(W)p[i+3];}}\
 W r=a0*b0*c0*d0;for(U i=m;i<n;i++)r*=(W)p[i];return (L)r;
#define ORIGMIN(T,id) const T*p=a;T r=id;for(U i=0;i<n;i++)r=MIN(r,p[i]);return (L)r;
#define ORIGMUL(T) const T*p=a;L r=1;for(U i=0;i<n;i++)r*=*p++;return r;
L l_o(const void*a,U n){ORIGMIN(L,9223372036854775807LL)} L l_n(const void*a,U n){WIDE4(L,9223372036854775807LL)}
L i_o(const void*a,U n){ORIGMIN(I,2147483647)}          L i_n(const void*a,U n){NARROW(I,2147483647)}
L h_o(const void*a,U n){ORIGMIN(H,32767)}               L h_n(const void*a,U n){NARROW(H,32767)}
L g_o(const void*a,U n){ORIGMIN(G,127)}                 L g_n(const void*a,U n){NARROW(G,127)}
L ml_o(const void*a,U n){ORIGMUL(L)} L ml_n(const void*a,U n){MULK(L)}
L mg_o(const void*a,U n){ORIGMUL(G)} L mg_n(const void*a,U n){MULK(G)}
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec*1e3+t.tv_nsec/1e6;}
typedef L(*fn)(const void*,U);
static double run(fn f,const void*d,U n,int r){double b=1e18;volatile L s=0;for(int k=0;k<r;k++){double t=now();s=f(d,n);double e=now()-t;if(e<b)b=e;}(void)s;return b;}
int main(void){U N=20000000;void*d=malloc((size_t)N*8);for(U i=0;i<N;i++)((L*)d)[i]=(L)(i%1000)+5;
 struct{const char*nm;fn o,n;}T[]={{"min L",l_o,l_n},{"min I",i_o,i_n},{"min H",h_o,h_n},{"min G",g_o,g_n},{"mul L",ml_o,ml_n},{"mul G",mg_o,mg_n}};
 U ns[]={10000,100000,2000000,20000000};int rp[]={8000,1500,120,15};
 for(int t=0;t<6;t++){for(int s=0;s<4;s++){double o=run(T[t].o,d,ns[s],rp[s]),x=run(T[t].n,d,ns[s],rp[s]);
   printf("%s n=%-9u  before %8.4f  after %8.4f  ms   %.2fx\n",T[t].nm,ns[s],o,x,o/x);}printf("\n");}
 return 0;}
