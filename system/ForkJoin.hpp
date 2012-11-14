
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __FORK_JOIN_HPP__
#define __FORK_JOIN_HPP__

#include "Grappa.hpp"
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
      Grappa_suspend();
    }
  }
  
  /// Release a number of tokens to Semaphore. If after this, all the tokens have been released,
  /// wake the sleeper.
  ///
  /// @param n Number of tokens to release.
  void release(int n=1) {
    count += n;
    if (count >= total && sleeper) {
      Grappa_wake(sleeper);
      sleeper = NULL;
    }
  }
  static void am_release(GlobalAddress<Semaphore>* gaddr, size_t sz, void* payload, size_t psz) {
    VLOG(3) << "in am_release()";
    assert(gaddr->node() == Grappa_mynode());
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
    Grappa_call_on(gaddr->node(), &Semaphore::am_release, gaddr, sizeof(GlobalAddress<Semaphore>), &n, sizeof(int64_t));
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
    while (!wakelist.empty()) Grappa_wake(wakelist.dequeue());
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
        Grappa_wake(wakelist.dequeue());
      }
    }
  }

  /// Suspend task until all tasks have been completed. If no tasks have been registered or all have
  /// completed before wait is called, this will fall through and not suspend the calling task.
  void wait() {
    if (outstanding > 0) {
      wakelist.enqueue(CURRENT_THREAD);
      while (outstanding > 0) Grappa_suspend();
    }
  }
  static void am_remoteSignal(GlobalAddress<LocalTaskJoiner>* joinAddr, size_t sz, void* payload, size_t psz) {
    CHECK(joinAddr->node() == Grappa_mynode());
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
    if (joinAddr.node() == Grappa_mynode()) {
      joinAddr.pointer()->signal();
    } else {
      //VLOG(1) << "remoteSignal -> " << joinAddr.node();
      Grappa_call_on(joinAddr.node(), &LocalTaskJoiner::am_remoteSignal, &joinAddr);
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
    Grappa_wake(fj->node_th);
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
    
    Grappa_privateTask(task_iters<T>, &args[i]);
  }
  while (*fj.finished < fj.nthreads) Grappa_suspend();
  
}

/// Internal: Task for doing fork_join_onenode on a node.
template<typename T>
void th_node_fork_join(const NodeForkJoinArgs<T>* a) {
  range_t myblock = blockDist(a->start, a->end, Grappa_mynode(), Grappa_nodes());
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
  Semaphore sem(Grappa_nodes(), 0);
  
  NodeForkJoinArgs<T> fj;
  fj.start = start;
  fj.end = end;
  fj.func = *func;
  fj.sem = make_global(&sem);
  
  for (Node i=0; i < Grappa_nodes(); i++) {
    Grappa_remote_privateTask(CACHE_WRAP(th_node_fork_join, &fj), i);
    
    Grappa_flush(i); // TODO: remove this?
  }
  VLOG(2) << "waiting to acquire all";
  sem.acquire_all(CURRENT_THREAD);
  
  VLOG(2) << "fork_join done";
}

template<typename T>
void th_node_fork_join_custom(const NodeForkJoinArgs<T>* a) {
  a->func(Grappa_mynode());
  
  VLOG(2) << "about to update sem on " << a->sem.node();
  Semaphore::release(&a->sem, 1);
}

