#ifndef _QTHREAD_H_
#define _QTHREAD_H_

#include <errno.h>		       /* for ENOMEM */

#include <qthread/qthread-int.h>       /* for uint32_t and uint64_t */
#include <qthread/common.h>	       /* important configuration options */

#include <string.h>		       /* for memcpy() */

#ifdef QTHREAD_NEEDS_IA64INTRIN
# ifdef HAVE_IA64INTRIN_H
#  include <ia64intrin.h>
# elif defined(HAVE_IA32INTRIN_H)
#  include <ia32intrin.h>
# endif
#endif

/*****************************************************************************
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  NOTE  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! *
 *                                                                           *
 *    The most complete documentaton is going to be in the man pages. The    *
 *    documentation here is just to give you a general idea of what each     *
 *    function does.                                                         *
 *                                                                           *
 *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *****************************************************************************/

/* Return Codes */
#define QTHREAD_REDUNDANT	1
#define QTHREAD_SUCCESS		0
#define QTHREAD_BADARGS		-1
#define QTHREAD_PTHREAD_ERROR	-2
#define QTHREAD_NOT_ALLOWED     -3
#define QTHREAD_MALLOC_ERROR	ENOMEM
#define QTHREAD_THIRD_PARTY_ERROR -4
#define QTHREAD_TIMEOUT		-5     /* neither ETIME nor ETIMEDOUT seem appropriate, strictly speaking */
#ifdef EOVERFLOW
# define QTHREAD_OVERFLOW	EOVERFLOW
#else
# define QTHREAD_OVERFLOW	-6
#endif
#define NO_SHEPHERD ((qthread_shepherd_id_t)-1)

#ifdef __cplusplus
#define Q_STARTCXX extern "C" {
#define Q_ENDCXX }
#else
#define Q_STARTCXX
#define Q_ENDCXX
#endif

#ifdef QTHREAD_ALIGNEDDATA_ALLOWED
# define Q_ALIGNED(x) __attribute__((aligned(x)))
#endif

Q_STARTCXX /* */
/* NOTE!!!!!!!!!!!
 * Reads and writes operate on aligned_t-size segments of memory.
 *
 * FEB locking only works on aligned addresses. On 32-bit architectures, this
 * isn't too much of an inconvenience. On 64-bit architectures, it's a pain in
 * the BUTT! This is here to try and help a little bit. */
#if QTHREAD_SIZEOF_ALIGNED_T == 4
typedef uint32_t Q_ALIGNED(QTHREAD_ALIGNMENT_ALIGNED_T) aligned_t;
typedef int32_t Q_ALIGNED(QTHREAD_ALIGNMENT_ALIGNED_T) saligned_t;
#elif QTHREAD_SIZEOF_ALIGNED_T == 8
typedef uint64_t Q_ALIGNED(QTHREAD_ALIGNMENT_ALIGNED_T) aligned_t;
typedef int64_t Q_ALIGNED(QTHREAD_ALIGNMENT_ALIGNED_T) saligned_t;
#else
#error "Don't know type for sizeof aligned_t"
#endif

typedef struct _syncvar_s {
    volatile union {
	volatile uint64_t w;
	volatile struct {
#ifdef BITFIELD_ORDER_FORWARD
	    volatile uint64_t data : 60;
	    volatile unsigned state : 3;
	    volatile unsigned lock : 1;
#else
	    volatile unsigned lock : 1;
	    volatile unsigned state : 3;
	    volatile uint64_t data : 60;
#endif
	} s;
    } u;
} syncvar_t;
#define SYNCVAR_STATIC_INITIALIZER { { 0 } }
Q_ENDCXX /* */

#ifdef QTHREAD_SST_PRIMITIVES
# include <qthread/qthread-sst.h>
#else

Q_STARTCXX /* */
typedef struct qthread_s qthread_t;
typedef unsigned int qthread_shepherd_id_t;

/* for convenient arguments to qthread_fork */
typedef aligned_t(*qthread_f) (qthread_t * me, void *arg);

/* use this function to initialize the qthreads environment before spawning any
 * qthreads. The argument to this function used to specify the number of
 * pthreads that will be spawned to shepherd the qthreads. This number is now
 * ignored, the qthread_init() function is deprecated, and qthread_initialize()
 * now takes its place. If you MUST specify the number of shepherds, use the
 * environment variable QTHREAD_NUM_SHEPHERDS. */
int qthread_init(qthread_shepherd_id_t nshepherds);
int qthread_initialize(void);

/* use this function to clean up the qthreads environment after execution of
 * the program is finished. This function will terminate any currently running
 * qthreads, so only use it when you are certain that execution has completed.
 * This function is automatically called when the program exits, so only use if
 * reclaiming resources from the library is necessary before the program exits.
 */
void qthread_finalize(void);

/* use this function to tell a shepherd to stop accepting new threads and to
 * offload its existing threads to nearby shepherds. This latter may not take
 * effect immediately, but may only take effect when the current executing
 * qthread on that shepherd next stops executing */
int qthread_disable_shepherd(const qthread_shepherd_id_t shep);
void qthread_enable_shepherd(const qthread_shepherd_id_t shep);

/* this function allows a qthread to specifically give up control of the
 * processor even though it has not blocked. This is useful for things like
 * busy-waits or cooperative multitasking. Without this function, threads will
 * only ever allow other threads assigned to the same pthread to execute when
 * they block. */
void qthread_yield(qthread_t * me);

/* this function allows a qthread to retrieve its qthread_t pointer if it has
 * been lost for some reason */
qthread_t *qthread_self(void);

/* these are the functions for generating a new qthread.
 *
 * Using qthread_fork() and variants:
 *
 *     The specified function (the first argument; note that it is a qthread_f
 *     and not a qthread_t) will be run to completion. You can detect that a
 *     thread has finished by specifying a location to store the return value
 *     (which will be stored with a qthread_writeF call). The qthread_fork_to
 *     function spawns the thread to a specific shepherd.
 */
int qthread_fork(const qthread_f f, const void *const arg, aligned_t * ret);
int qthread_fork_syncvar(const qthread_f f, const void *const arg, syncvar_t * ret);
int qthread_fork_to(const qthread_f f, const void *const arg, aligned_t * ret,
		    const qthread_shepherd_id_t shepherd);
