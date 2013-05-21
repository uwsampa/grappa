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
GRAPPA_DECLARE_STAT(SummarizingStatistic<double>, global_vector_push_latency);
GRAPPA_DECLARE_STAT(SummarizingStatistic<double>, global_vector_deq_latency);

namespace Grappa {
/// @addtogroup Containers
/// @{

const Core MASTER = 0;

template< typename T, int BUFFER_CAPACITY = (1<<10) >
class GlobalVector {
public:
  struct Master {
    size_t head;
    size_t tail;
    Master(): head(0), tail(0) {}
  };
  
  struct Proxy {
    GlobalVector * outer;
    T buffer[BUFFER_CAPACITY];
    
    size_t npush;
    
    // std::queue<T*> deqs;
    T* deqs[BUFFER_CAPACITY];
    size_t ndeq;
    
    
    Proxy(GlobalVector* const outer): outer(outer), npush(0), ndeq(0) {}
    
    Proxy* clone_fresh() { return locale_new<Proxy>(outer); }
    
    // bool is_full() { return npush == BUFFER_CAPACITY || deqs.size() == BUFFER_CAPACITY; }
    bool is_full() { return npush == BUFFER_CAPACITY || ndeq == BUFFER_CAPACITY; }
    
    void sync() {
      struct SyncResult { GlobalAddress<T> push_at, deq_at; };
      global_vector_push_msgs++;
      
      auto self = outer->self;
      auto npush = this->npush;
      // auto ndeq = this->deqs.size();
      auto ndeq = this->ndeq;
      
      DVLOG(2) << "self = " << self;
      auto r = delegate::call(MASTER, [self,npush,ndeq]{
        auto& m = self->master;
        DVLOG(2) << "master(head=" << m.head << ", tail=" << m.tail << ")";
        
        auto push_at = m.tail;
        m.tail += npush;
        if (m.tail >= self->capacity) m.tail %= self->capacity;
        
        auto deq_at = m.head;
        m.head += ndeq;
        if (m.head >= self->capacity) m.head %= self->capacity;
        
        // CHECK_NE(m.head, m.tail);
        return SyncResult{self->base+push_at,self->base+deq_at};
      });
      DVLOG(2) << "push{\n  push_at:" << r.push_at << ", deq_at:" << r.deq_at << ", npush:" << npush << "\n  base = " << outer->base << "\n}";
      if (npush) { typename Incoherent<T>::WO c(r.push_at, npush, buffer); }
      if (ndeq) {
        { typename Incoherent<T>::RO c(r.deq_at, ndeq, buffer); c.block_until_acquired(); }
        for (size_t i = 0; i < ndeq; i++) {
          DVLOG(3) << "buffer[" << i << "] = " << buffer[i];
          CHECK_EQ(buffer[i], 42);
          // *deqs.front() = buffer[i];
          // deqs.pop();
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
    DVLOG(3) << "create:\npush  self = " << self << "\npush  base = " << base;
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
    double t = Grappa_walltime();
    if (FLAGS_flat_combining) {
      this->proxy.combine([&e](Proxy& p) {
        global_vector_push_ops++;
        p.buffer[p.npush++] = e;
      });
    } else {
      global_vector_push_ops++; global_vector_push_msgs++;
      auto self = this->self;
      auto offset = delegate::call(MASTER, [self]{ return self->master.head++; });
      delegate::write(this->base+offset, e);
    }
    global_vector_push_latency += (Grappa_walltime() - t);
  }
  
  inline void enqueue(const T& e) { push(e); }
  
  T dequeue() {
    double t = Grappa_walltime();
    
    T result;
    proxy.combine([&result](Proxy& p){
      p.deqs[p.ndeq++] = &result;
    });
    
    global_vector_deq_latency += (Grappa_walltime() - t);
    return result;
  }
  
  /// Return number of elements currently in vector
  size_t size() const { auto self = this->self;
    return delegate::call(MASTER, [self]{
      auto& m = self->master;
      if (m.tail >= m.head) {
        return m.tail - m.head;
      } else {
        return self->capacity + m.tail-m.head;
      }
    });
  }
  
  bool empty() const { return size() == 0; }
  
  /// Return a Linear GlobalAddress to the first element of the vector.
  GlobalAddress<T> begin() const { return this->base + master.head; }
  
  /// Return a Linear GlobalAddress to the end of the vector, that is, one past the last element.
  GlobalAddress<T> end() const { return this->base + master.tail; }
  
  void clear() { auto self = this->self; delegate::call(MASTER, [self]{ self->master = Master(); }); }
  
  GlobalAddress<T> storage() const { return this->base; }

  template< GlobalCompletionEvent * GCE, int64_t Threshold, typename TT, typename F >
  friend void forall_localized(GlobalAddress<GlobalVector<TT>> self, F func);  
};

template< GlobalCompletionEvent * GCE = &impl::local_gce,
          int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
          typename T = decltype(nullptr),
          typename F = decltype(nullptr) >
void forall_localized(GlobalAddress<GlobalVector<T>> self, F func) {
  struct Range {size_t start, end; };
  auto a = delegate::call(MASTER, [self]{ return Range{self->master.head, self->master.tail}; });
  if (a.start < a.end) {
    Range r = {a.start, a.end};
    forall_localized_async<GCE,Threshold>(self->base+r.start, r.end-r.start, func);
  } else {
    for (auto r : {Range{0, a.end}, Range{a.start, self->capacity}}) {
      forall_localized_async<GCE,Threshold>(self->base+r.start, r.end-r.start, func);
    }
  }
  GCE->wait();
}

/// @}
} // namespace Grappa
