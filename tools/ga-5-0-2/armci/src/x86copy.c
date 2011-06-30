// $Id: x86copy.gcc,v 1.14 2003-02-06 18:49:36 manoj Exp $
// memory copy implementation derived from examples 3 and 4 at
// http://www.sgi.com/developers/library/resources/asc_cpu.html
//
// void *armci_asm_memcpy(void *dst, const void *src, size_t n, int id);
// id={0/1} - is used to support up to 2 threads
//
// we turn the assembly memcpy on/off based on checking of the CPU type
//

#include <stdio.h>
#include <string.h>


// EAX
#define CPUFAMILY  0x00000F00
#define MODEL      0x000000F0
// EDX
#define MMXSUPPORT 0x00800000
#define SSESUPPORT 0x02000000
#define IA64       0x70000000
static  char vendor[13];
typedef enum {unkown=0,p5, p2, p2m, p3, p4, pro, p6, K5, K6, K7, ia64} CPUTYPE;


#define EX4  // use sgi's example_4_cpy if n is greater than 2k
             // use memcpy and example_3_memcpy for the rest 
//#define  EX3  // use memcpy and example_3_memcpy

static char tbuf0[2048];
static char tbuf1[2048];
static char *ptbuf;
static int use_asm_copy=-1;

//
void *asm_memcpy3(void *dst, const void *src, size_t n)
{

	asm __volatile__ ("\
	      pushl %%ebx;\
              shr $6, %2;\
	loopl:\
	      prefetchnta 64(%0);\
	      prefetchnta 96(%0);\
	      movq 0(%0), %%mm1;\
	      movq 8(%0), %%mm2;\
	      movq 16(%0), %%mm3;\
	      movq 24(%0), %%mm4;\
	      movq 32(%0), %%mm5;\
	      movq 40(%0), %%mm6;\
	      movq 48(%0), %%mm7;\
	      movq 56(%0), %%mm0;\
	      movntq %%mm1, 0(%1);\
	      movntq %%mm2, 8(%1);\
	      movntq %%mm3, 16(%1);\
	      movntq %%mm4, 24(%1);\
	      movntq %%mm5, 32(%1);\
	      movntq %%mm6, 40(%1);\
	      movntq %%mm7, 48(%1);\
	      movntq %%mm0, 56(%1);\
	      add $64, %0;\
	      add $64, %1;\
	      dec %2;\
	      jnz loopl;\
	      emms;\
              popl %%ebx;"
	      : 
	      : "r"(src), "r"(dst), "c"(n)
	      : "%eax"
	      );

	return dst;
}


void *asm_memcpy4(void *dst, const void *src, size_t n, int bufid)
{
	
	int dum;

	if(bufid == 0) 
		ptbuf = tbuf0;
	else    
		ptbuf = tbuf1;      

	asm __volatile__ ("\
	        shr $11, %0;\
\
	loop2k:\
	        movl $2048, %%ecx;\
	        shr $6, %%ecx;\
\
	loopMemToL1: \
        	prefetchnta 64(%%esi); \
	        prefetchnta 96(%%esi); \
\
        	movq 0(%%esi), %%mm1; \
	        movq 8(%%esi), %%mm2; \
	        movq 16(%%esi), %%mm3; \
	        movq 24(%%esi), %%mm4; \
	        movq 32(%%esi), %%mm5; \
	        movq 40(%%esi), %%mm6; \
	        movq 48(%%esi), %%mm7; \
	        movq 56(%%esi), %%mm0; \
\
	        movq  %%mm1, 0(%4); \
	        movq  %%mm2, 8(%4); \
	        movq  %%mm3, 16(%4); \
	        movq  %%mm4, 24(%4); \
	        movq  %%mm5, 32(%4); \
	        movq  %%mm6, 40(%4); \
	        movq  %%mm7, 48(%4); \
	        movq  %%mm0, 56(%4); \
\
	        add $64, %%esi;\
	        add $64, %4;\
	        dec %%ecx ;\
	        jnz loopMemToL1 ;\
\
                subl $2048, %4;\
	        movl $2048, %%ecx;\
	        shr $6, %%ecx;\
\
	loopL1ToMem: \
\
	        movq 0(%4), %%mm1; \
	        movq 8(%4), %%mm2; \
	        movq 16(%4), %%mm3; \
	        movq 24(%4), %%mm4;\
	        movq 32(%4), %%mm5; \
	        movq 40(%4), %%mm6; \
	        movq 48(%4), %%mm7; \
	        movq 56(%4), %%mm0; \
\
	        movntq %%mm1, 0(%%edi); \
	        movntq %%mm2, 8(%%edi); \
	        movntq %%mm3, 16(%%edi); \
	        movntq %%mm4, 24(%%edi); \
	        movntq %%mm5, 32(%%edi); \
	        movntq %%mm6, 40(%%edi); \
	        movntq %%mm7, 48(%%edi); \
	        movntq %%mm0, 56(%%edi); \
\
	        add $64, %4;\
	        add $64, %%edi;\
	        dec %%ecx;\
	        jnz loopL1ToMem ;\
\
                subl $2048, %4;\
	 	dec %0;\
	        jnz loop2k; \
		emms;"
		: "=r"(dum)
		: "S"(src), "D"(dst), "0"(n), "r"(ptbuf)
		: "%ecx", "memory"
		 );

	return dst;
}