int qthread_fork_syncvar_to(const qthread_f f, const void *const arg, syncvar_t * ret,
		    const qthread_shepherd_id_t shepherd);

/* Using qthread_prepare()/qthread_schedule() and variants:
 *
 *     The combination of these two functions works like qthread_fork().
 *     First, qthread_prepare() creates a qthread_t object that is ready to be
 *     run (almost), but has not been scheduled. Next, qthread_schedule puts
 *     the finishing touches on the qthread_t structure and places it into an
 *     active queue.
 */
qthread_t *qthread_prepare(const qthread_f f, const void *const arg,
			   aligned_t * ret);
qthread_t *qthread_prepare_for(const qthread_f f, const void *const arg,
			       aligned_t * ret,
			       const qthread_shepherd_id_t shepherd);

int qthread_schedule(qthread_t * t);
int qthread_schedule_on(qthread_t * t, const qthread_shepherd_id_t shepherd);

/* This is a function to move a thread from one shepherd to another. */
int qthread_migrate_to(qthread_t * me, const qthread_shepherd_id_t shepherd);

/* This function sets the debug level if debugging has been enabled */
int qthread_debuglevel(int);

/* these are accessor functions for use by the qthreads to retrieve information
 * about themselves */
unsigned qthread_id(const qthread_t * t);
qthread_shepherd_id_t qthread_shep(const qthread_t * t);
size_t qthread_stackleft(const qthread_t * t);
aligned_t *qthread_retloc(const qthread_t * t);
int qthread_shep_ok(const qthread_t * t);

/* returns the distance from one shepherd to another */
int qthread_distance(const qthread_shepherd_id_t src,
		     const qthread_shepherd_id_t dest);
/* returns a list of shepherds, sorted by their distance from either this
 * qthread or the specified shepherd */
const qthread_shepherd_id_t *qthread_sorted_sheps(const qthread_t * t);
const qthread_shepherd_id_t *qthread_sorted_sheps_remote(const
							 qthread_shepherd_id_t
							 src);
/* returns the number of shepherds (i.e. one more than the largest valid shepherd id) */
qthread_shepherd_id_t qthread_num_shepherds(void);

/****************************************************************************
 * functions to implement FEB locking/unlocking
 ****************************************************************************
 *
 * These are the FEB functions. All but empty/fill have the potential of
 * blocking until the corresponding precondition is met. All FEB
 * blocking/reading/writing is done on a machine-word basis. Memory is assumed
 * to be full unless otherwise asserted, and as such memory that is full and
 * does not have dependencies (i.e. no threads are waiting for it to become
 * empty) does not require state data to be stored. It is expected that while
 * there may be locks instantiated at one time or another for a very large
 * number of addresses in the system, relatively few will be in a non-default
 * (full, no waiters) state at any one time.
 */

/* This function is just to assist with debugging; it returns 1 if the address
 * is full, and 0 if the address is empty */
int qthread_feb_status(const aligned_t * addr);
int qthread_syncvar_status(syncvar_t *const v);

/* The empty/fill functions merely assert the empty or full state of the given
 * address. You may be wondering why they require a qthread_t argument. The
 * reason for this is memory pooling; memory is allocated on a per-shepherd
 * basis (to avoid needing to lock the memory pool). Anyway, if you pass it a
 * NULL qthread_t, it will still work, it just won't be as fast. */
int qthread_empty(qthread_t * me, const aligned_t * dest);
int qthread_syncvar_empty(qthread_t * restrict const me, syncvar_t * restrict const dest);
int qthread_fill(qthread_t * me, const aligned_t * dest);
int qthread_syncvar_fill(qthread_t * restrict const me, syncvar_t * restrict const dest);

/* These functions wait for memory to become empty, and then fill it. When
 * memory becomes empty, only one thread blocked like this will be awoken. Data
 * is read from src and written to dest.
 *
 * The semantics of writeEF are:
 * 1 - destination's FEB state must be "empty"
 * 2 - data is copied from src to destination
 * 3 - the destination's FEB state gets changed from empty to full
 *
 * This function takes a qthread_t pointer as an argument. If this is called
 * from somewhere other than a qthread, use NULL for the me argument. If you
 * have lost your qthread_t pointer, it can be reclaimed using qthread_self()
 * (which, conveniently, returns NULL if you aren't a qthread).
 */
int qthread_writeEF(qthread_t * me, aligned_t * restrict const dest,
		    const aligned_t * restrict const src);
int qthread_writeEF_const(qthread_t * me, aligned_t * const dest,
			  const aligned_t src);
int qthread_syncvar_writeEF(qthread_t *restrict me, syncvar_t *restrict
			    const dest, const uint64_t *restrict const src);
int qthread_syncvar_writeEF_const(qthread_t *restrict me, syncvar_t *restrict
			    const dest, const uint64_t src);

/* This function is a cross between qthread_fill() and qthread_writeEF(). It
 * does not wait for memory to become empty, but performs the write and sets
 * the state to full atomically with respect to other FEB-based actions. Data
 * is read from src and written to dest.
 *
 * The semantics of writeF are:
 * 1 - data is copied from src to destination
 * 2 - the destination's FEB state gets set to full
 *
 * This function takes a qthread_t pointer as an argument. If this is called
 * from somewhere other than a qthread, use NULL for the me argument. If you
 * have lost your qthread_t pointer, it can be reclaimed using qthread_self()
 * (which, conveniently, returns NULL if you aren't a qthread).
 */
int qthread_writeF(qthread_t * me, aligned_t * restrict const dest,
		   const aligned_t * restrict const src);
int qthread_writeF_const(qthread_t * me, aligned_t * const dest,
			 const aligned_t src);
int qthread_syncvar_writeF(qthread_t *restrict me, syncvar_t *restrict
			   const dest, const uint64_t *restrict const src);
int qthread_syncvar_writeF_const(qthread_t *restrict me, syncvar_t *restrict
			   const dest, const uint64_t src);

/* This function waits for memory to become full, and then reads it and leaves
 * the memory as full. When memory becomes full, all threads waiting for it to
 * become full with a readFF will receive the value at once and will be queued
 * to run. Data is read from src and stored in dest.
 *
 * The semantics of readFF are:
 * 1 - src's FEB state must be "full"
 * 2 - data is copied from src to destination
 *
 * This function takes a qthread_t pointer as an argument. If this is called
 * from somewhere other than a qthread, use NULL for the me argument. If you
 * have lost your qthread_t pointer, it can be reclaimed using qthread_self()
 * (which, conveniently, returns NULL if you aren't a qthread).
 */
