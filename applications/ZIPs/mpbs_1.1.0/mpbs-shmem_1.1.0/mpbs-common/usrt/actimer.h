/*** UNCLASSIFIED//FOR OFFICIAL USE ONLY ***/

// Copyright (c) 2010, Institute for Defense Analyses,
// 4850 Mark Center Drive, Alexandria, VA 22311-1882; 703-845-2500.
//
// This material may be reproduced by or for the U.S. Government 
// pursuant to the copyright license under the clauses at DFARS 
// 252.227-7013 and 252.227-7014.

/* $Id: actimer.h,v 0.16 2010/06/29 17:46:22 fmmaley Exp fmmaley $ */


// ************************** INTRODUCTION **************************
//
// ACTIMER is a lightweight, portable mechanism for timing portions,
// a.k.a. "activities", of a C program and accumulating the timings.  It
// consists of this header file <actimer.h> alone and does not require
// any library other than the standard C library.  Where possible, it
// uses platform-native high-resolution timers to enable accurate timing
// of small blocks of code.  It supports a notion of "work units" (e.g.,
// function calls or loop iterations) so that the final summary can
// report time per unit of work.  It also has a notion of "subtlety",
// akin to the verbosity levels that many programs provide for
// debugging, which lets you leave fine-grained timing calls in your
// code but disable the most detailed timers at run time and minimize
// their overhead.
//
// To use ACTIMER, place the following directives in your C file:
//
//     #define ACTIMER_MODULE_NAME some_token
//     #define ACTIMER_MAIN
//     #include <actimer.h>
//
// A definition of ACTIMER_MODULE_NAME is required.  It allows different
// modules in your program to use ACTIMER independently without worrying
// about collisions between timer numbers.  ACTIMER_MAIN must be defined
// in exactly one compilation unit that includes <actimer.h>, so that 
// ACTIMER will add its global state and code to your program exactly
// once.
//
// The next step is to assign timer numbers to the different activities
// in your module.  Timer numbers are small nonnegative integers.  You
// can place literal timer numbers in your ACTIMER calls, but for code
// readability and more informative output we recommend defining an
// enumeration like this
//
//     enum timers_e { setup, mainloop, cleanup };
//
// and using the enumeration constants instead.  Now place calls to
// actimer_start(), actimer_stop(), and/or actimer_switch() in your
// code, and at the end of the program call actimer_retrieve() and/or
// actimer_summarize() to see the results.  ACTIMER is self-initializing
// except when using Pthreads (see actimer_prepare_threads() below).
//
// ACTIMER allows multiple timers to be running simultaneously, but
// their use must be properly nested.  That is, if you start timer B
// while timer A is running, then you must stop timer B before stopping
// timer A.  Actimer_start() pushes a timer onto the stack of running
// timers, actimer_stop() pops a timer off the stack, and actimer_switch()
// performs a pop followed by a push.
//
// ACTIMER supports two styles of timer use.  Each style has benefits
// and drawbacks.
//
// (1) When an activity begins, call actimer_start(); when it ends,
//     call actimer_stop().  If an activity B is contained within an
//     activity A, you may leave the timer for A running while timing B.
//     At the end, actimer_summarize() will report the total time for A
//     and also the time spent in A but not in B or any other
//     subactivity of A.
//
// Method (1) checks for double starts and double stops of each timer,
// as well as improper nesting, which helps catch bugs in timer use.
// Its drawback is that it can involve more timer calls than strictly
// necessary.
//
// (2) If you want to partition your entire program or module into
//     activities, call actimer_start() when you start the first
//     activity, and call actimer_switch() whenever you switch from one
//     activity to another.  Whenever you call a function that might
//     include separate activities, you must call actimer_switch() after
//     it returns.  This call is safe in that switching from a timer to
//     the same timer is a no-op.
//
// Method (2) is more forgiving because actimer_switch() does not care
// what timer it stops.  So it will catch fewer timer bugs than method
// (1), but it can be simpler if you control all program's code.
//
// ACTIMER can store timing data for an an arbitary number of modules,
// with a large number of timers per module.  The default limit on the
// number of timers per module is ACTIMER_DEFAULT_TIMERS (=64), but you
// can request more (or fewer) timers by calling actimer_initialize().
//
// You can declare multiple ACTIMER modules per source file by
// undefining and redefining ACTIMER_MODULE_NAME and including
// <actimer.h> again.  You can also spread a module across multiple
// files by reusing the ACTIMER_MODULE_NAME and defining ACTIMER_SHARED.


// ************************* MULTITHREADING *************************
//
// As of version 0.8, ACTIMER has experimental support for programs that
// use POSIX threads (Pthreads).  
// In a multithreaded program, each thread
// has its own independent collection of timers, and most actimer
// functions (initialize, start, stop, switch, get_time, etc.) touch
// only the current thread's timers.  Whenever a thread exits, and
// whenever a thread calls actimer_aggregate(), its timings are
// accumulated into the global store that actimer_summarize() displays.
//
// The idea is that accumulated timings and work units can be shifted
// from a thread's local store to the global store, and from the global
// store to the output, but these operations will not duplicate or
// destroy information.  So, with a modest amount of care, one can
// arrange that each tick on a timer and each work unit is reported
// exactly once.  To ensure this, one should:
//
//   * Avoid calling actimer_erase() or actimer_release(), which
//     destroy timing data;
//   * Make a final call to actimer_summarize() after all but one
//     thread has exited.
//
// In order for a Pthreads program to use ACTIMER, the module that
// defines ACTIMER_MAIN must also define ACTIMER_PTHREADS to 1 before
// including <actimer.h>.  In addition, the program must call
// actimer_prepare_threads() before using any other ACTIMER functions.
// A violation of the latter condition will cause the program to exit
// with an appropriate error message.


