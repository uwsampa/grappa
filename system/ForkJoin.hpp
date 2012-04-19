#ifndef __FORK_JOIN_HPP__
#define __FORK_JOIN_HPP__

#include "SoftXMT.hpp"
#include "Addressing.hpp"
#include "tasks/Thread.hpp"
#include "Delegate.hpp"
#include "Tasking.hpp"
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

//#define VLOG(verboselevel) VLOG(verboselevel) << "<" << SoftXMT_mynode() << "> "

#define min(A,B) ( (A) < (B) ? (A) : (B))

DECLARE_int64(max_forkjoin_threads_per_node);

//struct range_t { int64_t start, end; };
//
//static range_t blockDist(int64_t start, int64_t end, int64_t rank, int64_t numBlocks) {
//	int64_t numElems = end-start;
//	int64_t each   = numElems / numBlocks,
//  remain = numElems % numBlocks;
//	int64_t mynum = (rank < remain) ? each+1 : each;
//	int64_t mystart = start + ((rank < remain) ? (each+1)*rank : (each+1)*remain + (rank-remain)*each);
//	range_t r = { mystart, mystart+mynum };
//  return r;
//}

class Semaphore {
protected:
  Thread * sleeper;
  int count;
  int total;
public:
  Semaphore(int total, int starting): total(total), count(starting), sleeper(NULL) {}
  void acquire_all(Thread * me) {
    while (count < total) {
      VLOG(3) << "Semaphore.count = " << count << " of " << total << ", suspending...";
      sleeper = me;
      SoftXMT_suspend();
    }
  }
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
//    int64_t n = reinterpret_cast<int64_t>(payload);
    int n = (int)*((int64_t*)payload);
    VLOG(3) << "am_release n=" << n;
    gaddr->pointer()->release((int)n);
  }
  static void release(GlobalAddress<Semaphore>* gaddr, int n) {
//    int64_t nn = n;
    VLOG(3) << "about to call on " << gaddr->node();
//    SoftXMT_call_on(gaddr->node(), &Semaphore::am_release, gaddr, sizeof(GlobalAddress<Semaphore>), reinterpret_cast<void*>(nn), 0);
    SoftXMT_call_on(gaddr->node(), &Semaphore::am_release, gaddr, sizeof(GlobalAddress<Semaphore>), &n, sizeof(int64_t));
  }
};

struct ForkJoinIteration {
  void operator()(int64_t index);
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
  T* func;
  size_t nthreads;
  size_t finished;
  Thread * node_th;
  size_t local_start;
  size_t local_end;
  
  forkjoin_data_t(Thread * me, T* f, int64_t start, int64_t end) {
    size_t each_n = (end-start); // / SoftXMT_nodes();
    local_start = start; //start + SoftXMT_mynode() * each_n;
    local_end = end; //local_start + each_n;
    func = f;
    nthreads = min(FLAGS_max_forkjoin_threads_per_node, each_n);
    finished = 0;
    node_th = me;
  }
};

struct iters_args {
  size_t rank;
  void* fjdata;
};

template<typename T>
//static void th_iters(Thread * me, iters_args* arg) {
static void task_iters(iters_args * arg) {
  forkjoin_data_t<T> * fj = static_cast<forkjoin_data_t<T>*>(arg->fjdata);
  range_t myblock = blockDist(fj->local_start, fj->local_end, arg->rank, fj->nthreads);
  VLOG(3) << "iters_block: " << myblock.start << " - " << myblock.end;
  
  for (int64_t i=myblock.start; i < myblock.end; i++) {
    (*fj->func)(i);
  }
  fj->finished++;
  if (fj->finished == fj->nthreads) {
    SoftXMT_wake(fj->node_th);
  }
}

#define TASK_FUNCTOR(name, members) \
  struct name { \
    AUTO_DECLS(members) \
    AUTO_CONSTRUCTOR( name, members ) \
    name() {} /* default constructor */ \
    inline void run(); \
  }; \
  inline void name::run()

#define TASK_FUNCTOR_TEMPLATED(T, name, members) \
template< typename T > \
struct name { \
AUTO_DECLS(members) \
AUTO_CONSTRUCTOR(name, members) \
name() {} /* default constructor */ \
inline void run(); \
}; \
template< typename T >\
inline void name<T>::run()


TASK_FUNCTOR_TEMPLATED(T, do_iters, ((size_t,rank)) ((void*,fjdata)) ) {
  forkjoin_data_t<T> * fj = static_cast<forkjoin_data_t<T>*>(fjdata);
  range_t myblock = blockDist(fj->local_start, fj->local_end, rank, fj->nthreads);
  VLOG(3) << "iters_block: " << myblock.start << " - " << myblock.end;
  
  for (int64_t i=myblock.start; i < myblock.end; i++) {
    (*fj->func)(i);
  }
  fj->finished++;
  if (fj->finished == fj->nthreads) {
    SoftXMT_wake(fj->node_th);
  }
}

template<typename T>
static void fork_join_onenode(T* func, int64_t start, int64_t end) {
  forkjoin_data_t<T> fj(CURRENT_THREAD, func, start, end);
  iters_args args[fj.nthreads];
//  do_iters<T> funcs[fj.nthreads];
//  Thread* ths[fj.nthreads];
  VLOG(2) << "fj.nthreads = " << fj.nthreads;
  
  for (int i=0; i<fj.nthreads; i++) {
    args[i].fjdata = &fj;
    args[i].rank = i;
    
//    ths[i] = SoftXMT_template_spawn(&th_iters<T>, &args[i]);
    SoftXMT_privateTask(&task_iters<T>, &args[i]);
  }
  while (fj.finished < fj.nthreads) SoftXMT_suspend();
  
//  for (int i=0; i<fj.nthreads; i++) {
//    destroy_thread(ths[i]);
//  }
}