int qthread_readFF(qthread_t * me, aligned_t * const dest,
		   const aligned_t * const src);
int qthread_syncvar_readFF(qthread_t * restrict const me, uint64_t * restrict const dest,
		   syncvar_t * restrict const src);

/* These functions wait for memory to become full, and then empty it. When
 * memory becomes full, only one thread blocked like this will be awoken. Data
 * is read from src and written to dest.
 *
 * The semantics of readFE are:
 * 1 - src's FEB state must be "full"
 * 2 - data is copied from src to destination
 * 3 - the src's FEB bits get changed from full to empty when the data is copied
 *
 * This function takes a qthread_t pointer as an argument. If this is called
 * from somewhere other than a qthread, use NULL for the me argument. If you
 * have lost your qthread_t pointer, it can be reclaimed using qthread_self()
 * (which, conveniently, returns NULL if you aren't a qthread).
 */
int qthread_readFE(qthread_t * me, aligned_t * const dest,
		   const aligned_t * const src);
int qthread_syncvar_readFE(qthread_t * restrict const me, uint64_t * restrict const dest,
		   syncvar_t * restrict const src);

/* functions to implement FEB-ish locking/unlocking
 *
 * These are atomic and functional, but do not have the same semantics as full
 * FEB locking/unlocking (namely, unlocking cannot block), however because of
 * this, they have lower overhead.
 *
 * These functions take a qthread_t pointer as an argument. If this is called
 * from somewhere other than a qthread, use NULL for the me argument. If you
 * have lost your qthread_t pointer, it can be reclaimed using qthread_self().
 */
int qthread_lock(qthread_t * me, const aligned_t * a);
int qthread_unlock(qthread_t * me, const aligned_t * a);

#if defined(QTHREAD_MUTEX_INCREMENT) || (QTHREAD_ASSEMBLY_ARCH == QTHREAD_POWERPC32)
uint32_t qthread_incr32_(volatile uint32_t *, const int32_t);
uint64_t qthread_incr64_(volatile uint64_t *, const int64_t);
float qthread_fincr_(volatile float *, const float);
double qthread_dincr_(volatile double *, const double);
uint32_t qthread_cas32_(volatile uint32_t *, const uint32_t, const uint32_t);
uint64_t qthread_cas64_(volatile uint64_t *, const uint64_t, const uint64_t);
#endif

/* the following three functions implement variations on atomic increment. It
 * is done with architecture-specific assembly (on supported architectures,
 * when possible) and does NOT use FEB's or lock/unlock unless the architecture
 * is unsupported or cannot perform atomic operations at the right granularity.
 * All of these functions return the value of the contents of the operand
 * *after* incrementing.
 */
static QINLINE float qthread_fincr(volatile float *operand, const float incr)
{				       /*{{{ */
#if defined (QTHREAD_MUTEX_INCREMENT)
    return qthread_fincr_(operand,incr);
#else
# if (QTHREAD_ASSEMBLY_ARCH == QTHREAD_POWERPC32) || (QTHREAD_ASSEMBLY_ARCH == QTHREAD_POWERPC64)
    union
    {
	float f;
	uint32_t i;
    } retval;
    register float incremented_value;
    register uint32_t scratch_int;
    uint32_t conversion_memory = conversion_memory;
    __asm__ __volatile__("1:\n\t"
	    "lwarx  %0,0,%4\n\t"
	    // convert from int to float
	    "stw    %0,%2\n\t"
	    "lfs    %1,%2\n\t"
	    // do the addition
	    "fadds  %1,%1,%5\n\t"
	    // convert from float to int
	    "stfs   %1,%2\n\t"
	    "lwz    %3,%2\n\t"
	    // store back to original location
	    "stwcx. %3,0,%4\n\t"
	    "bne-   1b\n\t"
	    "isync"
	    :"=&b" (retval.i),		/* %0 */
	     "=&f" (incremented_value),	/* %1 */
	     "=m"  (conversion_memory),	/* %2 */
	     "=&r" (scratch_int)	/* %3 */
	    :"r"   (operand),		/* %4 */
	     "f"   (incr)		/* %5 */
	    :"cc", "memory");

    return retval.f;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_SPARCV9_64) || (QTHREAD_ASSEMBLY_ARCH == QTHREAD_SPARCV9_32)
    union
    {
	float f;
	uint32_t i;
    } oldval, newval;

    /* newval.f = *operand; */
    do {
	/* you *should* be able to move the *operand reference outside the
	 * loop and use the output of the CAS (namely, newval) instead.
	 * However, there seems to be a bug in gcc 4.0.4 wherein, if you do
	 * that, the while() comparison uses a temporary register value for
	 * newval that has nothing to do with the output of the CAS
	 * instruction. (See how obviously wrong that is?) For some reason that
	 * I haven't been able to figure out, moving the *operand reference
	 * inside the loop fixes that problem, even at -O2 optimization. */
	oldval.f = *operand;
	newval.f = oldval.f + incr;
	__asm__ __volatile__
	        ("membar #StoreStore|#LoadStore|#StoreLoad|#LoadLoad\n\t"
		 "cas [%1], %2, %0"
		 :"+r"(newval.i)
		 :"r"    (operand), "r"(oldval.i)
		 :"cc", "memory");
    } while (oldval.i != newval.i);
    return oldval.f;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_IA64)
    union
    {
	float f;
	uint32_t i;
    } oldval, newval, res;

    do {
	oldval.f = *operand;
	newval.f = oldval.f + incr;
	__asm__ __volatile__("mov ar.ccv=%0;;"::"rO"(oldval.i));
	__asm__ __volatile__("cmpxchg4.acq %0=[%1],%2,ar.ccv"
			     :"=r"(res.i)
			     :"r"    (operand), "r"(newval.i)
			     :"memory");
    } while (res.i != oldval.i);       /* if res!=old, the calc is out of date */
    return oldval.f;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_AMD64) || (QTHREAD_ASSEMBLY_ARCH == QTHREAD_IA32)
    union
    {
	float f;
	uint32_t i;
    } oldval, newval, retval;

    do {
	oldval.f = *operand;
	newval.f = oldval.f + incr;
	__asm__ __volatile__("lock; cmpxchg %1, (%2)"
			     :"=a"(retval.i)	/* store from EAX */
			     :"r"    (newval.i),
			      "r"(operand),
			      "0"(oldval.i)	/* load into EAX */
			     :"cc", "memory");
    } while (retval.i != oldval.i);
    return oldval.f;