template<typename T>
void fork_join_custom(T* func) {
  Semaphore sem(Grappa_nodes(), 0);
  
  NodeForkJoinArgs<T> fj;
  fj.start = 0;
  fj.end = 0;
  fj.func = *func;
  fj.sem = make_global(&sem);
  
  for (Node i=0; i < Grappa_nodes(); i++) {
    Grappa_remote_privateTask(CACHE_WRAP(th_node_fork_join_custom, &fj), i);
    Grappa_flush(i); // TODO: remove this?
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
///      Grappa_delegate_write_word(array+i, value);
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


#include "AsyncParallelFor.hpp"
#include "GlobalTaskJoiner.hpp"

/// Task joiner used internally by forall_local to join private tasks on all the nodes.
extern LocalTaskJoiner ljoin;

template< typename S, void (*F)(int64_t,int64_t,S), int64_t Threshold, bool UseGlobalJoin >
void spawn_private_task(int64_t a, int64_t b, S c);

template< typename S, void (*F)(int64_t,int64_t,S), int64_t Threshold, bool UseGlobalJoin >
void apfor_with_local_join(int64_t a, int64_t b, S c) {
  async_parallel_for<S, F, spawn_private_task<S,F,Threshold,UseGlobalJoin>, Threshold>(a, b, c);
  if (UseGlobalJoin) global_joiner.signal(); else ljoin.signal();
}

template< typename S, void (*F)(int64_t,int64_t,S), int64_t Threshold, bool UseGlobalJoin >
void spawn_private_task(int64_t a, int64_t b, S c) {
  if (UseGlobalJoin) global_joiner.registerTask(); else ljoin.registerTask();
  Grappa_privateTask( &apfor_with_local_join<S, F, Threshold, UseGlobalJoin>, a, b, c );
}


/// Version of AsyncParallelFor that spawns private tasks and joins all of them.
template< typename S, void (*F)(int64_t,int64_t,S), int64_t Threshold, bool UseGlobalJoin >
void async_parallel_for_private(int64_t start, int64_t iters, S shared_arg) {
  async_parallel_for< S, F, spawn_private_task<S,F,Threshold,UseGlobalJoin>, Threshold>(start, iters, shared_arg);
}

template< typename T, void F(T*) >
void for_iterations_task(int64_t start, int64_t niters, T * base) {
  //VLOG(1) << "for_local @ " << base << " ^ " << start << " # " << niters;
  for (int64_t i=start; i<start+niters; i++) {
    F(&base[i]);
  }
}

template< typename T, void F(T*), int64_t Threshold, bool UseGlobalJoin >
struct for_local_func : ForkJoinIteration {
  GlobalAddress<T> base;
  size_t nelems;
  for_local_func() {}
  for_local_func(GlobalAddress<T> base, size_t nelems): base(base), nelems(nelems) {}
  void operator()(int64_t nid) const {
    T * local_base = base.localize(),
	  * local_end = (base+nelems).localize();
    if (UseGlobalJoin) global_joiner.reset(); else ljoin.reset();
    async_parallel_for_private< T*, for_iterations_task<T,F>, Threshold, UseGlobalJoin >(0, local_end-local_base, local_base);
    if (UseGlobalJoin) global_joiner.wait(); else ljoin.wait();
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
template< typename T, void F(T*), int64_t Threshold, bool UseGlobalJoin >
void forall_local(GlobalAddress<T> base, size_t nelems) {
  for_local_func<T,F,Threshold,UseGlobalJoin> f(base, nelems);
  fork_join_custom(&f);
}
/// Duplicate of forall_local with default Threshold template arg. @see forall_local()
/// (duplication unnecessary with c++11)
template< typename T, void F(T*), int64_t Threshold >
void forall_local(GlobalAddress<T> base, size_t nelems) {
  forall_local<T,F,Threshold,true>(base, nelems);
}
/// Duplicate of forall_local with default Threshold template arg. @see forall_local()
/// (duplication unnecessary with c++11)
template< typename T, void F(T*) >
void forall_local(GlobalAddress<T> base, size_t nelems) {
  forall_local<T,F,ASYNC_PAR_FOR_DEFAULT>(base, nelems);
}

//#include <map>
//#include <utility>

//typedef std::pair<intptr_t,intptr_t> packed_pair;
//extern std::map<uint64_t,packed_pair> unpacking_map;

//inline uint64_t packing_hash(const packed_pair& p) {
  //return p.first + p.second * 8;
//}
template< typename S, void (*F)(int64_t,int64_t,S*), int64_t Threshold, bool UseGlobalJoin >
void spawn_local_task(int64_t a, int64_t b, S* c);

template< typename S, void (*F)(int64_t,int64_t,S*), int64_t Threshold, bool UseGlobalJoin >
void apfor_local(int64_t a, int64_t b, S* c) {
  async_parallel_for<S*, F, spawn_local_task<S,F,Threshold,UseGlobalJoin>, Threshold>(a, b, c);
  if (UseGlobalJoin) global_joiner.signal(); else ljoin.signal();
}

template< typename S, void (*F)(int64_t,int64_t,S*), int64_t Threshold, bool UseGlobalJoin >
void spawn_local_task(int64_t a, int64_t b, S* c) {
  if (UseGlobalJoin) global_joiner.registerTask(); else ljoin.registerTask();
  c->incrRefs();
  Grappa_privateTask( &apfor_local<S, F, Threshold, UseGlobalJoin>, a, b, c );
}


/// Version of AsyncParallelFor that spawns private tasks and joins all of them.
template< typename S, void (*F)(int64_t,int64_t,S*), int64_t Threshold, bool UseGlobalJoin >
void async_parallel_for_local(int64_t start, int64_t iters, S* shared_arg) {
  async_parallel_for<S*, F, spawn_local_task<S,F,Threshold,UseGlobalJoin>, Threshold>(start, iters, shared_arg);
}

template< typename T, typename P >
struct LocalForArgs {
  int64_t refs;
  T * base;
  P extra;
  GlobalAddress<T> base_addr;
  LocalForArgs(T* base, P extra): base(base), extra(extra), refs(0) {}
  void incrRefs() { refs++; /*if (extra == 13701) VLOG(1) << "++ " << refs;*/ }
  void decrRefs() { refs--; /*if (extra == 13701) VLOG(1) << "-- " << refs;*/ /*if (refs == 0) delete this;*/ }
};

/// Note: this allows being cast to (void*), but requires that the void* is only used once after being cast back to a SharedArgsPtr.
//template< typename T, typename P >
//struct SharedArgsPtr {
  //LocalForArgs<T,P> * ptr;
  //SharedArgsPtr(LocalForArgs<T,P> * ptr): ptr(ptr) { ptr->incrRefs(); }
  //SharedArgsPtr(const SharedArgsPtr<T,P>& a): ptr(a.ptr) { ptr->incrRefs(); }

  //// incremented when cast as void*, so don't increment when casting back
  //SharedArgsPtr(void * ptr): ptr((LocalForArgs<T,P>*)ptr) { }

  //~SharedArgsPtr() { ptr->decrRefs(); }

  //LocalForArgs<T,P>* operator->() const { return ptr; }
  //LocalForArgs<T,P>* operator*() const { return ptr; }

  //// increment before passing around as void*
  //operator void*() { this->ptr->incrRefs(); return (void*)ptr; }
//};

template< typename T, typename P, void F(int64_t,T*,const P&) >
void for_async_iterations_task(int64_t start, int64_t niters, LocalForArgs<T,P> * args) {
  //SharedArgsPtr<T,P> args(v_args);
  //VLOG(1) << "for_local @ " << base << " ^ " << start << " # " << niters;
  CHECK( args->base_addr.localize() == args->base );

  T * base = args->base;

  //if (args->extra == 13701) {
    //std::stringstream ss; ss << "neighbors[" << start << "::" << niters << "] =";
    //for (int64_t i=start; i<start+niters; i++) { ss << " " << args->base[i]; }
    //VLOG(1) << ss.str();
    //VLOG(1) << "args->base = " << args->base << ", base = " << base;
  //}

  for (int64_t i=start; i<start+niters; i++) {
    F(i, base, args->extra);
  }
  args->decrRefs();
}

/// Does same as `for_local_func`, but for use with `forall_local_async`
template< typename T, typename P, void F(int64_t,T*,const P&), int64_t Threshold >
void forall_local_async_task(GlobalAddress<T> base, size_t nelems, GlobalAddress<P> extra) {
  Node spawner = extra.node();
  P extra_buf; typename Incoherent<P>::RO extra_c(extra, 1, &extra_buf);

  T * local_base = base.localize();
  T * local_end = (base+nelems).localize();
  
  if (local_end > local_base) {
    //CHECK( local_end - local_base < nelems ) << "local_base: " << local_base << ", local_end: " << local_end << ", base+nelems: " << base+nelems;
    //packed_pair p = std::make_pair((intptr_t)local_base, extra);
    //uint64_t code = packing_hash(p);
    //CHECK( unpacking_map.count(code) == 0 );
    //unpacking_map[code] = p;

    LocalForArgs<T,P> * args = new LocalForArgs<T,P>(local_base, *extra_c);
    //SharedArgsPtr<T,P> args( new LocalForArgs<T,P>(local_base, *extra_c) );
    args->base_addr = base;
    args->incrRefs();

    //if (*extra_c == 33707) { VLOG(1) << "local_base: " << local_base << " .. " << local_end-local_base; }

    async_parallel_for_local<LocalForArgs<T,P>, for_async_iterations_task<T,P,F>, Threshold, true>
                              (0, local_end-local_base, args);
  }
  global_joiner.remoteSignalNode(spawner);
}

LOOP_FUNCTION(func_global_barrier, nid) {
  global_joiner.wait();
}

/// Asyncronously spawn remote tasks on nodes that *may contain elements*.
/// @param extra GlobalAddress of an extra argument that will be cached by each remote task.
///              *Must be an address **on the current node** because it uses this address to
///              know where it was spawned from(where to send global_joiner signal back to).*
template< typename T, typename P, void F(int64_t,T*,const P&), int64_t Threshold, bool Blocking >
void forall_local_async(GlobalAddress<T> base, size_t nelems, GlobalAddress<P> extra) {
  Node nnode = Grappa_nodes();

  Node fnodes;
  
  int64_t nbytes = nelems*sizeof(T);
  GlobalAddress<T> end = base+nelems;
  
  if (nelems > 0) {
    fnodes = 1;
  }

  size_t block_elems = block_size / sizeof(T);
  int64_t nfirstnode = base.block_max() - base;
  
  int64_t n = nelems - nfirstnode;
  
  if (n > 0) {
    int64_t nrest = n / block_elems;
    if (nrest >= nnode-1) {
      fnodes = nnode;
    } else {
      fnodes += nrest;
      if ((end - end.block_min()) && end.node() != base.node()) {
        fnodes += 1;
      }
    }
  }

  Node start_node = base.node();

  for (Node i=0; i<fnodes; i++) {
    global_joiner.registerTask();
    Grappa_remote_privateTask(forall_local_async_task<T,P,F,Threshold>, base, nelems, extra,
        (start_node+i)%nnode);
  }
  if (Blocking) { func_global_barrier f; fork_join_custom(&f); }
}

/// Duplicate for lack of default template arg.
template< typename T, typename P, void F(int64_t,T*,const P&), int64_t Threshold >
void forall_local_async(GlobalAddress<T> base, size_t nelems, GlobalAddress<P> extra) {
  forall_local_async<T,P,F,Threshold,false>(base, nelems, extra);
}

/// Duplicate for lack of default template arg.
template< typename T, typename P, void F(int64_t,T*,const P&) >
void forall_local_async(GlobalAddress<T> base, size_t nelems, GlobalAddress<P> extra) {
  forall_local_async<T,P,F,ASYNC_PAR_FOR_DEFAULT>(base, nelems, extra);
}


template< typename T, typename P, void F(int64_t,T*,const P&),int64_t Threshold>
void forall_local_blocking(GlobalAddress<T> base, size_t nelems, GlobalAddress<P> extra) {
  forall_local_async<T,P,F,Threshold,true>(base, nelems, extra);
}
template< typename T, typename P, void F(int64_t,T*,const P&)>
void forall_local_blocking(GlobalAddress<T> base, size_t nelems, GlobalAddress<P> extra) {
  forall_local_blocking<T,P,F,ASYNC_PAR_FOR_DEFAULT>(base, nelems, extra);
}
#endif /* define __FORK_JOIN_HPP__ */

