#ifndef __FORK_JOIN_HPP__
#define __FORK_JOIN_HPP__

#include "SoftXMT.hpp"
#include "Addressing.hpp"
#include "tasks/Thread.hpp"
#include "Delegate.hpp"
#include "Tasking.hpp"
#include "Cache.hpp"
#include "common.hpp"

#include <iostream>
#include <fstream>

#include <boost/static_assert.hpp>
#include <boost/type_traits/is_base_of.hpp>
#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/empty.hpp>
#include <boost/preprocessor/seq/transform.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

DECLARE_int64(max_forkjoin_threads_per_node);

/// Synchronization primitive that has similar semantics to a semaphore, where you start 
/// it with a certain number of tokens and can suspend waiting to acquire all of the tokens.
class Semaphore {
protected:
  int total;
  int count;
  Thread * sleeper;
public:
  Semaphore(int total, int starting): total(total), count(starting), sleeper(NULL) {}

  /// Suspend and wait for all tokens to be released.
  /// TODO: support multiple sleepers.
  void acquire_all(Thread * me) {
    while (count < total) {
      VLOG(3) << "Semaphore.count = " << count << " of " << total << ", suspending...";
      sleeper = me;
      SoftXMT_suspend();
    }
  }
  
  /// Release a number of tokens to Semaphore. If after this, all the tokens have been released,
  /// wake the sleeper.
  ///
  /// @param n Number of tokens to release.
  void release(int n=1) {
    count += n;
    if (count >= total && sleeper) {
      SoftXMT_wake(sleeper);
      sleeper = NULL;
    }
  }
  static void am_release(GlobalAddress<Semaphore>* gaddr, size_t sz, void* payload, size_t psz) {
    VLOG(3) << "in am_release()";
    assert(gaddr->node() == SoftXMT_mynode());
    int n = (int)*((int64_t*)payload);
    VLOG(3) << "am_release n=" << n;
    gaddr->pointer()->release((int)n);
  }

  /// Call release on remote semaphore. Sends an active message and potentially wakes the (remote) sleeper.
  ///
  /// Example usage:
  /// @code
  /// void task1(GlobalAddress<Semaphore> remote_sem) {
  ///   // do some work
  ///   Semaphore::release(&remote_sem, 1);
  /// }
  /// @endcode
  static void release(const GlobalAddress<Semaphore>* gaddr, int n) {
    VLOG(3) << "about to call on " << gaddr->node();
    SoftXMT_call_on(gaddr->node(), &Semaphore::am_release, gaddr, sizeof(GlobalAddress<Semaphore>), &n, sizeof(int64_t));
  }
};

/// A "joiner" is a synchronization primitive similar to a Semaphore, but with an unbounded 
/// number of potential tokens, meant to be used to join a number of tasks that isn't known 
/// from the start. In order to count as having outstanding work, a task must "register" itself.
/// A task that has called wait() will be woken when all "registered" calls have been matched by 
/// a "signal" call (so the number of outstanding tasks == 0).
///
/// This joiner is considered "Local" because it can only reside on a single node. However,
/// remoteSignal() can be used to send a signal to a LocalTaskJoiner on a different node.
struct LocalTaskJoiner {
  ThreadQueue wakelist;
  int64_t outstanding;
  LocalTaskJoiner(): outstanding(0) {}
  void reset() {
    outstanding = 0;
    while (!wakelist.empty()) SoftXMT_wake(wakelist.dequeue());
  }

  /// Tell joiner that there is one more task outstanding.
  void registerTask() {
    outstanding++;
  }

  /// Signal completion of a task. If it was the last outstanding task, then wake all tasks that
  /// have called wait().
  void signal() {
    if (outstanding == 0) {
      LOG(ERROR) << "too many calls to signal()";
    } else {
      outstanding--;
    }
    VLOG(3) << "barrier(outstanding=" << outstanding << ")";
    if (outstanding == 0) {
      while (!wakelist.empty()) {
        SoftXMT_wake(wakelist.dequeue());
      }
    }
  }