# else
# error Unsupported assembly architecture for qthread_fincr
# endif
#endif
}				       /*}}} */

static QINLINE double qthread_dincr(volatile double *operand,
				    const double incr)
{				       /*{{{ */
#if defined(QTHREAD_MUTEX_INCREMENT) || (QTHREAD_ASSEMBLY_ARCH == QTHREAD_POWERPC32)
    return qthread_dincr_(operand, incr);
#else
# if (QTHREAD_ASSEMBLY_ARCH == QTHREAD_POWERPC64)
    register uint64_t scratch_int;
    register double incremented_value;
    union
    {
	uint64_t i;
	double d;
    } retval;
    uint64_t conversion_memory = conversion_memory;
    __asm__ __volatile__("1:\n\t"
			 "ldarx %0,0,%4\n\t"
			 /*convert from integer to floating point */
			 "std   %0,%2\n\t"	// %2 is scratch memory (NOT a register)
			 "lfd   %1,%2\n\t"	// %1 is a scratch floating point register
			 /* do the addition */
			 "fadd  %1,%1,%5\n\t"	// %4 is the input increment
			 /* convert from floating point to integer */
			 "stfd   %1,%2\n\t"
			 "ld     %3,%2\n\t"
			 /* store back to original location */
			 "stdcx. %3,0,%4\n\t"
			 "bne-   1b\n\t"
			 "isync"
			 :"=&b" (retval.i),		/* %0 */
			  "=&f" (incremented_value),	/* %1 */
			  "=m"  (conversion_memory),	/* %2 */
			  "=r&" (scratch_int)		/* %3 */
			 :"r"   (operand),		/* %4 */
			  "f"   (incr)			/* %5 */
			 :"cc", "memory");

    return retval.d;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_SPARCV9_32)
    double oldval, newval;

    newval = *operand;
    do {
	/* this allows the compiler to be as flexible as possible with register
	 * assignments */
	register uint64_t tmp1 = tmp1;
	register uint64_t tmp2 = tmp2;

	oldval = newval;
	newval = oldval + incr;
	__asm__ __volatile__("ldx %0, %1\n\t"
			     "ldx %4, %2\n\t"
			     "membar #StoreStore|#LoadStore|#StoreLoad|#LoadLoad\n\t"
			     "casx [%3], %2, %1\n\t"
			     "stx %1, %0"
			     /* h means 64-BIT REGISTER
			      * (probably unnecessary, but why take chances?) */
			     :"=m"   (newval), "=&h"(tmp1), "=&h"(tmp2)
			     :"r"    (operand), "m"(oldval)
			     :"memory");
    } while (oldval != newval);
    return oldval;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_SPARCV9_64)
    union
    {
	uint64_t i;
	double d;
    } oldval, newval;

    /*newval.d = *operand; */
    do {
	/* you *should* be able to move the *operand reference outside the
	 * loop and use the output of the CAS (namely, newval) instead.
	 * However, there seems to be a bug in gcc 4.0.4 wherein, if you do
	 * that, the while() comparison uses a temporary register value for
	 * newval that has nothing to do with the output of the CAS
	 * instruction. (See how obviously wrong that is?) For some reason that
	 * I haven't been able to figure out, moving the *operand reference
	 * inside the loop fixes that problem, even at -O2 optimization. */
	oldval.d = *operand;
	newval.d = oldval.d + incr;
	__asm__ __volatile__
	        ("membar #StoreStore|#LoadStore|#StoreLoad|#LoadLoad\n\t"
		 "casx [%1], %2, %0"
		 :"+r"(newval.i)
		 :"r"(operand), "r"(oldval.i)
		 :"memory");
    } while (oldval.d != newval.d);
    return oldval.d;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_IA64)
    union
    {
	uint64_t i;
	double d;
    } oldval, newval, res;

    do {
	oldval.d = *operand;
	newval.d = oldval.d + incr;
	__asm__ __volatile__("mov ar.ccv=%0;;"::"rO"(oldval.i));
	__asm__ __volatile__("cmpxchg8.acq %0=[%1],%2,ar.ccv"
			     :"=r"(res.i)
			     :"r"    (operand), "r"(newval.i)
			     :"memory");
    } while (res.i != oldval.i);       /* if res!=old, the calc is out of date */
    return oldval.d;

# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_AMD64)
    union
    {
	double d;
	uint64_t i;
    } oldval, newval, retval;

    do {
	oldval.d = *operand;
	newval.d = oldval.d + incr;
#  ifdef __PGI
	__asm__ __volatile__("lock; cmpxchgq %1, (%2)\n\t"
		"mov %%rax,(%0)"
			     ::"r"(&retval.i),
			      "r"(newval.i), "r"(operand),
			      "a"(oldval.i)
			     :"memory");
#  else
	__asm__ __volatile__("lock; cmpxchgq %1, (%2)"
			     :"=a"(retval.i)
			     :"r"(newval.i), "r"(operand),
			      "0"(oldval.i)
			     :"memory");
#  endif
    } while (retval.i != oldval.i);
    return oldval.d;

# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_IA32)
    union
    {
	double d;
	uint64_t i;
	struct
	{
	    /* note: the ordering of these is both important and
	     * counter-intuitive; welcome to little-endian! */
	    uint32_t l;
	    uint32_t h;
	} s;
    } oldval, newval;
    register char test;

    do {
#ifdef __PIC__
       /* this saves off %ebx to make PIC code happy :P */
# define QTHREAD_PIC_PREFIX "xchg %%ebx, %4\n\t"
       /* this restores it */
# define QTHREAD_PIC_SUFFIX "\n\txchg %%ebx, %4"
# define QTHREAD_PIC_REG_4 "r"
#else
# define QTHREAD_PIC_PREFIX
# define QTHREAD_PIC_SUFFIX
# define QTHREAD_PIC_REG_4 "b"
#endif
	oldval.d = *operand;
	newval.d = oldval.d + incr;
	/* Yeah, this is weird looking, but it really makes sense when you
	 * understand the instruction's semantics (which make sense when you
	 * consider that it's doing a 64-bit op on a 32-bit proc):
	 *
	 *    Compares the 64-bit value in EDX:EAX with the operand
	 *    (destination operand). If the values are equal, the 64-bit value
	 *    in ECX:EBX is stored in the destination operand. Otherwise, the
	 *    value in the destination operand is loaded into EDX:EAX."
	 *
	 * So what happens is the oldval is loaded into EDX:EAX and the newval
	 * is loaded into ECX:EBX to start with (i.e. as inputs). Then
	 * CMPXCHG8B does its business, after which EDX:EAX is guaranteed to
	 * contain the value of *operand when the instruction executed. We test
	 * the ZF field to see if the operation succeeded. We *COULD* save
	 * EDX:EAX back into oldval to save ourselves a step when the loop
	 * fails, but that's a waste when the loop succeeds (i.e. in the common
	 * case). Optimizing for the common case, in this situation, means
	 * minimizing our extra write-out to the one-byte test variable.
	 */
	__asm__ __volatile__(QTHREAD_PIC_PREFIX
			     "lock; cmpxchg8b (%1)\n\t"
			     "setne %0"	/* test = (ZF==0) */
			     QTHREAD_PIC_SUFFIX
			     :"=q"(test)
			     :"r"(operand),
			     /*EAX*/ "a"(oldval.s.l),
			     /*EDX*/ "d"(oldval.s.h),
			     /*EBX*/ QTHREAD_PIC_REG_4(newval.s.l),
			     /*ECX*/ "c"(newval.s.h)
			     :"memory");
    } while (test);		       /* if ZF was cleared, the calculation is out of date */
    return oldval.d;

# else
#  error Unimplemented assembly architecture for qthread_dincr
# endif
#endif
}				       /*}}} */

static QINLINE uint32_t qthread_incr32(volatile uint32_t * operand,
				       const uint32_t incr)
{				       /*{{{ */
#ifdef QTHREAD_MUTEX_INCREMENT
    return qthread_incr32_(operand,incr);
#else
# if (QTHREAD_ASSEMBLY_ARCH == QTHREAD_POWERPC32) || \
     (QTHREAD_ASSEMBLY_ARCH == QTHREAD_POWERPC64)
    uint32_t retval;
    register unsigned int incrd = incrd;	/* no initializing */
    __asm__ __volatile__("1:\tlwarx  %0,0,%2\n\t"
			 "add    %1,%0,%3\n\t"
			 "stwcx. %1,0,%2\n\t"
			 "bne-   1b\n\t"	/* if it failed, try again */
			 "isync"	/* make sure it wasn't all a dream */
			 :"=&b"  (retval), "=&r"(incrd)
			 :"r"    (operand), "r"(incr)
			 :"cc", "memory");

    return retval;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_SPARCV9_32) || \
       (QTHREAD_ASSEMBLY_ARCH == QTHREAD_SPARCV9_64)
    register uint32_t oldval, newval;

    /* newval = *operand; */
    do {
	/* you *should* be able to move the *operand reference outside the
	 * loop and use the output of the CAS (namely, newval) instead.
	 * However, there seems to be a bug in gcc 4.0.4 wherein, if you do
	 * that, the while() comparison uses a temporary register value for
	 * newval that has nothing to do with the output of the CAS
	 * instruction. (See how obviously wrong that is?) For some reason that
	 * I haven't been able to figure out, moving the *operand reference
	 * inside the loop fixes that problem, even at -O2 optimization. */
	oldval = *operand;
	newval = oldval + incr;
	/* newval always gets the value of *operand; if it's
	 * the same as oldval, then the swap was successful */
	__asm__ __volatile__
	        ("membar #StoreStore|#LoadStore|#StoreLoad|#LoadLoad\n\t"
		 "cas [%1] , %2, %0"
		 :"+r"  (newval)
		 :"r"    (operand), "r"(oldval)
		 :"cc", "memory");
    } while (oldval != newval);
    return oldval;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_IA64)
    uint32_t res;

    if (incr == 1) {
	asm volatile ("fetchadd4.rel %0=[%1],1"
		      :"=r" (res)
		      :"r"  (operand));
    } else {
	uint32_t old, newval;

	do {
	    old = *operand;	       /* atomic, because operand is aligned */
	    newval = old + incr;
	    asm volatile ("mov ar.ccv=%0;;":	/* no output */
			  :"rO"    (old));

	    /* separate so the compiler can insert its junk */
	    asm volatile ("cmpxchg4.acq %0=[%1],%2,ar.ccv"
			  :"=r"(res)
			  :"r" (operand), "r"(newval)
			  :"memory");
	} while (res != old);	       /* if res!=old, the calc is out of date */
    }
    return res;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_IA32) || \
      (QTHREAD_ASSEMBLY_ARCH == QTHREAD_AMD64)

    uint32_t retval = incr;
    __asm__ __volatile__ ("lock ;  xaddl %0, (%1);"
		  :"+r" (retval)
		  :"r"  (operand)
		  :"memory");

    return retval;
# else
#  error Unimplemented assembly architecture for qthread_incr32
# endif
#endif
}				       /*}}} */

