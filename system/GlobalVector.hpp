#include "Addressing.hpp"
#include "Communicator.hpp"
#include "Collective.hpp"
#include "GlobalAllocator.hpp"
#include "Cache.hpp"
#include "FlatCombiner.hpp"
#include "ParallelLoop.hpp"
#include <queue>

GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, global_vector_push_ops);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, global_vector_push_msgs);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, global_vector_pop_ops);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, global_vector_pop_msgs);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, global_vector_deq_ops);
GRAPPA_DECLARE_STAT(SimpleStatistic<uint64_t>, global_vector_deq_msgs);
GRAPPA_DECLARE_STAT(SummarizingStatistic<double>, global_vector_push_latency);
GRAPPA_DECLARE_STAT(SummarizingStatistic<double>, global_vector_deq_latency);
GRAPPA_DECLARE_STAT(SummarizingStatistic<double>, global_vector_pop_latency);

namespace Grappa {
/// @addtogroup Containers
/// @{

const Core MASTER = 0;


template< typename T, int BUFFER_CAPACITY = (1<<10) >
class GlobalVector {
public:
  struct Master {
    
    size_t head;
    Mutex head_lock;
    
    size_t tail;
    Mutex tail_lock;
    
    size_t size;
    
    void clear() {
      head = 0; tail = 0; size = 0;
    }
    
    Master() { clear(); }
    ~Master() {}
    
    static void pushpop(GlobalAddress<GlobalVector> self, T * buffer, int64_t delta) {
      auto tail_lock = [self]{ return &self->master.tail_lock; };
      
      auto tail = delegate::call(MASTER, tail_lock, [self,delta](Mutex * l){
        lock(l);
        
        if (delta < 0) self->incr_with_wrap(&self->master.tail, delta);  // pop
        CHECK_GE(self->master.tail, 0);
        return self->master.tail;
      });
      
      if (delta > 0) { // push
        self->cache_with_wraparound<typename Incoherent<T>::WO>(tail, delta, buffer);
      } else { // pop
        self->cache_with_wraparound<typename Incoherent<T>::RO>(tail, 0-delta, buffer);
      }
      
      delegate::call(MASTER, [self,delta]() {
        if (delta > 0) self->incr_with_wrap(&self->master.tail, delta);  // push
        self->master.size += delta;
        
        unlock(&self->master.tail_lock);
      });
      CHECK_LE(self->master.size, self->capacity);
    }
    
    static void dequeue(GlobalAddress<GlobalVector> self, T * buffer, int64_t delta) {
      auto head_lock = [self]{ return &self->master.head_lock; };
      
      auto head = delegate::call(MASTER, head_lock, [self,delta](Mutex * l){
        lock(l);
        // if something was popped, tail will have been decremented, but may still be locked
        CHECK_LE(self->master.head+delta, self->master.tail);
        return self->master.head;
      });

      self->cache_with_wraparound<typename Incoherent<T>::RO>(head, delta, buffer);
      
      delegate::call(MASTER, [self,delta] {
        if (delta > 0) self->incr_with_wrap(&self->master.head, delta);
        self->master.size -= delta;
        
        CHECK_LE(self->master.size, self->capacity);
        unlock(&self->master.head_lock);
      });
    }
    
  };

  inline void incr_with_wrap(size_t * i, long incr) {
    *i += incr;
    if (*i >= capacity) {
      *i %= capacity;
    } else if (i < 0){
      *i += capacity;
    }
  }
  
  template< typename Cache >
  void cache_with_wraparound(size_t start, size_t nelem, T * buffer) {
    struct Range {size_t start, end; };
    if (start+nelem <= capacity) {
      Cache c(base+start, nelem, buffer); c.block_until_acquired();
    } else {
      auto end = (start+nelem)%capacity;
      Cache c1(base, end, buffer);
      Cache c2(base+start, capacity-start, buffer+end);
      c1.start_acquire();
      c2.block_until_acquired();
      c1.block_until_acquired();
    }
  }
  
  struct Proxy {
    GlobalVector * outer;
    T buffer[BUFFER_CAPACITY];
    
    long npush;
    
    T* deqs[BUFFER_CAPACITY];
    long ndeq;
    
    T* pops[BUFFER_CAPACITY];
    long npop;
    
    Proxy(GlobalVector* const outer): outer(outer), npush(0), ndeq(0), npop(0) {}
    
    Proxy* clone_fresh() { return locale_new<Proxy>(outer); }
    
    bool is_full() { return npush == BUFFER_CAPACITY || ndeq == BUFFER_CAPACITY; }
    
    void sync() {
      if (npush > 0) {
        ++global_vector_push_msgs;
        Master::pushpop(outer->self, buffer, npush);
      } else if (npop > 0) {
        ++global_vector_pop_msgs;
        Master::pushpop(outer->self, buffer, 0-npop);
        for (auto i = 0; i < npop; i++) {
          *pops[i] = buffer[i];
        }
      }
      if (ndeq > 0) {
        ++global_vector_deq_msgs;
        Master::dequeue(outer->self, buffer, ndeq);
        for (auto i = 0; i < ndeq; i++) {
          *deqs[i] = buffer[i];
        }
      }
    }
  };

public:  
  GlobalAddress<T> base;
  size_t capacity;
protected:
  GlobalAddress<GlobalVector> self;
  
