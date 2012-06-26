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
  static void release(const GlobalAddress<Semaphore>* gaddr, int n) {
    VLOG(3) << "about to call on " << gaddr->node();
    SoftXMT_call_on(gaddr->node(), &Semaphore::am_release, gaddr, sizeof(GlobalAddress<Semaphore>), &n, sizeof(int64_t));
  }
};

struct LocalTaskJoiner {
  ThreadQueue wakelist;
  int64_t outstanding;
  LocalTaskJoiner(): outstanding(0) {}
  void reset() {
    outstanding = 0;
    while (!wakelist.empty()) SoftXMT_wake(wakelist.dequeue());
  }
  void registerTask() {
    outstanding++;
  }
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
  static void remoteSignal(GlobalAddress<LocalTaskJoiner> joinAddr) {
    if (joinAddr.node() == SoftXMT_mynode()) {
      joinAddr.pointer()->signal();
    } else {
      //VLOG(1) << "remoteSignal -> " << joinAddr.node();
      SoftXMT_call_on(joinAddr.node(), &LocalTaskJoiner::am_remoteSignal, &joinAddr);
    }
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
    nthreads = min(FLAGS_max_forkjoin_threads_per_node, each_n);
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

template<typename T>
void th_node_fork_join(const NodeForkJoinArgs<T>* a) {
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

/// Create a functor for iterations of a loop. This automatically creates a struct which is a subtype of ForkJoinIteration, with the given "state" arguments as fields and a constructor with the fields enumerated in the given order.
/// 
/// Arguments:
///   name: struct which is created
///   index: name of variable used for loop iteration
///   state: seq of type/name pairs for functor state, of the form:
///     ((type1,name1)) ((type2,name2)) ...
///
/// Example:
///   LOOP_FUNCTOR(set_all, i, ((GlobalAddress<int64_t>,array)) ((int64_t,value)) ) {
///     SoftXMT_delegate_write_word(array+i, value);
///   }
#define LOOP_FUNCTOR(name, index_var, members) \
struct name : ForkJoinIteration { \
AUTO_DECLS(members) \
AUTO_CONSTRUCTOR( name, members ) \
name() {} /* default constructor */\
inline void operator()(int64_t) const; \
}; \
inline void name::operator()(int64_t index_var) const

#define LOOP_FUNCTOR_TEMPLATED(T, name, index_var, members) \
template< typename T > \
struct name : ForkJoinIteration { \
AUTO_DECLS(members) \
AUTO_CONSTRUCTOR( name, members ) \
name() {} /* default constructor */\
inline void operator()(int64_t) const; \
}; \
template< typename T > \
inline void name::operator()(int64_t index_var) const

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
    request_bytes = (args.addr.block_max() - args.addr) * sizeof(T);
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

#endif /* define __FORK_JOIN_HPP__ */