static QINLINE uint64_t qthread_incr64(volatile uint64_t * operand,
				       const uint64_t incr)
{				       /*{{{ */
#if defined(QTHREAD_MUTEX_INCREMENT) || \
    (QTHREAD_ASSEMBLY_ARCH == QTHREAD_POWERPC32) || \
    (QTHREAD_ASSEMBLY_ARCH == QTHREAD_SPARCV9_32)
    return qthread_incr64_(operand,incr);
#else
# if (QTHREAD_ASSEMBLY_ARCH == QTHREAD_POWERPC64)
    uint64_t retval;
    register uint64_t incrd = incrd;	/* no initializing */

    asm volatile ("1:\tldarx  %0,0,%2\n\t"
		  "add    %1,%0,%3\n\t"
		  "stdcx. %1,0,%2\n\t"
		  "bne-   1b\n\t"	/* if it failed, try again */
		  "isync"	/* make sure it wasn't all a dream */
		  :"=&b"   (retval), "=&r"(incrd)
		  :"r"     (operand), "r"(incr)
		  :"cc", "memory");

    return retval;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_SPARCV9_32)
    uint64_t oldval, newval = *operand;

    do {
	/* this allows the compiler to be as flexible as possible with register
	 * assignments */
	register uint64_t tmp1 = tmp1;
	register uint64_t tmp2 = tmp2;

	oldval = newval;
	newval += incr;
	/* newval always gets the value of *operand; if it's
	 * the same as oldval, then the swap was successful */
	__asm__ __volatile__("ldx %0, %1\n\t"
			     "ldx %4, %2\n\t"
			     "membar #StoreStore|#LoadStore|#StoreLoad|#LoadLoad\n\t"
			     "casx [%3] , %2, %1\n\t"
			     "stx %1, %0"
			     /* h means 64-BIT REGISTER
			      * (probably unnecessary, but why take chances?) */
			     :"=m"   (newval), "=&h"(tmp1), "=&h"(tmp2)
			     :"r"    (operand), "m"(oldval)
			     :"cc", "memory");
    } while (oldval != newval);
    return oldval;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_SPARCV9_64)
    register uint64_t oldval, newval;

#  ifdef QTHREAD_ATOMIC_CAS
    newval = *operand;
    do {
	oldval = newval;
	newval = __sync_val_compare_and_swap(operand, oldval, oldval + incr);
    } while (oldval != newval);
#  else
    do {
	/* you *should* be able to move the *operand reference outside the
	 * loop and use the output of the CAS (namely, newval) instead.
	 * However, there seems to be a bug in gcc 4.0.4 wherein, if you do
	 * that, the while() comparison uses a temporary register value for
	 * newval that has nothing to do with the output of the CAS
	 * instruction. (See how obviously wrong that is?) For some reason that
	 * I haven't been able to figure out, moving the *operand reference
	 * inside the loop fixes that problem, even at -O2 optimization. */
	oldval = *operand;
	newval = oldval + incr;
	/* newval always gets the value of *operand; if it's
	 * the same as oldval, then the swap was successful */
	__asm__ __volatile__
	        ("membar #StoreStore|#LoadStore|#StoreLoad|#LoadLoad\n\t"
		 "casx [%1] , %2, %0"
		 :"+r"(newval)
		 :"r"    (operand), "r"(oldval)
		 :"cc", "memory");
    } while (oldval != newval);
#  endif
    return oldval;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_IA64)
    uint64_t res;

    if (incr == 1) {
	asm volatile ("fetchadd8.rel %0=%1,1"
		      :"=r" (res)
		      :"m"     (*operand));
    } else {
	uint64_t old, newval;

	do {
	    old = *operand;	       /* atomic, because operand is aligned */
	    newval = old + incr;
	    asm volatile ("mov ar.ccv=%0;;":	/* no output */
			  :"rO"    (old));

	    /* separate so the compiler can insert its junk */
	    asm volatile ("cmpxchg8.acq %0=[%1],%2,ar.ccv"
			  :"=r" (res)
			  :"r"     (operand), "r"(newval)
			  :"memory");
	} while (res != old);	       /* if res!=old, the calc is out of date */
    }
    return res;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_IA32)
    union
    {
	uint64_t i;
	struct
	{
	    /* note: the ordering of these is both important and
	     * counter-intuitive; welcome to little-endian! */
	    uint32_t l;
	    uint32_t h;
	} s;
    } oldval, newval;
    register char test;

    do {
#ifndef QTHREAD_PIC_PREFIX
# ifdef __PIC__
	/* should share this code with the dincr stuff */
	/* this saves off %ebx to make PIC code happy :P */
#  define QTHREAD_PIC_PREFIX "xchg %%ebx, %4\n\t"
	/* this restores it */
#  define QTHREAD_PIC_SUFFIX "\n\txchg %%ebx, %4"
#  define QTHREAD_PIC_REG_4 "r"
# else
#  define QTHREAD_PIC_PREFIX
#  define QTHREAD_PIC_SUFFIX
#  define QTHREAD_PIC_REG_4 "b"
# endif
#endif
	oldval.i = *operand;
	newval.i = oldval.i + incr;
	/* Yeah, this is weird looking, but it really makes sense when you
	 * understand the instruction's semantics (which make sense when you
	 * consider that it's doing a 64-bit op on a 32-bit proc):
	 *
	 *    Compares the 64-bit value in EDX:EAX with the operand
	 *    (destination operand). If the values are equal, the 64-bit value
	 *    in ECX:EBX is stored in the destination operand. Otherwise, the
	 *    value in the destination operand is loaded into EDX:EAX."
	 *
	 * So what happens is the oldval is loaded into EDX:EAX and the newval
	 * is loaded into ECX:EBX to start with (i.e. as inputs). Then
	 * CMPXCHG8B does its business, after which EDX:EAX is guaranteed to
	 * contain the value of *operand when the instruction executed. We test
	 * the ZF field to see if the operation succeeded. We *COULD* save
	 * EDX:EAX back into oldval to save ourselves a step when the loop
	 * fails, but that's a waste when the loop succeeds (i.e. in the common
	 * case). Optimizing for the common case, in this situation, means
	 * minimizing our extra write-out to the one-byte test variable.
	 */
	__asm__ __volatile__(QTHREAD_PIC_PREFIX
			     "lock; cmpxchg8b (%1)\n\t"
			     "setne %0"	/* test = (ZF==0) */
			     QTHREAD_PIC_SUFFIX
			     :"=q"(test)
			     :"r"    (operand),
			     /*EAX*/ "a"(oldval.s.l),
			     /*EDX*/ "d"(oldval.s.h),
			     /*EBX*/ QTHREAD_PIC_REG_4(newval.s.l),
			     /*ECX*/ "c"(newval.s.h)
			     :"memory");
    } while (test);		       /* if ZF was cleared, the calculation is out of date */
    return oldval.i;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_AMD64)
    uint64_t retval = incr;

#  ifdef __PGI
    /* this is a workaround for a bug in the PGI compiler where the width of
     * retval is not respected and %eax is used instead of %rax */
    __asm__ __volatile__ ("lock xaddq %0, (%2)\n\t"
	    "mov %0,(%1)"
		  ::"r" (incr),
		  "r"(&retval),
		  "r" (operand)
		  :"memory");
