#ifndef SPINLOCK_H
#define SPINLOCK_H

#if defined(LINUX)

#if defined(__i386__) || defined(__alpha) || defined(__ia64) || defined(__x86_64__)
#  define SPINLOCK 
#  if defined(__GNUC__) && !defined(__INTEL_COMPILER)
#     if defined(__i386__) || defined(__x86_64__) 
#          include "tas-i386.h"
#     elif  defined(__ia64)
#          include "tas-ia64.h"
#     else
#          include "tas-alpha.h"
#     endif
#     define TESTANDSET testandset
#  else
#     define TESTANDSET gcc_testandset
#     define RELEASE_SPINLOCK gcc_clear_spinlock
#  endif
   extern int gcc_testandset();
   extern void gcc_clear_spinlock();
#endif

#endif



#ifdef SPINLOCK

#include <stdio.h>
#include <unistd.h>

#ifndef DBL_PAD
#   define DBL_PAD 16
#endif

/* make sure that locks are not sharing the same cache line */
typedef struct{
double  lock[DBL_PAD];
}pad_lock_t;

#ifndef LOCK_T
#define LOCK_T int
#endif
#define PAD_LOCK_T pad_lock_t

static INLINE void armci_init_spinlock(LOCK_T *mutex)
{
  *mutex =0;
}

static INLINE void armci_acquire_spinlock(LOCK_T *mutex)
{
#ifdef BGML
   return;
#else
int loop=0, maxloop =10;
   while (TESTANDSET(mutex)){
      loop++;
      if(loop==maxloop){ 
#if 0
         extern int armci_me;
         printf("%d:spinlock sleeping\n",armci_me); fflush(stdout);
#endif
         usleep(1);
         loop=0;
      }
  }
#endif
}



#ifdef  RELEASE_SPINLOCK
#ifdef MEMORY_BARRIER
#  define  armci_release_spinlock(x) MEMORY_BARRIER ();RELEASE_SPINLOCK(x)
#else
#  define  armci_release_spinlock(x) RELEASE_SPINLOCK(x)
#endif
#else
static INLINE void armci_release_spinlock(LOCK_T *mutex)
{
#ifdef BGML
   return;
#else
#ifdef MEMORY_BARRIER
  MEMORY_BARRIER ();
#endif
  *mutex =0;
#if (defined(MACX)||defined(LINUX)) && defined(__GNUC__) && defined(__ppc__)  
  __asm__ __volatile__ ("isync" : : : "memory");
#endif
#endif
}

#endif

#endif




#endif/*SPINLOCK_H*/