// ************************* API REFERENCE **************************
//
// Required #defines:
//
// ACTIMER_MODULE_NAME -- used to uniquely name accessor functions
// ACTIMER_MAIN -- should be set in exactly one module to cause definition
//                 of global timer state and global ACTIMER functions
//
// Optional #defines: 
//
// ACTIMER_DISABLE -- if set, timing goes away
// ACTIMER_PTHREADS -- must be set to 1 in the module that defines
//     ACTIMER_MAIN if Pthreads support is desired; can be set to 0
//     in any module to sacrifice Pthreads support for extra speed;
//     otherwise should be left undefined
// ACTIMER_SHARED -- if set, allows timers to be shared with other modules
//     of the same name; otherwise, repeating a module name is an error
// ACTIMER_STACK_SIZE -- if set, and ACTIMER_MAIN is defined, sets the size
//     of the timer stack for the current module; otherwise ACTIMER chooses
//     a reasonable default value
// ACTIMER_USE_RTC -- if set to 0, causes ACTIMER to avoid the use of CPU
//     cycle counters, which may not be consistent across multiple CPUs and
//     may cause bogus timings, especially for multithreaded programs; if
//     set to 1, uses cycle counters if they are available; otherwise
//     should be left undefined, and ACTIMER will make its own decision
//     (currently, it will always try to use cycle counters)
//
// Functions and macros:
//
// Most of the ACTIMER interface is implemented via macros that invoke
// functions.  This design should not cause any concern unless the
// program does something unnatural with the ACTIMER macros.  Unnatural
// acts include providing arguments that have side effects and providing
// timer numbers that are expressions rather than literals.
//
//
// LOCAL OPERATIONS -- The following operations affect only the current
// module's timers.
//
// void actimer_initialize(int n_timers, void* ptr);  [MACRO]
//
//     Tells ACTIMER that the program has started and defines the number
// of timers for the current module.  The value of 'n_timers' (with 0
// being interpreted as ACTIMER_DEFAULT_TIMERS) must be greater than all
// the timer numbers that this module will use.  Normally 'ptr' should
// be NULL, but you can use it to provide your own memory for the timers
// in this module.  If you do, then n_timers must be nonzero, and the
// pointer must be 8-byte aligned and point to a block of at least
// ACTIMER_SIZEOF(n_timers) bytes.
//     This call is optional.  If omitted, ACTIMER will initialize
// itself at the first call to actimer_start() and will allocate
// ACTIMER_DEFAULT_TIMERS timers.  Aside from controlling the number of
// timers and the space allocated for them, the only reason to call
// actimer_initialize() is to start certain internal timers so that
// actimer_measure_tick() can be used sooner and ACTIMER can correctly
// report the fraction of the running time covered by explicit timers.
// Calls to actimer_initialize() have no effect after the current module
// has been initialized, and will be flagged as errors if they try to
// alter the number of timers or the timer memory.
//
// void actimer_start(int timer, int subtlety);  [MACRO]
//
//     Starts the activity timer with number 'timer', unless 'subtlety'
// exceeds the subtlety level for the current module.  The timer number
// must be nonnegative and less than the number of timers allocated for
// the current module.  It is an error for the given timer to be already
// running, but it is not an error for other timers to be running.  In
// case of error, a message will be written to standard output, but at
// most one message of each type will be written for each valid timer
// until the timer is released.
//
// void actimer_stop(int timer, int subtlety, int64 work_done);  [MACRO]
//
//     If 'subtlety' exceeds the subtlety level for the current module,
// does nothing.  Otherwise stops the activity timer with number
// 'timer', and adds 'work_done' to the total number of work units
// associated with the that timer.  If the given timer is not running,
// or is not the most recently started timer, an error is reported.
// Errors are handled as by actimer_start().
//
// void actimer_switch(int timer, int subtlety, int64 work_todo);  [MACRO]
//
//     If 'subtlety' exceeds the subtlety level for the current module,
// does nothing.  Otherwise, stops the activity timer most recently
// started, starts the timer with number 'timer'; and adds 'work_todo'
// to the work total for that timer (NOT the one just stopped).  It is
// not an error to switch to the innermost running timer.  Switching to
// it has no effect except to increase its work total.  It is an error
// to switch to a different running timer, or to call actimer_switch()
// when no timer is running.  Errors are handled as by actimer_start().
//
// void actimer_erase();  [MACRO]
//
//     Erases the timing and work totals for the current module, but
// does not change the set of running timers.
//
// void actimer_release();  [MACRO]
//
//     Erases all stored information for the current module, returns
// it to an uninitialized state, and frees any associated memory allocated
// by ACTIMER.  This function is provided mainly for libraries that wish
// to retrieve their own timing data via actimer_get_{time,work,all}
// and then clean up after themselves.
//
// int64 actimer_get_time(int timer);  [MACRO]
// int64 actimer_get_work(int timer);  [MACRO]
//
//     Returns the total number of "ticks" or the number of work units
// recorded by the given timer, respectively, without affecting the
// timer.  It is not an error for the timer to be running, although
// the overhead of actimer_get_time() may be larger in that situation.
// The meaning of a "tick" is platform-dependent.
//
// int64 actimer_get_all(int exclusive, int64 time[], int64 work[]);
//       [MACRO]
//
//     Returns a bitmask of all the active timers in the current module,
// and stores in the given arrays, indexed by timer number, the total
// times (in "ticks") and total work units recorded by the active
// timers.  In the bitmask, timer number i corresponds to the bit
// I64(1)<<i.  If a timer has not been used since the timing information
// for this module was last initialized, then the corresponding elements
// of the time[] and work[] arrays are not changed.  The caller is
// responsible for making the arrays large enough.  Either 'time' or
// 'work' may be NULL if the caller does not want data of that type.  If
// 'exclusive' is nonzero, actimer_get_all() returns exclusive timings,
// i.e., for each timer, the number of ticks during which it was the
// innermost running timer.
//
//
// GLOBAL OPERATIONS -- The following operations involve the state for a
// thread or for the entire program.
//
// int actimer_set_subtlety(int subtlety);
//
//     If 'subtlety' is zero or positive, sets the ACTIMER subtlety
// level for this thread and returns the previous value.  If 'subtlety'
// is negative, returns the current subtlety level and does not change
// it.  NOTE: the subtlely level for a module -- a set of source files
// sharing the same value for ACTIMER_MODULE_NAME -- is frozen the first
// time that module calls actimer_initialize(), actimer_start(),
// actimer_stop(), or actimer_switch(), and persists until the module
// calls actimer_release().
//
// void actimer_prepare_threads(void);  [Pthreads only]
//
//     In order to use ACTIMER in a multithreaded (Pthreads) program,
// some thread must call this function before any thread calls any
// other ACTIMER function.  Call it before spawning threads.
//
// void actimer_aggregate(void);  [Pthreads only]
//
//     Takes a snapshot of all the timer data for the current thread,
// including the time elapsed on running timers, and adds it to the
// global totals to be printed by the next call to actimer_summarize().
// This call resets the time and work totals for the current thread's
// timers to zero, but does not change which timers are running, so it
// can safely be inserted at any point in the code.
//
// void actimer_summarize(FILE* stream);
//
//     Reports all the time and work data collected since ACTIMER was
// initialized or actimer_summarize() was last called, printing it to
// the given stream and then flushing that stream.  In effect,
// actimer_summarize() first calls actimer_aggregate() and then reports
// and clears the global totals.  It does not stop any running timers,
// although it pauses the current thread's timers while printing.
//     The thread that calls actimer_summarize() needs to have already
// called either actimer_start() or actimer_initialize(), preferably
// before any other thread begins using ACTIMER (or else the overall
// CPU-time and elapsed-time data will be misleading).
//     The summary contains, for each timer with nonzero elapsed time,
// the total ("inclusive") time it recorded and the time per work unit
// (if the number of work units was nonzero).  It also contains the
// total ("exclusive") time it was the innermost running timer, and
// this quantity divided by the number of work units (if nonzero).
//
// double actimer_measure_tick(int cputime);
//
//     Returns an estimate of the number of wall-clock seconds or CPU
// seconds (time your program actually got) per "tick", according to
// whether 'cputime' is zero or not.  The wall-clock estimate is likely
// to be much more precise.  In multithreaded mode, we cannot measure
// CPU time used by a single thread, so the wall-clock estimate is
// always used and 'cputime' is ignored.
//     WARNING: Do not call actimer_measure_tick() until you have
// already collected some timing data.  It will return zero if no timers
// have ever been started or if no timers have been started since the
// last call to actimer_summarize().  It may also return zero if the
// elapsed time has been very short, e.g., less than 0.01 second.
//
//
// Note: ACTIMER defines other local and global symbols for its own
// purposes.  Don't use them.