#  else
    __asm__ __volatile__ ("lock ; xaddq %0, (%1);"
		  :"+r" (retval)
		  :"r" (operand)
		  :"memory");
#  endif

    return retval;
# else
#  error Unimplemented assembly architecture for qthread_incr64
# endif
#endif
}				       /*}}} */

static QINLINE unsigned long qthread_incr_xx(
    volatile void *addr,
    const long int incr,
    const size_t length)
{				       /*{{{ */
    switch (length) {
	case 4:
	    return qthread_incr32((volatile uint32_t *)addr, incr);
	case 8:
	    return qthread_incr64((volatile uint64_t *)addr, incr);
	default:
	    /* This should never happen, so deliberately cause a seg fault
	     * for corefile analysis */
	    *(int *)(0) = 0;
    }
    return 0;			       /* compiler check */
}				       /*}}} */

#if ! defined(QTHREAD_ATOMIC_CAS) || defined(QTHREAD_MUTEX_INCREMENT)
static QINLINE uint32_t qthread_cas32(volatile uint32_t * operand,
				      const uint32_t oldval,
				      const uint32_t newval)
{				       /*{{{ */
#ifdef QTHREAD_MUTEX_INCREMENT // XXX: this is only valid if you don't read *operand without the lock
    return qthread_cas32_(operand,oldval,newval);
#else
# if (QTHREAD_ASSEMBLY_ARCH == QTHREAD_POWERPC32) || \
      (QTHREAD_ASSEMBLY_ARCH == QTHREAD_POWERPC64)
    register uint32_t result;
    __asm__ __volatile__ ("1:\n\t"
	    "lwarx  %0,0,%3\n\t"
	    "cmpw   %0,%1\n\t"
	    "bne    2f\n\t"
	    "stwcx. %2,0,%3\n\t"
	    "bne-   1b\n"
	    "2:"
	    "isync" /* make sure it wasn't all a dream */
	    :"=&b" (result)
	    :"r"(oldval), "r"(newval), "r"(operand)
	    :"cc", "memory");
    return result;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_SPARCV9_32) || \
	(QTHREAD_ASSEMBLY_ARCH == QTHREAD_SPARCV9_64)
    register uint32_t newv = newval;
    __asm__ __volatile__
	("membar #StoreStore|#LoadStore|#StoreLoad|#LoadLoad\n\t"
	 "cas [%1], %2, %0"
	 : "+r" (newv)
	 : "r" (operand), "r"(oldval)
	 : "cc", "memory");
    return newv;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_IA64)
    register uint32_t retval;
    __asm__ __volatile__ ("mov ar.ccv=%0;;": :"rO" (oldval));
    __asm__ __volatile__ ("cmpxchg4.acq %0=[%1],%2,ar.ccv"
	    :"=r"(retval)
	    :"r"(operand), "r"(newval)
	    :"memory");
    return retval;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_AMD64) || \
        (QTHREAD_ASSEMBLY_ARCH == QTHREAD_IA32)
    uint32_t retval;
    /* note that this is GNU/Linux syntax (aka AT&T syntax), not Intel syntax.
     * Thus, this instruction has the form:
     * [lock] cmpxchg reg, reg/mem
     *                src, dest
     */
    __asm__ __volatile__ ("lock; cmpxchg %1,(%2)"
	    : "=&a"(retval) /* store from EAX */
	    : "r"(newval), "r" (operand),
	      "0"(oldval) /* load into EAX */
	    :"cc","memory");
    return retval;
# else
#  error Unimplemented assembly architecture for qthread_cas32
# endif
#endif
}				       /*}}} */

static QINLINE uint64_t qthread_cas64(volatile uint64_t * operand,
				      const uint64_t oldval,
				      const uint64_t newval)
{				       /*{{{ */
#ifdef QTHREAD_MUTEX_INCREMENT
    return qthread_cas64_(operand, oldval, newval);
#else
# if (QTHREAD_ASSEMBLY_ARCH == QTHREAD_POWERPC64)
    register uint64_t result;
    __asm__ __volatile__ ("1:\n\t"
	    "ldarx  %0,0,%3\n\t"
	    "cmpw   %0,%1\n\t"
	    "bne    2f\n\t"
	    "stdcx. %2,0,%3\n\t"
	    "bne-   1b\n"
	    "2:"
	    "isync" /* make sure it wasn't all a dream */
	    :"=&b" (result)
	    :"r"(oldval), "r"(newval), "r"(operand)
	    :"cc", "memory");
    return result;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_SPARCV9_32)
    register uint64_t tmp1=tmp1;
    register uint64_t tmp2=tmp2;
    uint64_t newv = newval;
    __asm__ __volatile__
	("ldx %0, %1\n\t"
	 "ldx %4, %2\n\t"
	 "membar #StoreStore|#LoadStore|#StoreLoad|#LoadLoad\n\t"
	 "casx [%3], %2, %1\n\t"
	 "stx %1, %0"
	 /* h means 64-BIT REGISTER
	  * (probably unneecessary, but why take chances?) */
	 : "+m" (newv), "=&h" (tmp1), "=&h"(tmp2)
	 : "r" (operand), "m"(oldval)
	 : "cc", "memory");
    return newv;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_SPARCV9_64)
    register uint64_t newv = newval;
    __asm__ __volatile__
	("membar #StoreStore|#LoadStore|#StoreLoad|#LoadLoad\n\t"
	 "casx [%1], %2, %0"
	 : "+r" (newv)
	 : "r" (operand), "r"(oldval)
	 : "cc", "memory");
    return newv;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_IA64)
    register uint32_t retval;
    __asm__ __volatile__ ("mov ar.ccv=%0;;": :"rO" (oldval));
    __asm__ __volatile__ ("cmpxchg8.acq %0=[%1],%2,ar.ccv"
	    :"=r"(retval)
	    :"r"(operand), "r"(newval)
	    :"memory");
    return retval;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_IA32)
    union {
	uint64_t i;
	struct {
	    /* note: the ordering of these is both important and
	     * counter-intuitive; welcome to little-endian! */
	    uint32_t l;
	    uint32_t h;
	} s;
    } oldv, newv, ret;
    oldv.i = oldval;
    newv.i = newval;