CPUTYPE cpu_check()
{

    int family;
    int model;
    int mmxsupport;
    int ssesupport;
    int isIA64;
    unsigned int reax;
    unsigned int redx;

    asm __volatile__ ("\
         movl %%ebx, %%edi;\
         movl $1, %%eax;\
         cpuid;\
         movl %%eax, %0;\
         movl %%edi, %%ebx;"\
        :"=r"(reax)
        :"0"(reax)
	:"%eax", "%edi"
        );

   asm __volatile__ ("\
         movl %%ebx, %%edi;\
         movl $1, %%eax;\
         cpuid;\
         movl %%edx, %0;\
         movl %%edi, %%ebx;"\
        :"=r"(redx)
        :"0"(redx)
	:"%eax", "%edx", "%edi"
      );

    asm __volatile__ (
         "movl $vendor, %%esi;"
	 "movl %%ebx, %%edi;"
         "movl $0, %%eax;"
         "cpuid;"
         "movl %%ebx, 0(%%esi);"
         "movl %%edx, 4(%%esi);"
         "movl %%ecx, 8(%%esi);"
	 "movl %%edi, %%ebx"
        :
        :
	 : "%ecx", "%edx", "%esi", "%edi"
        );

#ifdef DEBUG
    printf("eax=%x\n", reax);
    printf("edx=%x\n", redx);
    printf("vendor = %s\n", vendor);
#endif

    family = (CPUFAMILY & reax) >> 8;
    model  = (MODEL     & reax) >> 4;

    mmxsupport = (redx & MMXSUPPORT);
    ssesupport = (redx & SSESUPPORT);
    isIA64     = (redx & IA64);

#ifdef DEBUG
    printf("mmx support = %s\n", mmxsupport ? "yes" : "no");
    printf("SSE support = %s\n", ssesupport ? "yes" : "no");
#endif

    if(strcmp(vendor, "GenuineIntel") == 0){

//        if(isIA64)
//            return ia64;

        switch(family){
        case 5: // P5
            return p5;
            break;
        case 6: // pro,P2,P3
            if(model == 1) return pro;
            else if(model == 3 || model == 5) return p2;
            else if(model == 6 ) return p2m; //pentium II mobile/celeron
            else if( model==7 || model == 8 ) //celeron/p3
                return p3;
            else
                return unkown;
            break;
        case 0xf: // extended family
            if((reax & 0x0ff00000) == 0 && model == 0) return p4;
            break;
        }
    }
    else if(strcmp(vendor, "AuthenticAMD") == 0){
        if(family == 5){
            if(model < 4)
                return K5;
            else
                return K6;
        }

        else if(family == 6){
            return K7;
        }
        else
            return unkown;
    }
    else {
        if(family == 5)
            return p5;
        else if(family == 6)
            return p6;
    }

    return unkown;
}


static inline int asmcpy_works()
{
        CPUTYPE type = cpu_check();

        if( type == p3 || type == K7 || type == p4)
                return 1;
        else
                return 0;
}

#include "tas-i386.h"
int _x86copy_mutex=0;

//
// n <128     memcpy
// 128>n<2048 MMX copy       
// n >2047    MMX copy with buffer
//
void *armci_asm_memcpy_nofence(void *dst, const void *src, size_t n, int bufid)
{

    int locked=0;
    int residual;
    if(use_asm_copy<0) use_asm_copy = asmcpy_works();
    if(!use_asm_copy || (n<128) ) return memcpy(dst, src, n);
     

  /* memcpy4 has problems in multithreaded environment -- we allow only
   * one thread to use it
   */
  if(n>=2048)locked = testandset((int*)&_x86copy_mutex);

  if(locked){
    residual = (int)n % 64;
    if(residual != 0) memcpy(dst, src, residual);
    if(n>=64)
       asm_memcpy3((char*)dst+residual, (char*)src+residual, n - residual);
  }else {

    residual = (int)n % 2048;

    if(residual != 0){
       int res64 = residual%64;
       if(res64 != 0) memcpy(dst, src, res64);
       if(residual >= 64)
          asm_memcpy3((char*)dst+res64, (char*)src+res64, residual - res64);
    }

    if(n>=2048){
	asm_memcpy4((char*)dst + residual, (char*)src + residual, 
	            n - residual, bufid);
        _x86copy_mutex = 0;
    }
  }

    return dst;
}



void *armci_asm_memcpy(void *dst, const void *src, size_t n, int bufid)
{
void *p;
    if(use_asm_copy<0) use_asm_copy = asmcpy_works();
    if(!use_asm_copy ||n<128) return memcpy(dst, src, n);
     p = armci_asm_memcpy_nofence(dst,src,n,bufid);
     __asm__ __volatile__ ("sfence":::"memory");
     return p;
}


void armci_asm_mem_fence()
{
 __asm__ __volatile__ ("sfence":::"memory");
}