  /// Suspend task until all tasks have been completed. If no tasks have been registered or all have
  /// completed before wait is called, this will fall through and not suspend the calling task.
  void wait() {
    if (outstanding > 0) {
      wakelist.enqueue(CURRENT_THREAD);
      while (outstanding > 0) SoftXMT_suspend();
    }
  }
  static void am_remoteSignal(GlobalAddress<LocalTaskJoiner>* joinAddr, size_t sz, void* payload, size_t psz) {
    CHECK(joinAddr->node() == SoftXMT_mynode());
    joinAddr->pointer()->signal();
  }

  /// Call signal on task joiner that may be on another node.
  ///
  /// Usage:
  /// @code
  /// void task1(GlobalAddress<LocalTaskJoiner> rem_joiner) {
  ///   LocalTaskJoiner::remoteSignal(rem_joiner);
  /// }
  /// @endcode
  static void remoteSignal(GlobalAddress<LocalTaskJoiner> joinAddr) {
    if (joinAddr.node() == SoftXMT_mynode()) {
      joinAddr.pointer()->signal();
    } else {
      //VLOG(1) << "remoteSignal -> " << joinAddr.node();
      SoftXMT_call_on(joinAddr.node(), &LocalTaskJoiner::am_remoteSignal, &joinAddr);
    }
  }
};

/// Base class for iteration functors. Functors passed to ForkJoin should override operator()
/// in the way indicated in this struct.
struct ForkJoinIteration {
  void operator()(int64_t index) const;
};

template<typename T>
struct NodeForkJoinArgs {
  BOOST_STATIC_ASSERT((boost::is_base_of<ForkJoinIteration, T>::value));
  size_t start;
  size_t end;
  T func;
  GlobalAddress<Semaphore> sem;
};

template<typename T>
struct forkjoin_data_t {
  const T* func;
  size_t nthreads;
  size_t* finished;
  Thread * node_th;
  size_t local_start;
  size_t local_end;
  
  forkjoin_data_t(Thread * me, const T* f, int64_t start, int64_t end, size_t * finished) {
    size_t each_n = (end-start);
    local_start = start;
    local_end = end;
    func = f;
    nthreads = MIN(FLAGS_max_forkjoin_threads_per_node, each_n);
    this->finished = finished;
    node_th = me;
  }
};

struct iters_args {
  size_t rank;
  const void* fjdata;
};

template<typename T>
void task_iters(iters_args * arg) {
  const forkjoin_data_t<T> * fj = static_cast<const forkjoin_data_t<T>*>(arg->fjdata);
  range_t myblock = blockDist(fj->local_start, fj->local_end, arg->rank, fj->nthreads);
  VLOG(4) << "iters_block: " << myblock.start << " - " << myblock.end;
  
  for (int64_t i=myblock.start; i < myblock.end; i++) {
    (*fj->func)(i);
  }
  (*fj->finished)++;
  if (*fj->finished == fj->nthreads) {
    SoftXMT_wake(fj->node_th);
  }
}

/// Parallel loop on a single node using private tasks and *not* recursive decomposition.
/// Iterations get split evenly among tasks created by this function, the calling task is 
/// suspended until those tasks complete.
///
/// Uses gflag: `max_forkjoin_threads_per_node` to determine the number of tasks to spawn.
/// 
/// TODO: update for new Grappa
///
/// @param func Pointer to a functor object that extends ForkJoinIteration. This will be shared by all tasks created by this fork/join.
/// @param start Starting iteration number, will be the first task's first iteration.
/// @param end Ending iteration number.
template<typename T>
void fork_join_onenode(const T* func, int64_t start, int64_t end) {
  size_t finished = 0;
  
  forkjoin_data_t<T> fj(CURRENT_THREAD, func, start, end, &finished);
  iters_args args[fj.nthreads];
  VLOG(2) << "fj.nthreads = " << fj.nthreads;
  
  for (int i=0; i<fj.nthreads; i++) {
    args[i].fjdata = &fj;
    args[i].rank = i;
    
    SoftXMT_privateTask(task_iters<T>, &args[i]);
  }
  while (*fj.finished < fj.nthreads) SoftXMT_suspend();
  
}