#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <micro64.h>
#include <bitops.h>

#ifndef ACTIMER_MODULE_NAME
#error "Must #define ACTIMER_MODULE_NAME for every individual module."
#endif

#undef _ACTIMER_SHARED
#ifdef ACTIMER_SHARED
#define _ACTIMER_SHARED 1
#else
#define _ACTIMER_SHARED 0
#endif

#undef _ACTIMER_PTHREADS
#if defined(ACTIMER_PTHREADS) && (ACTIMER_PTHREADS != 0)
#define _ACTIMER_PTHREADS 1
#elif defined(ACTIMER_PTHREADS) && (ACTIMER_PTHREADS == 0)
#define _ACTIMER_PTHREADS -1
#else
#define _ACTIMER_PTHREADS 0
#endif


#ifndef ACTIMER_ONCE_PER_FILE
#define ACTIMER_ONCE_PER_FILE

#ifdef __GNUC__
#define ACTIMER_NOWARN __attribute__ ((unused))
#else
#define ACTIMER_NOWARN
#endif

#define ACTIMER_MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define ACTIMER_MAKE_STR(X)   ACTIMER_MAKE_VAL(X)
#define ACTIMER_MAKE_VAL(X)   #X
#define ACTIMER_CONCAT1(X,Y)  ACTIMER_CONCAT2(X,Y)
#define ACTIMER_CONCAT2(X,Y)  X ## Y
#undef ACTIMERCTL
#define ACTIMERCTL ACTIMER_CONCAT1(actimerctl_, ACTIMER_MODULE_NAME)
#undef ACTIMERCTX
#define ACTIMERCTX ACTIMER_CONCAT1(actimerctx_, ACTIMER_MODULE_NAME)
#undef ACTIMERGET
#define ACTIMERGET ACTIMER_CONCAT1(actimerget_, ACTIMER_MODULE_NAME)

#define ACTIMER_MAX_TIMERS    256
#define ACTIMER_DEFAULT_TIMERS 64

typedef struct actimer_t         actimer_t;
typedef struct actimer_context_t actimer_context_t;
typedef struct actimer_thread_t  actimer_thread_t;
typedef union  actimer_anchor_t  actimer_anchor_t;

// Structure representing one timer.
struct actimer_t {
  int64 start_time;   // if zero or negative, this timer is stopped
  int64 total_time;   // total time this timer has run
  int64 exclu_time;   // total time this timer has been at top of stack
  int64 total_work;   // counts arbitrary "work units" assoc. with this timer
  const char* name;   // name for this timer
  uint32 errors;      // identifies errors in usage of this timer (for each
                      // timer, we only print the first error of each type)
};

// Structure representing one module that wants timings.  It contains
// the array of timers for that module.
struct actimer_context_t {
  actimer_context_t* next;  // next context in doubly linked list
  actimer_context_t* prev;  // previous context in doubly linked list
  actimer_thread_t* parent; // link to thread state
  const char* name;         // name of this module
  uint32 errors;            // identifies errors in usage of this context
  int subtlety;             // subtletly level for this module
  int8 dynamic;             // was this context dynamically allocated?
  int8 invalid;             // is this context unavailable for use?
  int8 shared;              // share counters with like-named modules?
  int n_timers;             // dimension of the timers[] array
  actimer_t timers[];       // array of timers (C99 flexible array)
};

// Structure representing one thread that wants timings.  It contains
// the stack of running timers.  It points to the base context, which is
// the root of a doubly linked list of contexts ordered by context name.
// Finally, it tracks all *pointers* to contexts so that contexts can be
// safely deallocated.
struct actimer_thread_t {
  actimer_context_t* base;  // the base context, with one basic timer
  double wall;              // initial wall-clock time, in microseconds
  double rusage;            // initial CPU time used, in microseconds
  int64 latest;             // time of last actimer request
#if _ACTIMER_PTHREADS > 0
  actimer_context_t** contexts;  // dynamic array of context pointers
  int n_contexts;                // the size of that array
#else
  actimer_context_t*** handles;  // dynamic array of context handles
  int n_handles;                 // the size of that array
#endif
  int subtlety;             // subtlety level
  int top;                  // stack pointer (index of top timer)
  int n_stack;              // dimension of the following array
  actimer_t* stack[];       // stack of running timers (C99 flexible array)
};

#define ACTIMER_SIZEOF(n) (sizeof(actimer_context_t) + (n)*sizeof(actimer_t))

// Structure that stores static data foreach (module,file) pair
union actimer_anchor_t {
  actimer_context_t* context;
  int                id;
};

// Public routines:

extern int    actimer_set_subtlety (int level);
extern double actimer_measure_tick (int cputime);
extern void   actimer_summarize (FILE* file);
#if _ACTIMER_PTHREADS > 0
extern void   actimer_prepare_threads (void);
extern void   actimer_aggregate (void);
#endif

// Private routines:

// Find an existing shared context by name; if not found, creates a new
// context in the given block of memory or (if NULL) in newly allocated
// memory.  Returns the thread's "base" context on error.  If the
// requested number of timers is 0, it means defer to the existing
// context or allocate ACTIMER_DEFAULT_TIMERS if necessary.
extern actimer_context_t* actimer_new_context
    (actimer_context_t** handle, void* memory, const char *context_name,
     int shared, int threaded, int using_rtc, int n_timers);

// Clear all stored timing data for this context.
extern void actimer_erase_context (actimer_context_t* context);

// Remove the context from the thread, but don't free any associated memory.
extern void actimer_remove_context (actimer_context_t* context);

// Destroy a context (the caller must remove it first or clear the stack).
extern void actimer_free_context (actimer_context_t* context);

// Take a snapshot of all running timers, i.e., add their current elapsed
// times (inclusive and exclusive) to the running totals
extern void actimer_record_all (actimer_thread_t* actimer);

