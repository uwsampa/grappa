////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#pragma once

#include "Addressing.hpp"
#include "Communicator.hpp"
#include "Collective.hpp"
#include "GlobalAllocator.hpp"
#include "Cache.hpp"
#include "FlatCombiner.hpp"
#include "ParallelLoop.hpp"
#include "Delegate.hpp"
#include <queue>

GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, global_vector_push_ops);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, global_vector_push_msgs);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, global_vector_pop_ops);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, global_vector_pop_msgs);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, global_vector_deq_ops);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, global_vector_deq_msgs);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, global_vector_matched_pops);
GRAPPA_DECLARE_METRIC(SimpleMetric<uint64_t>, global_vector_matched_pushes);
GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, global_vector_push_latency);
GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, global_vector_deq_latency);
GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, global_vector_pop_latency);

// tracks number of operations combined at a time on the master
GRAPPA_DECLARE_METRIC(SummarizingMetric<double>, global_vector_master_combined);

namespace Grappa {
/// @addtogroup Containers
/// @{

const Core MASTER = 0;

class SuspendedDelegateQueue {
protected:
  SuspendedDelegate * head;
  SuspendedDelegate * tail;
public:
  bool blocked;
  
  SuspendedDelegateQueue() { clear(); }
  
  void clear() { head = tail = nullptr; blocked = false; }
  
  void push(SuspendedDelegate * c) {
    c->next = nullptr;
    if (head == nullptr) {
      head = c;
      tail = c;
    } else {
      tail->next = reinterpret_cast<Worker*>(c);
      tail = c;
    }
  }
  SuspendedDelegate * pop() {
    SuspendedDelegate * c = head;
    head = reinterpret_cast<SuspendedDelegate*>(c->next);
    c->next = nullptr;
    return c;
  }
  
  bool empty() const { return head == nullptr; }
  
};

template< typename T, int BUFFER_CAPACITY = (1<<10) >
class GlobalVector {
public:
  struct Master {
    
    size_t head, head_allocator;
    size_t tail, tail_allocator;
    size_t size;
    
    bool combining;
    CompletionEvent ce;
    SuspendedDelegateQueue push_q, pop_q, deq_q;
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
      if (FLAGS_flat_combining_local_only) {
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
          m->ce.wait(SuspendedDelegate::create([self,m,c] {
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
      long ncombined = 0;
      
      while ( !(m->push_q.blocked || m->push_q.empty()) ) {
        ++ncombined;
        m->ce.enroll();
        invoke(m->push_q.pop());
      }
      while ( !(m->deq_q.blocked || m->deq_q.empty()) ) {
        ++ncombined;
        m->ce.enroll();
        invoke(m->deq_q.pop());
      }
      m->ce.wait(SuspendedDelegate::create([self,m,ncombined] {
        DVLOG(2) << "after pushes: tail(" << m->tail << "), tail_allocator(" << m->tail_allocator << ")";
        m->tail = m->tail_allocator;
        m->head = m->head_allocator;
        long ncombined2 = ncombined;
        while (!m->pop_q.empty()) {
          ++ncombined2;
          m->ce.enroll();
          invoke(m->pop_q.pop());
        }
        global_vector_master_combined += ncombined2;
        m->ce.wait(SuspendedDelegate::create([self,m] {
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
        q->push(SuspendedDelegate::create([result_addr,func]{
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
      self->template cache_with_wraparound<typename Incoherent<T>::WO>(push_at, npush, buffer);
      
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
      self->template cache_with_wraparound<typename Incoherent<T>::RO>(pop_at, npop, buffer);
      
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
      self->template cache_with_wraparound<typename Incoherent<T>::RO>(deq_at, ndeq, buffer);
      
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
    auto self = symmetric_global_alloc<GlobalVector>();
    VLOG(3) << "create:\n  self = " << self << "\n  base = " << base;
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
    double t = Grappa::walltime();
    if (FLAGS_flat_combining) {
      proxy.combine([&e](Proxy& p) {
        if (p.npop > 0) {
          ++global_vector_matched_pushes;
          *p.pops[--p.npop] = e;
          return FCStatus::MATCHED;
        } else {
          p.buffer[p.npush++] = e;
          return FCStatus::BLOCKED;
        }
      });
    } else {
      T val = e;
      ++global_vector_push_msgs;
      Master::push(self, &val, 1);
    }
    global_vector_push_latency += (Grappa::walltime() - t);
  }
  
  T pop() {
    ++global_vector_pop_ops;
    double t = Grappa::walltime();
    T val;
    if (FLAGS_flat_combining) {
      proxy.combine([&val](Proxy& p){
        if (p.npush > 0) {
          ++global_vector_matched_pops;
          val = p.buffer[--p.npush];
          return FCStatus::MATCHED;
        } else {
          p.pops[p.npop++] = &val;
          return FCStatus::BLOCKED;
        }
      });
    } else {
      ++global_vector_pop_msgs;
      Master::pop(self, &val, 1);
    }
    global_vector_pop_latency += (Grappa::walltime() - t);
    return val;
  }
  
  inline void enqueue(const T& e) { push(e); }
  
  T dequeue() {
    ++global_vector_deq_ops;
    double t = Grappa::walltime();
    
    T val;
    if (FLAGS_flat_combining) {
      proxy.combine([&val](Proxy& p){
        p.deqs[p.ndeq++] = &val;
        return FCStatus::BLOCKED;
      });
    } else {
      ++global_vector_deq_msgs;
      Master::dequeue(self, &val, 1);
    }
    global_vector_deq_latency += (Grappa::walltime() - t);
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

  struct Range {size_t start, end, size; };
  Range getMasterRange() {
    auto self = this->self;
    return delegate::call(MASTER, [self]{
      auto& m = self->master;
      return Range{m.head, m.tail, m.size};
    });
  }

  template< GlobalCompletionEvent * C = &impl::local_gce,
            int64_t Threshold = impl::USE_LOOP_THRESHOLD_FLAG,
            typename F = nullptr_t >
  friend void forall(GlobalAddress<GlobalVector> self, F func) {
    auto a = self->getMasterRange();
    if (a.size == self->capacity) {
      Grappa::forall<SyncMode::Async,C,Threshold>(self->base, self->capacity, func);
    } else if (a.start < a.end) {
      Range r = {a.start, a.end};
      Grappa::forall<SyncMode::Async,C,Threshold>(self->base+r.start, r.end-r.start, func);
    } else if (a.start > a.end) {
      for (auto r : {Range{0, a.end}, Range{a.start, self->capacity}}) {
        Grappa::forall<SyncMode::Async,C,Threshold>(self->base+r.start, r.end-r.start, func);
      }
    }
    C->wait();
  }

// make sure it's aligned to linear address block_size so we can mirror-allocate it
} GRAPPA_BLOCK_ALIGNED;

/// @}
} // namespace Grappa