/// Internal: Task for doing fork_join_onenode on a node.
template<typename T>
void th_node_fork_join(const NodeForkJoinArgs<T>* a) {
  range_t myblock = blockDist(a->start, a->end, SoftXMT_mynode(), SoftXMT_nodes());
  VLOG(2) << "myblock: " << myblock.start << " - " << myblock.end;
  fork_join_onenode(&a->func, myblock.start, myblock.end);
  
  VLOG(2) << "about to update sem on " << a->sem.node();
  Semaphore::release(&a->sem, 1);
}

/// Essentially a parallel for loop that spawns many private tasks across nodes in the 
/// system and has each task execute a chunk of iterations of the loop. It is essentially
/// 'fork_join_onenode' inside a 'fork_join_custom'.
///
/// TODO: update this for new Grappa (still works, but is definitely suboptimal, 
/// in particular, does too much copying and uses an old tasking interface.
template<typename T>
void fork_join(T* func, int64_t start, int64_t end) {
  Semaphore sem(SoftXMT_nodes(), 0);
  
  NodeForkJoinArgs<T> fj;
  fj.start = start;
  fj.end = end;
  fj.func = *func;
  fj.sem = make_global(&sem);
  
  for (Node i=0; i < SoftXMT_nodes(); i++) {
    SoftXMT_remote_privateTask(CACHE_WRAP(th_node_fork_join, &fj), i);
    
    SoftXMT_flush(i); // TODO: remove this?
  }
  VLOG(2) << "waiting to acquire all";
  sem.acquire_all(CURRENT_THREAD);
  
  VLOG(2) << "fork_join done";
}

template<typename T>
void th_node_fork_join_custom(const NodeForkJoinArgs<T>* a) {
  a->func(SoftXMT_mynode());
  
  VLOG(2) << "about to update sem on " << a->sem.node();
  Semaphore::release(&a->sem, 1);
}

template<typename T>
void fork_join_custom(T* func) {
  Semaphore sem(SoftXMT_nodes(), 0);
  
  NodeForkJoinArgs<T> fj;
  fj.start = 0;
  fj.end = 0;
  fj.func = *func;
  fj.sem = make_global(&sem);
  
  for (Node i=0; i < SoftXMT_nodes(); i++) {
    SoftXMT_remote_privateTask(CACHE_WRAP(th_node_fork_join_custom, &fj), i);
    SoftXMT_flush(i); // TODO: remove this?
  }
  VLOG(2) << "waiting to acquire all";
  sem.acquire_all(CURRENT_THREAD);
  
  VLOG(2) << "fork_join done";
}

/// Create a functor for iterations of a loop. This automatically creates a struct which is a
/// subtype of ForkJoinIteration, with the given "state" arguments as fields and a constructor
/// with the fields enumerated in the given order.
/// 
/// @param name Struct which is created
/// @param index Name of variable used for loop iteration
/// @param state Seq of type/name pairs for functor state, of the form:
///     ((type1,name1)) ((type2,name2)) ...
///
/// Example:
/// \code
///    LOOP_FUNCTOR(set_all, i, ((GlobalAddress<int64_t>,array)) ((int64_t,value)) ) {
///      SoftXMT_delegate_write_word(array+i, value);
///    }
///    ...
///    void user_main() {
///      set_all mysetfunc(array, 2);
///      fork_join(&array, 0, array_size);
///    }
/// \endcode
#define LOOP_FUNCTOR(name, index_var, members) \
struct name : ForkJoinIteration { \
AUTO_DECLS(members) \
AUTO_CONSTRUCTOR( name, members ) \
name() {} /* default constructor */\
inline void operator()(int64_t) const; \
}; \
inline void name::operator()(int64_t index_var) const

