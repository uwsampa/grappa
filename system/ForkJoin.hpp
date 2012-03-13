#include "SoftXMT.hpp"
#include "Addressing.hpp"
#include <iostream>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_base_of.hpp>

#define SLOG(verboselevel) VLOG(verboselevel) << "<" << SoftXMT_mynode() << "> "

#define min(A,B) ( (A) < (B) ? (A) : (B))

DEFINE_int64(max_forkjoin_threads_per_node, 256, "maximum number of threads to spawn for a fork-join region");

struct range_t { int64_t start, end; };

static range_t blockDist(int64_t start, int64_t end, int64_t rank, int64_t numBlocks) {
	int64_t numElems = end-start;
	int64_t each   = numElems / numBlocks,
  remain = numElems % numBlocks;
	int64_t mynum = (rank < remain) ? each+1 : each;
	int64_t mystart = (rank < remain) ? (each+1)*rank : (each+1)*remain + (rank-remain)*each;
	range_t r = { mystart, mystart+mynum };
  return r;
}

class Semaphore {
protected:
  thread * sleeper;
  int count;
  int total;
public:
  Semaphore(int total, int starting): total(total), count(starting) {}
  void acquire_all(thread * me) {
    sleeper = me;
    while (count < total) {
      SLOG(2) << "Semaphore.count = " << count << " of " << total << ", suspending...";
      SoftXMT_suspend();
    }
  }
  void release(int n=1) {
    count += n;
    if (count >= total) {
      SoftXMT_wake(sleeper);
    }
  }
  static void am_release(GlobalAddress<Semaphore>* gaddr, size_t sz, void* payload, size_t psz) {
    SLOG(2) << "in am_release()";
    assert(gaddr->node() == SoftXMT_mynode());
    int n = (int)reinterpret_cast<int64_t>(payload);
    gaddr->pointer()->release(n);
  }
  static void release(GlobalAddress<Semaphore>* gaddr, int n) {
    int64_t nn = n;
    SLOG(2) << "about to call on " << gaddr->node();
    SoftXMT_call_on(gaddr->node(), &Semaphore::am_release, gaddr, sizeof(GlobalAddress<Semaphore>), reinterpret_cast<void*>(nn), 0);
  }
};

struct ForkJoinIteration {
  void operator()(thread * me, int64_t index);
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
  thread * node_th;
  size_t local_start;
  size_t local_end;
  
  forkjoin_data_t(thread * me, T* f, int64_t start, int64_t end) {
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
static void th_iters(thread * me, iters_args* arg) {
  forkjoin_data_t<T> * fj = static_cast<forkjoin_data_t<T>*>(arg->fjdata);
  range_t myblock = blockDist(fj->local_start, fj->local_end, arg->rank, fj->nthreads);
  
  for (int64_t i=myblock.start; i < myblock.end; i++) {
    (*fj->func)(me, i);
  }
  fj->finished++;
  if (fj->finished == fj->nthreads) {
    SoftXMT_wake(fj->node_th);
  }
}

template<typename T>
static void fork_join_onenode(thread * spawner, T* func, int64_t start, int64_t end) {
  forkjoin_data_t<T> fj(spawner, func, start, end);
  iters_args args[fj.nthreads];
  
  for (int i=0; i<fj.nthreads; i++) {
    args[i].fjdata = &fj;
    args[i].rank = i;
    
    SoftXMT_template_spawn(&th_iters<T>, &args[i]);
  }
  while (fj.finished < fj.nthreads) SoftXMT_suspend();
}

template<typename T>
static void th_node_fork_join(thread * me, NodeForkJoinArgs<T>* a) {
  range_t myblock = blockDist(a->start, a->end, SoftXMT_mynode(), SoftXMT_nodes());
  fork_join_onenode(me, &a->func, myblock.start, myblock.end);
  
  SLOG(2) << "about to update sem on " << a->sem.node();
  Semaphore::release(&a->sem, 1);
}

template<typename T>
static void fork_join(thread * me, T* func, int64_t start, int64_t end) {
  Semaphore sem(SoftXMT_nodes(), 0);
  
  NodeForkJoinArgs<T> fj;
  fj.start = start;
  fj.end = end;
  fj.func = *func;
  fj.sem = make_global(&sem);
  
  for (int i=0; i < SoftXMT_nodes(); i++) {
    SoftXMT_remote_spawn(&th_node_fork_join, &fj, i);
    SoftXMT_flush(i); // TODO: remove this?
  }
  SLOG(2) << "waiting to acquire all";
  sem.acquire_all(me);
  
  SLOG(2) << "fork_join done";
}