// Report an error to the user.  The error can be associated with
// a timer (if neither context nor timer is NULL), a context (if
// context is non-NULL but timer is NULL), or an entire thread
// (if context is NULL).  A timer-specific or context-specific error
// is reported at most once per timer or per context, respectively,
// until the context is released.  (The code parameter is used to
// identify the error.)  A thread-specific error with a nonzero code
// causes the program to exit with that status code.
extern void actimer_error (int code, actimer_context_t* context,
			   actimer_t* timer, char* format, ...);

// Allocate or reallocate memory, clearing the new portion
extern void* actimer_alloc (void* p, size_t oldsize, size_t newsize);

// Low-overhead timing:

#if defined(_rtc) && (!defined(ACTIMER_USE_RTC) || ACTIMER_USE_RTC != 0)

#define _ACTIMER_USING_RTC 1
#define actimer_rtc _rtc

#else

#define _ACTIMER_USING_RTC 0
#include <sys/times.h>
ACTIMER_NOWARN static INLINE int64
actimer_rtc (void) 
{
  struct timeval tp;
  gettimeofday (&tp, NULL);
  return I64(1000000)*tp.tv_sec + tp.tv_usec;
}

#endif // rtc

// CTL requests

#define ACTIMER_INIT_REQ    0
#define ACTIMER_START_REQ   1
#define ACTIMER_STOP_REQ    2
#define ACTIMER_SWITCH_REQ  3
#define ACTIMER_FREE_REQ    4
#define ACTIMER_ERASE_REQ   5

#define actimer_initialize(TNUM,PTR)			\
  ACTIMERCTL(ACTIMER_INIT_REQ, TNUM, PTR, 0, 0)
#define actimer_start(TNUM,SUBTLETY) \
  ACTIMERCTL(ACTIMER_START_REQ, TNUM, (void*)ACTIMER_MAKE_STR(TNUM), SUBTLETY, 0)
#define actimer_stop(TNUM,SUBTLETY,WORKDONE) \
  ACTIMERCTL(ACTIMER_STOP_REQ, TNUM, (void*)ACTIMER_MAKE_STR(TNUM), SUBTLETY, WORKDONE)
#define actimer_switch(TNUM,SUBTLETY,WORKTODO) \
  ACTIMERCTL(ACTIMER_SWITCH_REQ, TNUM, (void*)ACTIMER_MAKE_STR(TNUM), SUBTLETY, WORKTODO)
#define actimer_release() \
  ACTIMERCTL(ACTIMER_FREE_REQ, 0, NULL, 0, 0)
#define actimer_erase() \
  ACTIMERCTL(ACTIMER_ERASE_REQ, 0, NULL, 0, 0)

// GET requests

#define ACTIMER_TIME_GET    0
#define ACTIMER_WORK_GET    1
#define ACTIMER_INCL_GET    2
#define ACTIMER_EXCL_GET    3

#define actimer_get_time(TNUM) \
  ACTIMERGET(ACTIMER_TIME_GET, TNUM, NULL, NULL)
#define actimer_get_work(TNUM) \
  ACTIMERGET(ACTIMER_WORK_GET, TNUM, NULL, NULL)
#define actimer_get_all(EXCL, TIMES, WORKS) \
  ACTIMERGET(ACTIMER_INCL_GET + !!(EXCL), 0, TIMES, WORKS)

// Access to the local context

#if _ACTIMER_PTHREADS >= 0
extern actimer_context_t** actimer_get_handle(actimer_anchor_t* state);
#define ACTIMER_HANDLE actimer_get_handle(&ACTIMERCTX)
#else
#define ACTIMER_HANDLE (&(ACTIMERCTX.context))
#endif

#endif  // ACTIMER_ONCE_PER_FILE



#ifdef ACTIMER_DISABLE

ACTIMER_NOWARN static void
ACTIMERCTL (int request, int timer_no, void* pointer,
	    int subtlety, int64 work_units)
{
  ;
}

ACTIMER_NOWARN static int64
ACTIMERGET (int request, int timer_no, int64 time[], int64 work[]) 
{
  return I64(0);
}

#else

// In the single-threaded case, the global state for this module is just
// a pointer to an actimer_context object.  When the context is first
// created, the location ("handle") of this pointer is registered in
// the actimer_thread object so that when the context is destroyed,
// we can erase all references to it.  (If the module is shared,
// its actimer_context may have multiple handles.)

// In the Pthreads case, the global state for this module is an integer
// ID that is unique to a (module,file) pair.  It is used as an index
// into the context arrays that belong to actimer_thread objects.  It
// must only be accessed through actimer_get_handle, which performs the
// appropriate locking.

static actimer_anchor_t ACTIMERCTX;

ACTIMER_NOWARN static void
ACTIMERCTL (int request, int timer_no, void* pointer,
	    int subtlety, int64 work_units)

{
  actimer_thread_t *actimer;
  actimer_context_t *context, **handle;
  actimer_t *timer, *otimer;
  const char* timer_name;
  int64 now;

  // Find the actimer_context for this thread and module.  If necessary,
  // create it and store a pointer to it in the expected place.
  handle = ACTIMER_HANDLE;
  context = *handle;
  if (context == NULL) {
    if (request == ACTIMER_FREE_REQ ||
	request == ACTIMER_ERASE_REQ)  return;
    void* memory = (request == ACTIMER_INIT_REQ ? pointer : NULL);
    int n_timers = (request == ACTIMER_INIT_REQ ? timer_no : 0);
    context = actimer_new_context
      (handle, memory, ACTIMER_MAKE_STR(ACTIMER_MODULE_NAME),
       _ACTIMER_SHARED, _ACTIMER_PTHREADS, _ACTIMER_USING_RTC, n_timers);
    *handle = context;
    if (request == ACTIMER_INIT_REQ)  return;
  }

  // context->invalid means we were unable to allocate context,
  // or were forced to destroy it due to stack overflow
  if (context->invalid)  return;
  if (subtlety > context->subtlety)  return;

  switch (request) {
  case ACTIMER_INIT_REQ:
    if ((pointer != NULL && pointer != (void*)context) ||
	(timer_no != 0 && timer_no != context->n_timers))
      actimer_error(6, context, NULL, "belated attempt to initialize");
    return;

  case ACTIMER_ERASE_REQ:
    actimer_erase_context(context);
    return;

  case ACTIMER_FREE_REQ:
    actimer_remove_context(context);
    actimer_free_context(context);
    return;

  default:
    break;
  }

  timer_name = (const char*) pointer;
  if (timer_no < 0 || timer_no >= context->n_timers) {
    actimer_error(0, context, NULL, "timer number %d (%s) is out of range"
		  " [0,%d]", timer_no, timer_name, context->n_timers-1);
    return;
  }

  actimer = context->parent;
  timer = &context->timers[timer_no];
  if (timer->name == NULL)
    timer->name = timer_name;
  now = actimer_rtc();
  actimer->stack[actimer->top]->exclu_time += now - actimer->latest;
  actimer->latest = now;

  switch (request) {
  case ACTIMER_START_REQ:
    if (timer->start_time > 0)
      actimer_error(0, context, timer, "repeated start");
    else if (actimer->top >= actimer->n_stack - 1) {
      actimer_error(5, context, NULL, "stack overflow, dropping this module");
      actimer_remove_context(context);
      context->invalid = 1;
    } else {
      timer->start_time = now;
      actimer->stack[++actimer->top] = timer;
    }
    break;

  case ACTIMER_STOP_REQ:
    if (timer->start_time <= 0)
      actimer_error(1, context, timer, "stop without start");
    else if (timer != actimer->stack[actimer->top])
      actimer_error(2, context, timer, "stop paired with start of %s",
		    actimer->stack[actimer->top]->name);
    else {
      timer->total_time += now - timer->start_time;
      timer->start_time  = -1;
      actimer->top--;
    }    
    timer->total_work += work_units;
    break;

  case ACTIMER_SWITCH_REQ:
    timer->total_work += work_units;
    if (actimer->top == 0) {
      actimer_error(3, context, timer, "switch with no running timer");
      actimer->top++;
    } else {
      otimer = actimer->stack[actimer->top];
      if (timer == otimer)
	return;
      otimer->total_time += now - otimer->start_time;
      otimer->start_time  = -1;
    }
    if (timer->start_time > 0) {
      actimer_error(4, context, timer, "repeated start (via switch)");
      actimer->top--;
    } else {
      timer->start_time = now;
      actimer->stack[actimer->top] = timer;
    }
    break;

  default:
    actimer_error(5, context, timer, "invalid request (%d)", request);
    break;
  }
}