/// Like LOOP_FUNCTOR but with allowing an additional template parameter.
/// @see LOOP_FUNCTOR()
#define LOOP_FUNCTOR_TEMPLATED(T, name, index_var, members) \
template< typename T > \
struct name : ForkJoinIteration { \
AUTO_DECLS(members) \
AUTO_CONSTRUCTOR( name, members ) \
name() {} /* default constructor */\
inline void operator()(int64_t) const; \
}; \
template< typename T > \
inline void name<T>::operator()(int64_t index_var) const

/// Like LOOP_FUNCTOR but with no arguments passed and saved in functor (must have different
/// name because of limitations of C macros)
/// @see LOOP_FUNCTOR
#define LOOP_FUNCTION(name, index_var) \
struct name : ForkJoinIteration { \
inline void operator()(int64_t) const; \
}; \
inline void name::operator()(int64_t index_var) const

struct ConstReplyArgs {
  int64_t replies_left;
  Thread * sleeper;
};

template< typename T >
struct ConstRequestArgs {
  GlobalAddress<T> addr;
  size_t count;
  T value;
  GlobalAddress<ConstReplyArgs> reply;
};

static void memset_reply_am(GlobalAddress<ConstReplyArgs> * reply, size_t sz, void * payload, size_t psz) {
  CHECK(reply->node() == SoftXMT_mynode());
  ConstReplyArgs * r = reply->pointer();
  (r->replies_left)--;
  if (r->replies_left == 0) {
    SoftXMT_wake(r->sleeper);
  }
}

template< typename T >
static void memset_request_am(ConstRequestArgs<T> * args, size_t sz, void* payload, size_t psz) {
  CHECK(args->addr.node() == SoftXMT_mynode()) << "args->addr.node() = " << args->addr.node();
  T * ptr = args->addr.pointer();
  for (size_t i=0; i<args->count; i++) {
    ptr[i] = args->value;
  }
  SoftXMT_call_on(args->reply.node(), &memset_reply_am, &args->reply);
}

/// Initialize an array of elements of generic type with a given value.
/// 
/// This version sends a large number of active messages, the same way as the Incoherent
/// releaser, to set each part of a global array. In theory, this version should be able
/// to be called from multiple locations at the same time (to initialize different regions of global memory).
/// 
/// @param base Base address of the array to be set.
/// @param value Value to set every element of array to (will be copied to all the nodes)
/// @param count Number of elements to set, starting at the base address.
template< typename T >
static void SoftXMT_memset(GlobalAddress<T> request_address, T value, size_t count) {
  size_t offset = 0;
  size_t request_bytes = 0;
  
  ConstReplyArgs reply;
  reply.replies_left = 0;
  reply.sleeper = CURRENT_THREAD;
  
  ConstRequestArgs<T> args;
  args.addr = request_address;
  args.value = value;
  args.reply = make_global(&reply);
  
  for (size_t total_bytes = count*sizeof(T); offset < total_bytes; offset += request_bytes) {
    // compute number of bytes remaining in the block containing args.addr
    request_bytes = (args.addr.first_byte().block_max() - args.addr.first_byte());
    if (request_bytes > total_bytes - offset) {
      request_bytes = total_bytes - offset;
    }
    CHECK(request_bytes % sizeof(T) == 0);
    args.count = request_bytes / sizeof(T);
    
    reply.replies_left++;
    SoftXMT_call_on(args.addr.node(), &memset_request_am, &args);
    
    args.addr += args.count;
  }
  
  while (reply.replies_left > 0) SoftXMT_suspend();
}

LOOP_FUNCTOR_TEMPLATED(T, memset_func, nid, ((GlobalAddress<T>,base)) ((T,value)) ((size_t,count))) {
  T * local_base = base.localize(), * local_end = (base+count).localize();
  for (size_t i=0; i<local_end-local_base; i++) {
    local_base[i] = value;
  }
}