#ifndef QTHREAD_PIC_PREFIX
#ifdef __PIC__
       /* this saves off %ebx to make PIC code happy :P */
# define QTHREAD_PIC_PREFIX "xchg %%ebx, %4\n\t"
       /* this restores it */
# define QTHREAD_PIC_SUFFIX "\n\txchg %%ebx, %4"
# define QTHREAD_PIC_REG_4 "r"
#else
# define QTHREAD_PIC_PREFIX
# define QTHREAD_PIC_SUFFIX
# define QTHREAD_PIC_REG_4 "b"
#endif
#endif
    /* the PIC stuff is already defined above */
    __asm__ __volatile__ (
	    QTHREAD_PIC_PREFIX
	    "lock; cmpxchg8b (%2)"
	    QTHREAD_PIC_SUFFIX
	    :/*EAX*/"=a"(ret.s.l),
	    /*EDX*/"=d"(ret.s.h)
	    :"r"(operand),
	    /*EAX*/"a"(oldv.s.l),
	    /*EBX*/QTHREAD_PIC_REG_4(newv.s.l),
	    /*EDX*/"d"(oldv.s.h),
	    /*ECX*/"c"(newv.s.h)
	    :"memory");
    return ret.i;
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_AMD64)
    /* note that this is GNU/Linux syntax (aka AT&T syntax), not Intel syntax.
     * Thus, this instruction has the form:
     * [lock] cmpxchg reg, reg/mem
     *                src, dest
     */
#  ifdef __PGI
    /* this is a workaround for a bug in the PGI compiler where the width of
     * retval is not respected and %eax is used instead of %rax */
    uint64_t retval;
    __asm__ __volatile__ ("lock cmpxchg %1,(%2)\n\t"
	    "mov %%rax,(%0)"
	    ::"r"(&retval), "r"(newval), "r"(operand),
	    "a"(oldval) /* load into RAX */
	    :"cc","memory");
    return retval;
#  else
    uint64_t retval;
    __asm__ __volatile__ ("lock; cmpxchg %1,(%2)"
	    : "=a"(retval) /* store from RAX */
	    : "r"(newval), "r" (operand),
	      "a"(oldval) /* load into RAX */
	    :"cc","memory");
    return retval;
#  endif
# elif (QTHREAD_ASSEMBLY_ARCH == QTHREAD_POWERPC32)
    /* In general, RISC doesn't provide a way to do 64 bit operations from 32
     * bit code. Sorry! */
    uint64_t retval;
    qthread_t *me = qthread_self();

    qthread_lock(me, (aligned_t*)operand);
    retval = *operand;
    if (retval == oldval) {
	*operand = newval;
    }
    qthread_unlock(me, (aligned_t*)operand);
    return retval;
# else
#  error Unimplemented assembly architecture for qthread_cas64
# endif
#endif
}				       /*}}} */

static QINLINE aligned_t qthread_cas_xx(
    volatile aligned_t * addr,
    const aligned_t oldval,
    const aligned_t newval,
    const size_t length)
{				       /*{{{ */
    switch (length) {
	case 4:
	    return qthread_cas32((volatile uint32_t *)addr, oldval, newval);
	case 8:
	    return qthread_cas64((volatile uint64_t *)addr, oldval, newval);
	default:
	    /* This should never happen, so deliberately cause a seg fault
	     * for corefile analysis */
	    *(int *)(0) = 0;
    }
    return 0;			       /* compiler check */
}				       /*}}} */

static QINLINE void *qthread_cas_ptr_(
    void *volatile *const addr,
    void *const oldval,
    void *const newval)
{
#if (SIZEOF_VOIDP == 4)
    return (void *)(uintptr_t) qthread_cas32((volatile uint32_t *)
					     addr, (uint32_t) (uintptr_t)
					     oldval, (uint32_t) (uintptr_t)
					     newval);
#elif (SIZEOF_VOIDP == 8)
    return (void *)(uintptr_t) qthread_cas64((volatile uint64_t *)
					     addr, (uint64_t) (uintptr_t)
					     oldval, (uint64_t) (uintptr_t)
					     newval);
#else
#error The size of void* either could not be determined, or is very unusual.
    /* This should never happen, so deliberately cause a seg fault for
     * corefile analysis */
    *(int *)(0) = 0;
    return NULL;		       /* compiler check */
#endif
}
#endif /* ifdef QTHREAD_ATOMIC_CAS */

#ifdef QTHREAD_ATOMIC_CAS
# define qthread_cas(ADDR, OLDV, NEWV) \
    __sync_val_compare_and_swap((ADDR), (OLDV), (NEWV))
# define qthread_cas_ptr(ADDR, OLDV, NEWV) \
    (void*)__sync_val_compare_and_swap((ADDR), (OLDV), (NEWV))
#else
# define qthread_cas(ADDR, OLDV, NEWV) \
    qthread_cas_xx((volatile aligned_t*)(ADDR), (aligned_t)(OLDV), (aligned_t)(NEWV), sizeof(*(ADDR)))
# ifdef QTHREAD_ATOMIC_CAS_PTR
#  define qthread_cas_ptr(ADDR, OLDV, NEWV) \
    (void*)__sync_val_compare_and_swap((ADDR), (OLDV), (NEWV))
# else
#  define qthread_cas_ptr(ADDR, OLDV, NEWV) \
    qthread_cas_ptr_((void*volatile*const)(ADDR), (void*const)(OLDV), (void*const)(NEWV))
# endif
#endif

#ifdef QTHREAD_USE_ROSE_EXTENSIONS
extern int __qthreads_temp;
void qthread_reset_forCount(qthread_t *);
#endif

Q_ENDCXX /* */

#ifndef __cplusplus

# if defined(QTHREAD_ATOMIC_INCR) && !defined(QTHREAD_MUTEX_INCREMENT)
#  define qthread_incr( ADDR, INCVAL ) \
    __sync_fetch_and_add(ADDR, INCVAL)
# else
#  define qthread_incr( ADDR, INCVAL )                  \
   qthread_incr_xx( (volatile void*)(ADDR), (long int)(INCVAL), sizeof(*(ADDR)) )
# endif

#else /* ifdef __cplusplus */
# include "qthread.hpp"
#endif /* __cplusplus */

#endif /* QTHREAD_SST_PRIMITIVES */

#endif /* _QTHREAD_H_ */