ACTIMER_NOWARN static int64
ACTIMERGET (int request, int timer_no, int64 time[], int64 work[])

{
  actimer_thread_t* actimer;
  actimer_context_t *context;
  actimer_t *timer;
  int64 timer_mask;

  context = *ACTIMER_HANDLE;
  if (context == NULL || context->invalid)
    return I64(0);

  actimer = context->parent;
  if (request == ACTIMER_INCL_GET || request == ACTIMER_EXCL_GET) {
    int n;
    int inclusive = (request == ACTIMER_INCL_GET);
    actimer_record_all(actimer);
    for (n = 0; n < context->n_timers; n++)
      if (context->timers[n].name != NULL) {
	timer_mask |= I64(1) << n;
	timer = &context->timers[n];
	if (time != NULL)
	  time[n] = inclusive ? timer->total_time : timer->exclu_time;
	if (work != NULL)
	  work[n] = timer->total_work;
      }
    return timer_mask;
  }

  if (timer_no < 0 || timer_no >= context->n_timers)
    return I64(0);
  timer = &context->timers[timer_no];

  switch (request) {
  case ACTIMER_TIME_GET:
    return timer->total_time +
      (timer->start_time > 0 ? actimer_rtc() - timer->start_time : 0);

  case ACTIMER_WORK_GET:
    return timer->total_work;

  default:
    actimer_error(6, context, timer, "invalid 'get' request (%d)", request);
    return I64(0);
  }
}
					     
#endif


//
// globally accessible and shared routines (non-static)
// also global variables
//

#ifdef ACTIMER_MAIN
#ifdef ACTIMER_DISABLE

#if _ACTIMER_PTHREADS > 0
void actimer_prepare_threads (void)
{
  ;
}

void actimer_aggregate (void)
{
  ;
}
#endif

int actimer_set_subtlety (int level)
{
  return 0;
}

double actimer_measure_tick (int cputime)
{
  return 1.0e-9;
}

void actimer_summarize (FILE* file)
{
  ;
}

#else

#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

#ifndef ACTIMER_STACK_SIZE
// We could vary the default stack size according to the platform...
#define ACTIMER_STACK_SIZE 64
#endif

// Here is the global state and its accessor.

#if _ACTIMER_PTHREADS > 0

// In the multithreaded case, the actimer_thread objects and their base
// contexts will be dynamically allocated thread-specific data (TSD).
// We need shared global state to hold the TSD key, and also to assign
// unique numeric IDs to (module,file) pairs.

#include <pthread.h>
static struct {
  pthread_mutex_t   lock;     // a lock for all global data
  int               maxid;    // the largest ID that has been handed out
  int               ready;    // has the key been created?
  actimer_thread_t* total;    // holds aggregate timings
  double            elapsed;  // total microseconds elapsed on all threads
  // double            used;     // total CPU time used on all threads
  pthread_key_t     key;      // for thread-specific data
} _actimer_global = { PTHREAD_MUTEX_INITIALIZER, 0, 0, NULL, 0.0, 0.0 };

static actimer_thread_t* actimer_get_thread (void)
{
  actimer_thread_t* actimer;

  if (!_actimer_global.ready)
    actimer_error(1, NULL, NULL, "nobody called actimer_prepare_threads()");
  actimer = pthread_getspecific(_actimer_global.key);
  if (actimer)
    return actimer;

  actimer = actimer_alloc(NULL, 0, sizeof(actimer_thread_t) +
			  ACTIMER_STACK_SIZE*sizeof(actimer_t*));
  actimer->base = actimer_alloc(NULL, 0, ACTIMER_SIZEOF(1));
  actimer->base->dynamic = 1;
  actimer->n_contexts = 4;
  actimer->contexts = actimer_alloc(NULL, 0, 4 * sizeof(actimer_context_t*));
  actimer->contexts[0] = actimer->base;
  pthread_setspecific(_actimer_global.key, actimer);
  return actimer;
}

#else

// In the single-threaded case, the global state can simply be static.

static union {
  actimer_context_t context;
  char data[ACTIMER_SIZEOF(1)];
} _actimer_base;

static union {
  actimer_thread_t state;
  char data[sizeof(actimer_thread_t) + ACTIMER_STACK_SIZE*sizeof(actimer_t*)];
} _actimer_thread = { .state = { &_actimer_base.context } };

static INLINE actimer_thread_t* actimer_get_thread (void)
{
  return &_actimer_thread.state;
}

#endif


/* PRIVATE FUNCTIONS IN ACTIMER_MAIN */

#if _ACTIMER_PTHREADS > 0

actimer_context_t** actimer_get_handle (actimer_anchor_t* state)
{
  actimer_thread_t* actimer = actimer_get_thread();
  volatile int* idloc = &(state->id);
  int id = *idloc, n = actimer->n_contexts;

  if (id == 0) {
    pthread_mutex_lock(&_actimer_global.lock);
    // Check again -- maybe it was updated just now
    id = *idloc;
    if (id == 0)
      *idloc = id = ++_actimer_global.maxid;
    pthread_mutex_unlock(&_actimer_global.lock);
  }
  // Expand the array if necessary
  if (id >= n) {
    actimer->contexts = actimer_alloc(actimer->contexts,
				      n * sizeof(actimer_context_t*),
				      2*id * sizeof(actimer_context_t*));
    actimer->n_contexts = 2*id;
  }

  return &actimer->contexts[id];
}

