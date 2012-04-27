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

#define min(A,B) ( (A) < (B) ? (A) : (B))

DECLARE_int64(max_forkjoin_threads_per_node);

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
    int n = (int)*((int64_t*)payload);
    VLOG(3) << "am_release n=" << n;
    gaddr->pointer()->release((int)n);
  }
  static void release(GlobalAddress<Semaphore>* gaddr, int n) {
    VLOG(3) << "about to call on " << gaddr->node();
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
    size_t each_n = (end-start);
    local_start = start;
    local_end = end;
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
void task_iters(iters_args * arg) {
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


template<typename T>
void fork_join_onenode(T* func, int64_t start, int64_t end) {
  forkjoin_data_t<T> fj(CURRENT_THREAD, func, start, end);
  iters_args args[fj.nthreads];
  VLOG(2) << "fj.nthreads = " << fj.nthreads;
  
  for (int i=0; i<fj.nthreads; i++) {
    args[i].fjdata = &fj;
    args[i].rank = i;
    
    SoftXMT_privateTask(CACHE_WRAP(task_iters<T>, &args[i]));
  }
  while (fj.finished < fj.nthreads) SoftXMT_suspend();
  
}

template<typename T>
void th_node_fork_join(NodeForkJoinArgs<T>* a) {
  range_t myblock = blockDist(a->start, a->end, SoftXMT_mynode(), SoftXMT_nodes());
  VLOG(2) << "myblock: " << myblock.start << " - " << myblock.end;
  fork_join_onenode(&a->func, myblock.start, myblock.end);
  
  VLOG(2) << "about to update sem on " << a->sem.node();
  Semaphore::release(&a->sem, 1);
}

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
void th_node_fork_join_custom(NodeForkJoinArgs<T>* a) {
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
#define LOOP_FUNCTOR(name, index_var, members) \
struct name : ForkJoinIteration { \
AUTO_DECLS(members) \
AUTO_CONSTRUCTOR( name, members ) \
name() {} /* default constructor */\
inline void operator()(int64_t); \
}; \
inline void name::operator()(int64_t index_var)

#define LOOP_FUNCTION(name, index_var) \
struct name : ForkJoinIteration { \
inline void operator()(int64_t); \
}; \
inline void name::operator()(int64_t index_var)

LOOP_FUNCTOR(func_set_const, index, ((GlobalAddress<int64_t>,base_addr)) ((int64_t,value)) ) {
  SoftXMT_delegate_write_word(base_addr+index, value);
}


#endif /* define __FORK_JOIN_HPP__ */
