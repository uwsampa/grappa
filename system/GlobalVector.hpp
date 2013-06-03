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

class ContinuationQueue {
protected:
  Continuation * head;
  Continuation * tail;
public:
  bool blocked;
  
  ContinuationQueue() { clear(); }
  
  void clear() { head = tail = nullptr; blocked = false; }
  
  void push(Continuation * c) {
    c->next = nullptr;
    if (head == nullptr) {
      head = c;
      tail = c;
    } else {
      tail->next = reinterpret_cast<Worker*>(c);
      tail = c;
    }
  }
  Continuation * pop() {
    Continuation * c = head;
    head = reinterpret_cast<Continuation*>(c->next);
    c->next = nullptr;
    return c;
  }
  
  bool empty() const { return head == nullptr; }
  
};

template< typename T, int BUFFER_CAPACITY = (1<<12) >
class GlobalVector {
public:
  struct Master {
    
    size_t head, head_allocator;
    size_t tail, tail_allocator;
    size_t size;
    
    bool combining;
    CompletionEvent ce;
    ContinuationQueue push_q, pop_q, deq_q;
    bool has_requests() { return !push_q.empty() || !pop_q.empty() || !deq_q.empty(); }
    
    void clear() {
      head = tail = head_allocator = tail_allocator = size = 0;
      push_q.clear();
      pop_q.clear();
      deq_q.clear();
      combining = false;
    }
    
    Master() { clear(); }
    ~Master() {}
    
    static void master_combine(GlobalAddress<GlobalVector> self) {
      auto* m = &self->master;
      m->combining = true;
      
      // TODO: make me less ugly...
      if (!FLAGS_flat_combining || FLAGS_flat_combining_local_only) {
        if (m->has_requests()) {
          m->ce.enroll();
          enum class Choice { POP, DEQ, PUSH };
          Choice c;
          if (!m->push_q.empty()) {
            c = Choice::PUSH;
            invoke(m->push_q.pop());
          } else if (!m->pop_q.empty()) {
            c = Choice::POP;
            invoke(m->pop_q.pop());
          } else if (!m->deq_q.empty()) {
            c = Choice::DEQ;
            invoke(m->deq_q.pop());
          }
          DVLOG(2) << "combining: tail(" << m->tail << "), tail_allocator(" << m->tail_allocator << ")";
          m->ce.wait(new_continuation([self,m,c] {
            switch (c) {
              case Choice::PUSH: m->tail = m->tail_allocator; break;
              case Choice::POP:  m->tail_allocator = m->tail; break;
              case Choice::DEQ:  m->head = m->head_allocator; break;
            }
            if (m->has_requests()) {
              master_combine(self);
            } else {
              m->combining = false;
            }
            DVLOG(2) << "woken: tail(" << m->tail << "), tail_allocator(" << m->tail_allocator << ")";
          }));
        }
        return;
      } // else: do second level of combining
      
      DVLOG(2) << "combining: tail(" << m->tail << "), tail_allocator(" << m->tail_allocator << ")";
      
      while ( !(m->push_q.blocked || m->push_q.empty()) ) {
        m->ce.enroll();
        invoke(m->push_q.pop());
      }
      while ( !(m->deq_q.blocked || m->deq_q.empty()) ) {
        m->ce.enroll();
        invoke(m->deq_q.pop());
      }
      m->ce.wait(new_continuation([self,m] {
        DVLOG(2) << "after pushes: tail(" << m->tail << "), tail_allocator(" << m->tail_allocator << ")";
        m->tail = m->tail_allocator;
        m->head = m->head_allocator;
        
        while (!m->pop_q.empty()) {
          m->ce.enroll();
          invoke(m->pop_q.pop());
        }
        m->ce.wait(new_continuation([self,m] {
          DVLOG(2) << "after pops: tail(" << m->tail << "), tail_allocator(" << m->tail_allocator << ")";
          m->tail_allocator = m->tail;
          // find new combiner...
          if (m->has_requests()) {
            master_combine(self);
          } else {
            m->combining = false;
          }
        }));
      }));
    }
    
    template< typename Q, typename F >
    static auto request(GlobalAddress<GlobalVector> self, Q yield_q, F func) -> decltype(func()) {
      using R = decltype(func());
      
      FullEmpty<R> result;
      auto result_addr = make_global(&result);
      auto do_call = [self,result_addr,yield_q,func]{
        auto m = &self->master;
        auto q = yield_q(m);
        q->push(new_continuation([result_addr,func]{
          auto val = func();
          auto set_result = [result_addr,val]{ result_addr->writeXF(val); };
          
          if (result_addr.core() == mycore()) set_result();
          else send_heap_message(result_addr.core(), set_result);
        }));
        if (!m->combining) master_combine(self); // try becoming combiner
      };
      if (MASTER == mycore()) do_call(); else send_message(MASTER, do_call);
  
      auto r = result.readFF();
      return r;
    }
    