#else

actimer_context_t** actimer_get_handle (actimer_anchor_t* state)
{
  return &(state->context);
}

#endif

// Returns wall-clock time in microseconds
static double actimer_wall (void)
{
  struct timeval tp;

  gettimeofday (&tp, NULL);
  return 1.0e6 * tp.tv_sec + tp.tv_usec;
}

// Returns CPU time in microseconds
static double actimer_rusage (void)
{
  struct rusage usage;

  getrusage(RUSAGE_SELF, &usage);
  return 1.0e6 * (usage.ru_utime.tv_sec + usage.ru_stime.tv_sec)
    + (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec);
}

// Record the time elapsed on all running timers, as if we stopped and
// restarted them.  This should be harmless (provided that the thread
// structure has been initialized).
void actimer_record_all (actimer_thread_t* actimer)
{
  int n;
  int64 now = actimer_rtc();
  actimer->stack[actimer->top]->exclu_time += now - actimer->latest;
  for (n = actimer->top; n >= 0; n--) {
    actimer_t* timer = actimer->stack[n];
    timer->total_time += now - timer->start_time;
    timer->start_time = now;
  }
  actimer->latest = now;
}

// Erase timing data and reset thread-wide timers; don't change the
// set of running timers, but reset their start times.
static void actimer_reset_all (actimer_thread_t* actimer)
{
  actimer_context_t* context;
  for (context = actimer->base; context != NULL; context = context->next)
    actimer_erase_context(context);

  // Start wall-clock and cpu usage timers
  actimer->wall     = actimer_wall();
  actimer->rusage   = actimer_rusage();

  // Restart all running timers
  if (actimer->n_stack > 0) {
    int n;
    int64 now = actimer_rtc();
    for (n = actimer->top; n >= 0; n--)
      actimer->stack[n]->start_time = now;
    actimer->latest = now;
  }
}

// Set up the stack and the base context, which are initially
// zeroed out except for actimer->base and actimer->base->dynamic.
static void actimer_init_thread (actimer_thread_t* actimer, int n_stack)
{
  actimer_context_t* base = actimer->base;

  // Initialize the base context
  base->parent   = actimer;
  base->name     = "";  // the lexicographically first string
  base->invalid  = 1;   // this context is not for users
  base->n_timers = 1;

  // Initialize the thread object
  if (n_stack > 0) {
    actimer->top      = 0;
    actimer->n_stack  = n_stack;
    actimer->stack[0] = &base->timers[0];
    actimer_reset_all(actimer);
  }
}

actimer_context_t* actimer_new_context
    (actimer_context_t** handle, void* memory, const char *module_name,
     int shared, int threaded, int using_rtc, int n_timers)
{
  actimer_thread_t* actimer;
  actimer_context_t *base, *context, *mypred;
  size_t size;
  int excessive = 0;
  
  if (threaded < 0 && _ACTIMER_PTHREADS > 0)
    actimer_error(2, NULL, NULL, "modules %s and %s have incompatible values "
		  "of ACTIMER_PTHREADS", ACTIMER_MAKE_STR(ACTIMER_MODULE_NAME),
		  module_name);

  actimer = actimer_get_thread();
  if (actimer->latest == 0)
    actimer_init_thread(actimer, ACTIMER_STACK_SIZE);
  base = actimer->base;

  if (using_rtc != _ACTIMER_USING_RTC) {
    actimer_error(0, NULL, NULL, "module %s compiled with wrong clock "
		  "(ACTIMER_USE_RTC), cannot be timed", module_name);
    return base;
  }

  // Register the handle if necessary.
#if _ACTIMER_PTHREADS <= 0
  { int n, n0 = actimer->n_handles;
    for (n = 0; n < n0; n++)
      if (actimer->handles[n] == handle)
	break;
      else if (actimer->handles[n] == NULL) {
	actimer->handles[n] = handle;
	break;
      }
    if (n == n0) {
      n = (n <= 2) ? 4 : 2*n;
      actimer->handles = actimer_alloc(actimer->handles,
				       n0 * sizeof(actimer_context_t**),
				       n * sizeof(actimer_context_t**));
      actimer->handles[n0] = handle;
      actimer->n_handles = n;
    }
  }
#endif

  for (mypred = base, context = base->next; context != NULL;
       mypred = context, context = context->next) {
    int order = strcmp(module_name, context->name);
    if (order < 0) break;
    if (order == 0) {
      if (shared && context->shared) {
	if (n_timers != 0 && n_timers != context->n_timers)
	  actimer_error(1, context, NULL, "has %d timers, but now wants %d",
			context->n_timers, n_timers);
	return context;
      }
      actimer_error(2, context, NULL, "duplicate name, not shared");
      return base;  // Permanently invalidate this context
    }
  }

  // Decide on the number of timers
  if (n_timers > ACTIMER_MAX_TIMERS) {
    excessive = n_timers;
    n_timers = ACTIMER_MAX_TIMERS;
  } else if (n_timers <= 0) {
    if (memory) {
      actimer_error(0, NULL, NULL, "attempt to initialize module %s ",
		    "with %d timers", module_name, n_timers);
      return base;
    }
    n_timers = ACTIMER_DEFAULT_TIMERS;
  }

  // Grab the memory
  size = ACTIMER_SIZEOF(n_timers);
  if (memory) {
    context = (actimer_context_t*) memory;
    memset(memory, 0, size);
  } else
    context = actimer_alloc(NULL, 0, size);

  // Report if the number of timers was too large
  if (excessive)
    actimer_error(3, context, NULL, "wants %d timers, maximum is %d",
		  excessive, ACTIMER_MAX_TIMERS);

  // Add to doubly-linked list
  if (mypred->next)
    mypred->next->prev = context;
  context->next = mypred->next;
  context->prev = mypred;
  mypred->next = context;

  // Fill in the other fields
  context->parent   = actimer;
  context->name     = module_name;
  context->subtlety = actimer->subtlety;
  context->dynamic  = !memory;
  context->invalid  = 0;
  context->shared   = shared;
  context->n_timers = n_timers;

  return context;
}

void actimer_remove_context (actimer_context_t* context)
{
  actimer_thread_t* actimer = context->parent;
  int64 now;
  int m, n;

  if (context == actimer->base) {
    actimer_error(4, context, NULL, "attempt to remove base context");
    return;
  }

  // Stop all timers in this context
  for (n = 0; n < context->n_timers; n++)
    context->timers[n].start_time = -1;

  // Pop stopped timers off the stack
  while (actimer->top > 0 && actimer->stack[actimer->top]->start_time <= 0)
    actimer->top--;

  // Bump up the exclusive timing of the top remaining timer
  now = actimer_rtc();
  actimer->stack[actimer->top]->exclu_time += now - actimer->latest;
  actimer->latest = now;

  // Look for buried timers and remove them
  for (m = 0, n = 1; n <= actimer->top; n++)
    if (actimer->stack[n]->start_time <= 0)
      actimer_error(7, context, actimer->stack[n], "released out of order");
    else
      actimer->stack[++m] = actimer->stack[n];
  actimer->top = m;
}

