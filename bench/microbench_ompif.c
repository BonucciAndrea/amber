#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
typedef int64_t L; typedef unsigned U;
#define AMPARN 1000000u
#define PR(x) _Pragma(#x)
#define AMRED_IF(n,...) PR(omp parallel for simd reduction(__VA_ARGS__) if((n)>=AMPARN) schedule(static))
#define AMPFOR(...)     PR(omp parallel for simd reduction(__VA_ARGS__) schedule(static))
/* upstream addfL: pragma with if() clause */
L up(const void*a,U n){const L*restrict p=a;L r=0;AMRED_IF(n,+:r)for(U i=0;i<n;i++)r+=p[i];return r;}
/* hard-branch variant */
L hb(const void*a,U n){const L*restrict p=a;L r=0;
 if(n>=AMPARN){AMPFOR(+:r)for(U i=0;i<n;i++)r+=p[i];}
 else{for(U i=0;i<n;i++)r+=p[i];}return r;}
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec*1e3+t.tv_nsec/1e6;}
int main(void){U N=20000000;L*d=malloc((size_t)N*8);for(U i=0;i<N;i++)d[i]=i%1000;
 U ns[]={1000,10000,100000,999999,2000000,20000000};int rp[]={20000,8000,2000,200,120,15};
 for(int s=0;s<6;s++){U n=ns[s];double bu=1e18,bh=1e18;volatile L t;
  for(int k=0;k<rp[s];k++){double q=now();t=up(d,n);double e=now()-q;if(e<bu)bu=e;}
  for(int k=0;k<rp[s];k++){double q=now();t=hb(d,n);double e=now()-q;if(e<bh)bh=e;}(void)t;
  printf("addfL n=%-9u  if()clause %8.4f  hard-branch %8.4f ms   %.2fx\n",n,bu,bh,bu/bh);}
 return 0;}