  Master master;
  FlatCombiner<Proxy> proxy;
  
  char _pad[block_size-sizeof(base)-sizeof(capacity)-sizeof(self)-sizeof(master)-sizeof(proxy)];
  
public:
  GlobalVector(): proxy(locale_new<Proxy>(this)) {}
  
  GlobalVector(GlobalAddress<GlobalVector> self, GlobalAddress<T> storage_base, size_t total_capacity)
    : proxy(locale_new<Proxy>(this))
  {
    this->self = self;
    base = storage_base;
    capacity = total_capacity;
  }
  ~GlobalVector() {}
  
  static GlobalAddress<GlobalVector> create(size_t total_capacity) {
    auto base = global_alloc<T>(total_capacity);
    auto self = mirrored_global_alloc<GlobalVector>();
    DVLOG(2) << "create:\n  self = " << self << "\n  base = " << base;
    call_on_all_cores([self,base,total_capacity]{
      new (self.localize()) GlobalVector(self, base, total_capacity);
    });
    return self;
  }
  
  void destroy() {
    auto self = this->self;
    global_free(this->base);
    call_on_all_cores([self]{ self->~GlobalVector(); });
    global_free(self);
  }  
  
  /// Push element on the back (queue or stack)
  void push(const T& e) {
    ++global_vector_push_ops;
    double t = Grappa_walltime();
    if (FLAGS_flat_combining) {
      proxy.combine([&e](Proxy& p) {
        if (p.npop > 0) {
          *p.pops[--p.npop] = e;
        } else {
          p.buffer[p.npush++] = e;
        }
      });
    } else {
      T val = e;
      Master::pushpop(self, &val, 1);
    }
    global_vector_push_latency += (Grappa_walltime() - t);
  }
  
  T pop() {
    ++global_vector_pop_ops;
    double t = Grappa_walltime();
    T val;
    if (FLAGS_flat_combining) {
      proxy.combine([&val](Proxy& p){
        if (p.npush > 0) {
          val = p.buffer[--p.npush];
        } else {
          p.pops[p.npop++] = &val;
        }
      });
    } else {
      Master::pushpop(self, &val, -1);
    }
    global_vector_pop_latency += (Grappa_walltime() - t);
    return val;
  }
  
  inline void enqueue(const T& e) { push(e); }
  
  T dequeue() {
    ++global_vector_deq_ops;
    double t = Grappa_walltime();
    
    T val;
    if (FLAGS_flat_combining) {
      proxy.combine([&val](Proxy& p){
        p.deqs[p.ndeq++] = &val;
      });
    } else {
      Master::dequeue(self, &val, 1);
    }
    global_vector_deq_latency += (Grappa_walltime() - t);
    return val;
  }
  
  /// Return number of elements currently in vector
  size_t size() const { auto self = this->self;
    return delegate::call(MASTER, [self]{ return self->master.size; });
  }
  
  bool empty() const { return size() == 0; }
  
  /// Return a Linear GlobalAddress to the first element of the vector.
  GlobalAddress<T> begin() const { return this->base + master.head; }
  
  /// Return a Linear GlobalAddress to the end of the vector, that is, one past the last element.
  GlobalAddress<T> end() const { return this->base + master.tail; }
  
  void clear() {
    auto self = this->self;
    delegate::call(MASTER, [self]{ self->master.clear(); });
  }
  
  GlobalAddress<T> storage() const { return this->base; }

  template< GlobalCompletionEvent * GCE, int64_t Threshold, typename TT, typename F >
  friend void forall_localized(GlobalAddress<GlobalVector<TT>> self, F func);  
};

template< GlobalCompletionEvent * GCE = &impl::local_gce,
          int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
          typename T = decltype(nullptr),
          typename F = decltype(nullptr) >
void forall_localized(GlobalAddress<GlobalVector<T>> self, F func) {
  struct Range {size_t start, end, size; };
  auto a = delegate::call(MASTER, [self]{ auto& m = self->master; return Range{m.head, m.tail, m.size}; });
  if (a.size == self->capacity) {
    forall_localized_async<GCE,Threshold>(self->base, self->capacity, func);
  } else if (a.start < a.end) {
    Range r = {a.start, a.end};
    forall_localized_async<GCE,Threshold>(self->base+r.start, r.end-r.start, func);
  } else if (a.start > a.end) {
    for (auto r : {Range{0, a.end}, Range{a.start, self->capacity}}) {
      forall_localized_async<GCE,Threshold>(self->base+r.start, r.end-r.start, func);
    }
  }
  GCE->wait();
}

/// @}
} // namespace Grappa
