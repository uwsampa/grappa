#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

// LP64 types

typedef   signed  char  int8 ;
typedef unsigned  char uint8 ;
typedef          short  int16;
typedef unsigned short uint16;
typedef            int  int32;
typedef unsigned   int uint32;
typedef           long  int64;
typedef unsigned  long uint64;

// timing

#include <time.h>

typedef struct {
  double accum_wall, accum_cpus;
  double start_wall, start_cpus;
  time_t init_time;
  char running;
} timer;

void timer_clear     (timer *t);
void timer_start     (timer *t);
void timer_stop      (timer *t);
void timer_report    (timer *t, double *pwall, double *pcpus, int64 print);

// some masking macros

#define _ZERO64       0uL
#define _maskl(x)     (((x) == 0) ? _ZERO64   : ((~_ZERO64) << (64-(x))))
#define _maskr(x)     (((x) == 0) ? _ZERO64   : ((~_ZERO64) >> (64-(x))))
#define _mask(x)      (((x) < 64) ? _maskl(x) : _maskr(2*64 - (x)))

// PRNG

#define _BR_RUNUP_      128L
#define _BR_LG_TABSZ_    7L
#define _BR_TABSZ_      (1L<<_BR_LG_TABSZ_)

typedef struct {
  uint64 hi, lo, ind;
  uint64 tab[_BR_TABSZ_];
} brand_t;

#define _BR_64STEP_(H,L,A,B) {\
  uint64 x;\
  x = H ^ (H << A) ^ (L >> (64-A));\
  H = L | (x >> (B-64));\
  L = x << (128 - B);\
}

uint64 brand         (brand_t *p);
void   brand_init    (brand_t *p, uint64 val);

// upc/shmem functionality

// max size of tab (in wrds)
#ifndef LG_NWRDS
//#define LG_NWRDS  27L
// HACK: can't upc_all_alloc more than 2^24 words /PE
#define LG_NWRDS  27L
#endif
#define NWRDS    (1uL << LG_NWRDS)

#if defined(__UPC__)
#include <upc_relaxed.h>
#include <upc_collective.h>
#define MY_GTHREAD MYTHREAD
#define GTHREADS   THREADS
#define TYPE       shared uint64

#define mpp_init()        // no-op
#define mpp_finalize()    // no-op
#define mpp_barrier_all() upc_barrier
// Change for XT
// #define mpp_exit(status)  upc_global_exit(status)
#define mpp_exit(status)  exit(-1); 

#else
// XT different shmem.h path
// #include <shmem.h>
#include <mpp/shmem.h>
#define MY_GTHREAD shmem_my_pe()
#define GTHREADS   shmem_n_pes()
#define TYPE       uint64

#define mpp_init         start_pes(0)
//#define mpp_init         shmem_init
#define mpp_finalize     
//#define mpp_finalize     shmem_finalize
#define mpp_barrier_all  shmem_barrier_all
#define shmem_put shmem_put64
// Change for XT
// #define mpp_exit(status) globalexit(status)
#define mpp_exit(status)  exit(-1); 

#endif

TYPE *mpp_alloc      (int64 nbytes); 
void  mpp_free       (TYPE *ptr);
int64 mpp_accum_long (int64 val);
void  do_sync_init   (void);
void  mpp_put        (volatile TYPE *dst, uint64 *src, int64 nelem, int64 pe);
void  mpp_broadcast  (TYPE *dst, TYPE *src, int64 nelem, int64 root);
int64 all2all        (TYPE *dst, uint64 *src, int64 len, int64 nwrd);
void  do_all2all     (TYPE *tab, uint64 *loc, brand_t *br, int64 msize,
		      int64 tsize, int64 rep, int64 print);
void  do_warmup      (brand_t *br);