void actimer_erase_context (actimer_context_t* context)
{
  int n;
  for (n = 0; n < context->n_timers; n++) {
    actimer_t* timer = &context->timers[n];
    timer->total_time = 0;
    timer->exclu_time = 0;
    timer->total_work = 0;
  }
}

void actimer_free_context (actimer_context_t* context)
{
  actimer_thread_t* actimer = context->parent;
  int n;

  // Get rid of all references to this context
#if _ACTIMER_PTHREADS > 0
  for (n = 0; n < actimer->n_contexts; n++)
    if (actimer->contexts[n] == context)
      actimer->contexts[n] = NULL;
#else
  for (n = 0; n < actimer->n_handles; n++)
    if (actimer->handles[n] && *(actimer->handles[n]) == context)
      *(actimer->handles[n]) = NULL;
#endif

  // Remove the context from the doubly-linked list, and deallocate it
  context->prev->next = context->next;
  if (context->next)
    context->next->prev = context->prev;
  if (context->dynamic)
    free(context);
  else
    memset(context, 0, ACTIMER_SIZEOF(context->n_timers));
}

void actimer_error (int code, actimer_context_t* context, actimer_t* timer,
		    char* format, ...)
{
  va_list args;

  if (context) {
    uint32* errors = (timer ? &timer->errors : &context->errors);
    if (*errors & (1<<code))  return;
    *errors |= (1<<code);
  }
  printf("actimer>");
#if _ACTIMER_PTHREADS > 0
  // BOGUS: this could fail on some platforms
  printf(" thread x%lx", (unsigned long)pthread_self());
#endif
  if (context)
    printf(" module %s", context->name);
  if (timer)
    printf(" timer %s", timer->name);
  printf(": ");
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n");
  if (!context && code)
    exit(code);
}

void* actimer_alloc (void* p, size_t oldsize, size_t newsize)
{
  if (newsize <= oldsize)
    return p;
  p = realloc(p, newsize);
  if (p == NULL)
    actimer_error(3, NULL, NULL, "unable to allocate %zu bytes; "
		  " we're doomed\n", newsize);
  memset((char*)p + oldsize, 0, newsize - oldsize);
  return p;
}


/* PUBLIC ROUTINES IN ACTIMER_MAIN */

#if _ACTIMER_PTHREADS > 0

// Adds the data from the given actimer_thread object into the global
// aggregate actimer_thread object, but does not delete it (yet).

static void actimer_merge (actimer_thread_t* thread)
{
  actimer_thread_t* total;
  actimer_context_t *newc, *oldc, *spot;
  int n;

  if (thread->latest == 0)
    return;
  actimer_record_all(thread);

  pthread_mutex_lock(&_actimer_global.lock);
  {
    // Merge global data, creating a new structure if needed
    if (_actimer_global.total == NULL) {
      total = actimer_alloc(NULL, 0, sizeof(actimer_thread_t));
      total->base = actimer_alloc(NULL, 0, ACTIMER_SIZEOF(1));
      total->base->dynamic = 1;
      actimer_init_thread(total, 0);
      total->wall   = thread->wall;
      total->rusage = thread->rusage;
      _actimer_global.total = total;
    } else {
      total = _actimer_global.total;
      total->wall   = ACTIMER_MIN(total->wall, thread->wall);
      total->rusage = ACTIMER_MIN(total->rusage, thread->rusage);
    }

    // Aggregate the time intervals measured in microseconds.  Note that
    // 'used' is currently unused because in the usual threading model,
    // getrusage() already aggregates over threads.
    _actimer_global.elapsed += actimer_wall() - thread->wall;
    // _actimer_global.used += actimer_rusage() - thread->rusage;

    // Merge data from contexts
    newc = thread->base, oldc = total->base, spot = NULL;
    for (; newc != NULL; newc = newc->next) {
      int cmp = 0;
      while (oldc != NULL && (cmp = strcmp(newc->name, oldc->name)) > 0)
	spot = oldc, oldc = oldc->next;

      if (oldc == NULL || cmp < 0) {
	// Create an empty context
	oldc = actimer_alloc(NULL, 0, ACTIMER_SIZEOF(newc->n_timers));
	oldc->parent   = total;
	oldc->name     = newc->name;
	oldc->dynamic  = 1;
	oldc->n_timers = newc->n_timers;
	if (spot->next)
	  spot->next->prev = oldc;
	oldc->next = spot->next;
	oldc->prev = spot;
	spot->next = oldc;
      } else if (oldc->n_timers < newc->n_timers) {
	// Expand an old context
	oldc = actimer_alloc(oldc, ACTIMER_SIZEOF(oldc->n_timers),
			     ACTIMER_SIZEOF(newc->n_timers));
	if (oldc->next)
	  oldc->next->prev = oldc;
	if (oldc->prev)
	  oldc->prev->next = oldc;
	oldc->n_timers = newc->n_timers;
      }

      for (n = 0; n < newc->n_timers; n++) {
	oldc->timers[n].total_time += newc->timers[n].total_time;
	oldc->timers[n].exclu_time += newc->timers[n].exclu_time;
	oldc->timers[n].total_work += newc->timers[n].total_work;
	if (! oldc->timers[n].name)
	  oldc->timers[n].name = newc->timers[n].name;
      }
    }
  }
  pthread_mutex_unlock(&_actimer_global.lock);
}

static void actimer_destroy_thread (void* tsd)
{
  actimer_thread_t* actimer = (actimer_thread_t*) tsd;

  // Merge data into global structure before destroying it
  actimer_merge(actimer);

  while (actimer->base->next != NULL)
    actimer_free_context(actimer->base->next);
  if (actimer->base->dynamic)
    free(actimer->base);
  free(actimer->contexts);
  free(actimer);
}

void actimer_prepare_threads (void)
{
  // Make sure the key is only created once
  pthread_mutex_lock(&_actimer_global.lock);
  if (! _actimer_global.ready) {
    pthread_key_create(&_actimer_global.key, &actimer_destroy_thread);
    _actimer_global.ready = 1;
  }
  pthread_mutex_unlock(&_actimer_global.lock);
}

void actimer_aggregate (void)
{
  actimer_thread_t* actimer = actimer_get_thread();
  if (actimer->latest == 0)
    return;
  actimer_merge(actimer);
  actimer_reset_all(actimer);
}

#endif

int actimer_set_subtlety (int level)
{
  actimer_thread_t* actimer = actimer_get_thread();
  int old = actimer->subtlety;

  if (level >= 0)
    actimer->subtlety = level;
  return old;
}