template<typename T>
//static void th_node_fork_join(Thread * me, NodeForkJoinArgs<T>* a) {
static void th_node_fork_join(NodeForkJoinArgs<T>* a) {
  range_t myblock = blockDist(a->start, a->end, SoftXMT_mynode(), SoftXMT_nodes());
  VLOG(2) << "myblock: " << myblock.start << " - " << myblock.end;
  fork_join_onenode(&a->func, myblock.start, myblock.end);
  
  VLOG(2) << "about to update sem on " << a->sem.node();
  Semaphore::release(&a->sem, 1);
}

template<typename T>
static void fork_join(T* func, int64_t start, int64_t end) {
  Semaphore sem(SoftXMT_nodes(), 0);
  
  NodeForkJoinArgs<T> fj;
  fj.start = start;
  fj.end = end;
  fj.func = *func;
  fj.sem = make_global(&sem);
  
  for (Node i=0; i < SoftXMT_nodes(); i++) {
    SoftXMT_remote_privateTask(&th_node_fork_join, &fj, i);
    
    SoftXMT_flush(i); // TODO: remove this?
  }
  VLOG(2) << "waiting to acquire all";
  sem.acquire_all(CURRENT_THREAD);
  
  VLOG(2) << "fork_join done";
}

template<typename T>
//static void th_node_fork_join_custom(Thread * me, NodeForkJoinArgs<T>* a) {
static void th_node_fork_join_custom(NodeForkJoinArgs<T>* a) {
  a->func(SoftXMT_mynode());
  
  VLOG(2) << "about to update sem on " << a->sem.node();
  Semaphore::release(&a->sem, 1);
}

template<typename T>
static void fork_join_custom(T* func) {
  Semaphore sem(SoftXMT_nodes(), 0);
  
  NodeForkJoinArgs<T> fj;
  fj.start = 0;
  fj.end = 0;
  fj.func = *func;
  fj.sem = make_global(&sem);
  
  for (Node i=0; i < SoftXMT_nodes(); i++) {
    SoftXMT_remote_privateTask(&th_node_fork_join_custom, &fj, i);
    SoftXMT_flush(i); // TODO: remove this?
  }
  VLOG(2) << "waiting to acquire all";
  sem.acquire_all(CURRENT_THREAD);
  
  VLOG(2) << "fork_join done";
}

template< typename T>
struct parallel_loop_args {
  T* func;
  int64_t start;
  int64_t iterations;
  GlobalAddress<Thread> spawner;
  GlobalAddress<bool> done;
};

static void remote_wake_am(GlobalAddress<Thread>* remote_thread, size_t sz, void* payload, size_t psz) {
  assert(remote_thread->node() == SoftXMT_mynode());
  GlobalAddress<bool>* done = static_cast< GlobalAddress<bool>* >(payload);
  *done->pointer() = true;
  SoftXMT_wake(remote_thread->pointer());
}

template< typename T >
static void parallel_loop(T* func, int64_t start, int64_t iterations);

template< typename T>
static void parallel_loop_task(parallel_loop_args<T>* args) {
  parallel_loop(args->func, args->start, args->iterations);
  SoftXMT_call_on(&remote_wake_am, args->spawner, sizeof(GlobalAddress<Thread>), args->done, sizeof(GlobalAddress<bool>));
}

template< typename T >
static void parallel_loop(T* func, int64_t start, int64_t iterations) {
  if (iterations == 0) {
    return;
  } else if (iterations == 1) {
    func(start);
  } else {
    bool done = false;
    parallel_loop_args<T> a;
    a.func = func;
    a.start = start + (iterations+1)/2;
    a.iterations = iterations/2;
    a.spawner = make_global(CURRENT_THREAD);
    a.done = make_global(&done);
    SoftXMT_publicTask(&parallel_loop_task<T>, &a);
    
    parallel_loop(func, start, (iterations+1)/2);
    
    while (!done) SoftXMT_suspend();
  }
}

/// Create a functor for iterations of a loop. This automatically creates a struct which is a subtype of ForkJoinIteration, with the given "state" arguments as fields and a constructor with the fields enumerated in the given order.
/// 
/// Arguments:
///   name: struct which is created
///   index: name of variable used for loop iteration
///   state: seq of type/name pairs for functor state, of the form:
///     ((type1,name1)) ((type2,name2)) ...
///
/// Example:
///   LOOP_FUNCTOR(set_const, i, ((GlobalAddress<int64_t>,array)) ((int64_t,value)) ) {
///     SoftXMT_delegate_write_word(array+i, value);
///   }
#define LOOP_FUNCTOR(name, index_var,  members) \
struct name : ForkJoinIteration { \
AUTO_DECLS(members) \
AUTO_CONSTRUCTOR( name, members ) \
name() {} /* default constructor */\
inline void operator()(int64_t); \
}; \
inline void name::operator()(int64_t index_var)

LOOP_FUNCTOR(func_set_const, index, ((GlobalAddress<int64_t>,base_addr)) ((int64_t,value)) ) {
  SoftXMT_delegate_write_word(base_addr+index, value);
}







#endif /* define __FORK_JOIN_HPP__ */