/// Does memset across a global array using a single task on each node and doing local assignments
/// Uses 'GlobalAddress::localize()' to determine the range of actual memory from the global array
/// on a particular node.
/// 
/// Must be called by itself (preferably from the user_main task) because it contains a call to
/// fork_join_custom().
///
/// @see SoftXMT_memset()
///
/// @param base Base address of the array to be set.
/// @param value Value to set every element of array to (will be copied to all the nodes)
/// @param count Number of elements to set, starting at the base address.
template< typename T >
void SoftXMT_memset_local(GlobalAddress<T> base, T value, size_t count) {
  {
    memset_func<T> f(base, value, count);
    fork_join_custom(&f);
  }
}

#include "AsyncParallelFor.hpp"

/// Task joiner used internally by forall_local to join private tasks on all the nodes.
extern LocalTaskJoiner ljoin;

template< typename S, void (*F)(int64_t,int64_t,S), int64_t Threshold >
void spawn_private_task(int64_t a, int64_t b, S c);

template< typename S, void (*F)(int64_t,int64_t,S), int64_t Threshold >
void apfor_with_local_join(int64_t a, int64_t b, S c) {
  async_parallel_for<S, F, spawn_private_task<S,F,Threshold>, Threshold>(a, b, c);
  ljoin.signal();
}

template< typename S, void (*F)(int64_t,int64_t,S), int64_t Threshold >
void spawn_private_task(int64_t a, int64_t b, S c) {
  ljoin.registerTask();
  SoftXMT_privateTask( &apfor_with_local_join<S, F, Threshold>, a, b, c );
}

/// Version of AsyncParallelFor that spawns private tasks and joins all of them.
template< typename S, void (*F)(int64_t,int64_t,S), int64_t Threshold >
void async_parallel_for_private(int64_t start, int64_t iters, S shared_arg) {
  ljoin.reset();
  async_parallel_for< S, F, spawn_private_task<S,F,Threshold>, Threshold>(start, iters, shared_arg);
  ljoin.wait();
}

template< typename T, void F(T*) >
void for_iterations_task(int64_t start, int64_t niters, T * base) {
  //VLOG(1) << "for_local @ " << base << " ^ " << start << " # " << niters;
  for (int64_t i=start; i<start+niters; i++) {
    F(&base[i]);
  }
}

template< typename T, void F(T*), int64_t Threshold >
struct for_local_func : ForkJoinIteration {
  GlobalAddress<T> base;
  size_t nelems;
  void operator()(int64_t nid) const {
    T * local_base = base.localize(),
	  * local_end = (base+nelems).localize();
    async_parallel_for_private< T*, for_iterations_task<T,F>, Threshold >(0, local_end-local_base, local_base);
  }
};

/// Iterator over a global array where each node does only the iterations corresponding to elements
/// local to it. Each node does a parallel recursive decomposition over its local elements. Most 
/// communication operations, including delegate ops should be able to be used, with the exception
/// of stop-the-world sync ops that assume they're being called from user_main or a SIMD context.
///
/// Limitations:
///  - the element type must be evenly divisible by the block_size, otherwise some elements may
///    span node boundaries, causing them to not be completely local to any node.
///  - To be called from user_main task because only one can be in flight at a time.
///
/// @param base Base address (linear) of global array to iterate over.
/// @param nelems Number of elements to iterate over.
/// @tparam T Type of the elements of the array
/// @tparam F A function that takes a T* which will be called with a pointer for each element of the 
///           array (on all nodes). This value can be assumed to be local and can be safely modified.
/// @tparam Threshold Same as async_parallel_for threshold.
template< typename T, void F(T*), int64_t Threshold >
void forall_local(GlobalAddress<T> base, size_t nelems) {
  for_local_func<T,F,Threshold> f;
  f.base = base;
  f.nelems = nelems;
  fork_join_custom(&f);
}
// duplicated to handle default template args (unnecessary with c++11)
template< typename T, void F(T*) >
void forall_local(GlobalAddress<T> base, size_t nelems) {
  forall_local<T,F,ASYNC_PAR_FOR_DEFAULT>(base, nelems);
}

#endif /* define __FORK_JOIN_HPP__ */