double actimer_measure_tick (int cputime)
{
  actimer_thread_t* actimer = actimer_get_thread();
  int64 thread_ticks, start;

  // Make sure thread timer is running
  start = actimer->base->timers[0].start_time;
  if (start <= 0)
    return 0.0;

  thread_ticks = actimer_rtc() - start;
  if (thread_ticks <= 0)
    return 0.0;

  if (cputime && _ACTIMER_PTHREADS <= 0)
    return (actimer_rusage() - actimer->rusage) / (1.0e6 * thread_ticks);
  else
    return (actimer_wall() - actimer->wall) / (1.0e6 * thread_ticks);
}

// which: 1 = exclusive, 2 = inclusive, 3 = both (they are equal)
// 'globaltix' is the number of ticks elapsed on all threads;
// 'localtix' is the length, in ticks, of the interval covered
// by this report
static void actimer_report (actimer_thread_t* actimer, FILE* file,
			    const char* tag, double seconds_per_tick,
			    int64 localtix, int64 globaltix, int which)
{
  actimer_context_t* context;
  actimer_t *timer;
  const char* format;
  int64 time, work, sum_time;
  int n;

  fprintf (file, "%s> work-units    seconds   per-unit    %%time %s timer\n",
	   tag, (_ACTIMER_PTHREADS > 0 ? " %union " : ""));

  sum_time = 0;
  for (context = actimer->base->next; context; context = context->next) {
    int active = 0;
    for (n = 0; n < context->n_timers; n++) {
      timer = &context->timers[n];
      time = (which & 2) ? timer->total_time : timer->exclu_time;
      work = timer->total_work;
      if (!context->timers[n].name || (time == 0 && work == 0))
	continue;
      if (0 == active++)
	fprintf (file, "%s> module %s\n", tag, context->name);
      format = work > I64(9999999999) ? "%s> %-13.0f %#7.4g " : "%s> %10.0f %#10.4g ";
      fprintf (file, format, tag, (double)work, time * seconds_per_tick);
      fprintf (file, (work > 0 ? "%#10.4g " : "         * "),
	       (work > 0 ? time * seconds_per_tick / work : 0.0));
      fprintf (file, "%7.2f%% ", time * 100.0 / localtix);
#if _ACTIMER_PTHREADS > 0
      fprintf (file, "%6.2f%% ", time * 100.0 / globaltix);
#endif
      fprintf (file, " #%d %-s\n", n, timer->name);
      sum_time += time;
    }
  }

  for (n = 1; n <= 2; n++)
    if (which & n) {
      time = (n == 1 ? sum_time : actimer->base->timers[0].total_time);
      fprintf (file, "%s> %s   %#10.4g            %7.2f%%",
	       tag, (n == 1 ? "totals: " : "overall:"),
	       time * seconds_per_tick, time * 100.0 / localtix);
#if _ACTIMER_PTHREADS > 0
      fprintf (file, " %6.2f%%", time * 100.0 / globaltix);
#endif
      fprintf (file, "\n");
    }
  fprintf (file, "%s>\n", tag);
}

// Here 'elapsed' and 'used' are sums over threads, while actimer->wall
// and actimer->rusage are minimums over threads, and 'interval' is the
// length of time covered by this summary.  All are measured in microseconds.
static void actimer_print_summary (FILE* file, actimer_thread_t* actimer,
				   double elapsed, double used, double interval)
{
  const char tag[] = "actimer";
  actimer_context_t* context;
  double scale;
  int64 localtix, globaltix;
  int n, nested;

  if (actimer->base->next == NULL) {
    fprintf (file, "%s> nothing to summarize\n", tag);
    return;
  }

  elapsed  *= 1.0e-6;
  used     *= 1.0e-6;
  interval *= 1.0e-6;
  globaltix = actimer->base->timers[0].total_time;
  if (globaltix == 0 || elapsed < 0.5e-6) {
    fprintf (file, "%s> can't summarize: time interval too short\n", tag);
    return;
  }
  localtix = (int64) (globaltix * (interval / elapsed));
  scale = elapsed / globaltix;

  nested = 0;
  context = actimer->base->next;
  for (; context != NULL && !nested; context = context->next)
    for (n = 0; !nested && n < context->n_timers; n++)
      nested |= (context->timers[n].exclu_time !=
		 context->timers[n].total_time);

  if (!nested) {
    fprintf (file, "%s> Timings:\n", tag);
    actimer_report (actimer, file, tag, scale, localtix, globaltix, 3);
  } else {
    fprintf (file, "%s> Exclusive timings:\n", tag);
    actimer_report (actimer, file, tag, scale, localtix, globaltix, 1);
    fprintf (file, "%s> Inclusive timings:\n", tag);
    actimer_report (actimer, file, tag, scale, localtix, globaltix, 2);
  }

  fprintf (file, "%s> CPU time:  %#10.4g            %7.2f%%",
	   tag, used, 100.0 * used / interval);
#if _ACTIMER_PTHREADS > 0
  fprintf (file, " %6.2f%%\n%s> elapsed:   %#10.4g",
	   100.0 * used / elapsed, tag, interval);
#endif
  fprintf (file, "\n");
  fflush (file);
}

void actimer_summarize (FILE* file)
{
  actimer_thread_t* actimer = actimer_get_thread();
  double elapsed, used;

#if _ACTIMER_PTHREADS <= 0
  if (actimer->latest == 0)
    return;
  actimer_record_all(actimer);
  elapsed = actimer_wall() - actimer->wall;
  used = actimer_rusage() - actimer->rusage;
  actimer_print_summary(file, actimer, elapsed, used, elapsed);
  actimer_reset_all(actimer);
#else
  actimer_merge(actimer);
  pthread_mutex_lock(&_actimer_global.lock);
  {
    actimer_thread_t* total = _actimer_global.total;
    double interval = actimer_wall() - total->wall;
    elapsed = _actimer_global.elapsed;
    used    = actimer_rusage() - total->rusage;
    if (total->base->timers[0].total_time > 0)
      actimer_print_summary(file, total, elapsed, used, interval);
    _actimer_global.elapsed = 0.0;
    // _actimer_global.used    = 0.0;
    actimer_reset_all(total);
  }
  pthread_mutex_unlock(&_actimer_global.lock);
  if (actimer->latest != 0)
    actimer_reset_all(actimer);
#endif
}

#endif  // End: not defined(ACTIMER_DISABLE)
#endif  // End: routines for ACTIMER_MAIN only

#ifdef __cplusplus
}
#endif


// Possible enhancements:
//   * avoid relying on C99 features
//   * implement a "fast mode" with no error checking?
//     (in fast mode, actimer_initialize() would be mandatory)
//   * combine arguments to ACTIMERCTL for efficiency?
//   * everything private should have a name starting with underscore?
//   * time the timing function and subtract its overhead
//   * move subtlety test outside of subroutine?  should subtlety really
//     be local to a context?