    static void push(GlobalAddress<GlobalVector> self, T * buffer, int64_t npush) {
      auto yield_q = [](Master * m){ return &m->push_q; };
      auto origin = mycore();
      auto push_at = request(self, yield_q, [self,npush,origin]{
        auto push_at = self->master.tail_allocator;
        self->incr_with_wrap(&self->master.tail_allocator, npush);
        self->master.size += npush;
        DVLOG(2) << "in request(from:" << origin << ", npush:" << npush << ", push_at:" << push_at << ")";
        return push_at;
      });
      DVLOG(2) << "response from request: push_at(" << push_at << ") (npush:" << npush << ")";
      self->cache_with_wraparound<typename Incoherent<T>::WO>(push_at, npush, buffer);
      
      send_message(MASTER, [self]{ self->master.ce.complete(); });
    }

    static void pop(GlobalAddress<GlobalVector> self, T * buffer, int64_t npop) {
      auto origin = mycore();
      auto yield_q = [](Master * m){ return &m->pop_q; };
      size_t pop_at = request(self, yield_q, [self,npop,origin]{
        CHECK_LE(0-(int64_t)npop, 0);
        self->incr_with_wrap(&self->master.tail, 0-(int64_t)npop);
        self->master.size -= npop;
        DVLOG(2) << "in request(from:" << origin << ", npop:" << npop << ", pop_at:" << self->master.tail << ")";
        return self->master.tail;
      });
      DVLOG(2) << "response from request: pop_at(" << pop_at << ") (npop:" << npop << ")";
      self->cache_with_wraparound<typename Incoherent<T>::RO>(pop_at, npop, buffer);
      
      send_message(MASTER, [self]{ self->master.ce.complete(); });
    }
    
    static void dequeue(GlobalAddress<GlobalVector> self, T * buffer, int64_t ndeq) {
      auto origin = mycore();
      auto yield_q = [](Master * m){ return &m->deq_q; };
      auto deq_at = request(self, yield_q, [self,ndeq,origin]{
        self->incr_with_wrap(&self->master.head, ndeq);
        // if (self->master.head > self->master.tail) {
        //   // oops, too far, rollback and wait for pushes to finish
        //   self->incr_with_wrap(&self->master.head, 0-ndeq);
        //   self->master.deq_q.blocked = true;
        //   // add self back onto q...
        // } // TODO: how do I not send a wakekup on return?
        self->master.size -= ndeq;
        return self->master.head;
      });
      DVLOG(2) << "response from request: deq_at(" << deq_at << ") (ndeq:" << ndeq << ")";
      self->cache_with_wraparound<typename Incoherent<T>::RO>(deq_at, ndeq, buffer);
      
      send_message(MASTER, [self]{ self->master.ce.complete(); });
    }
    
  };

  inline void incr_with_wrap(size_t * i, long incr) {
    long val = *i;
    val += incr;
    if (val >= capacity) {
      val %= capacity;
    } else if (val < 0){
      val += capacity;
    }
    DCHECK_GE(val,0);
    *i = val;
  }
  
  template< typename Cache >
  void cache_with_wraparound(size_t start, size_t nelem, T * buffer) {
    CHECK_GE(start, 0); CHECK_LT(start, capacity); CHECK_GE(start+nelem, 0);
    struct Range {size_t start, end; };
    if (start+nelem <= capacity) {
      DVLOG(3) << "put(base[" << start << "]<" << (base+start).core() << ":" << (base+start).pointer() << ">, buffer:" << buffer << ", nelem:" << nelem << ")\n" << base;
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
    
    Proxy(GlobalVector* const outer): outer(outer) { clear(); }
    
    void clear() { npush = 0; ndeq = 0; npop = 0; }
    
    Proxy* clone_fresh() { return locale_new<Proxy>(outer); }
    
    bool is_full() { return npush == BUFFER_CAPACITY || ndeq == BUFFER_CAPACITY || npop == BUFFER_CAPACITY; }
    
    void sync() {
      if (npush > 0) {
        ++global_vector_push_msgs;
        // Master::pushpop(outer->self, buffer, npush);
        Master::push(outer->self, buffer, npush);
      } else if (npop > 0) {
        ++global_vector_pop_msgs;
        // Master::pushpop(outer->self, buffer, 0-npop);
        Master::pop(outer->self, buffer, npop);
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
  
  static const size_t padded_size = 4*block_size;
  char _pad[padded_size-sizeof(base)-sizeof(capacity)-sizeof(self)-sizeof(master)-sizeof(proxy)];
  
public:
  GlobalVector(): proxy(locale_new<Proxy>(this)) {}
  
  GlobalVector(GlobalAddress<GlobalVector> self, GlobalAddress<T> storage_base, size_t total_capacity)
    : proxy(locale_new<Proxy>(this))
  {
    size_t sz = sizeof(base)+sizeof(capacity)+sizeof(self)+sizeof(master)+sizeof(proxy);
    CHECK_LT(sz, padded_size);
    this->self = self;
    base = storage_base;
    capacity = total_capacity;
  }
  ~GlobalVector() {}
  
  static GlobalAddress<GlobalVector> create(size_t total_capacity) {
    auto base = global_alloc<T>(total_capacity);
    auto self = mirrored_global_alloc<GlobalVector>();
    VLOG(0) << "create:\n  self = " << self << "\n  base = " << base;
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
      ++global_vector_push_msgs;
      Master::push(self, &val, 1);
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
      ++global_vector_pop_msgs;
      Master::pop(self, &val, 1);
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
      ++global_vector_deq_msgs;
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
