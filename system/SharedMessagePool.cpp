#include "SharedMessagePool.hpp"
#include "Statistics.hpp"
#include "ConditionVariable.hpp"
#include <stack>

DEFINE_int64(shared_pool_size, 1L << 18, "Size (in bytes) of global SharedMessagePool (on each Core)");

/// Note: max is enforced only for blockable callers, and is enforced early to avoid callers
/// that cannot block (callers from message handlers) to have a greater chance of proceeding.
/// In 'init_shared_pool()', 'emergency_alloc' margin is set, currently, to 1/4 of the max.
///
/// Currently setting this max to cap shared_pool usage at ~1GB, though this may be too much
/// for some apps & machines, as it is allocated out of the locale-shared heap. It's also
/// worth noting that under current settings, most apps don't reach this point at steady state.
DEFINE_int64(shared_pool_max, -1, "Maximum number of shared pools allowed to be allocated (per core).");
DEFINE_double(shared_pool_memory_fraction, 0.5, "Fraction of remaining memory to use for shared pool (after taking out global_heap and stacks)");

GRAPPA_DEFINE_STAT(SimpleStatistic<uint64_t>, shared_message_pools_allocated, 0);

namespace Grappa {

  /// type T must have a member: 'T * next'
  template< typename T, void *(*Allocator)(size_t) = malloc >
  class PiggybackStack {
  protected:
    T* pop() {
      T* r = top;
      if (r != nullptr) {
        top = r->next;
        if (top) CHECK(top->allocated == 0);
        r->next = nullptr;
      }
      return r;
    }
    
    bool is_emergency() { return global_scheduler.in_no_switch_region(); }
  public:
    long allocated;
    const long max_alloc;
    const long emergency_alloc;
    T * top;
  
    PiggybackStack(long max_alloc = -1, long emergency_alloc = 0): allocated(0), max_alloc(max_alloc), emergency_alloc(emergency_alloc), top(nullptr) {}
    
    ~PiggybackStack() {
      CHECK(false) << "didn't think this was being cleaned up.";
      while (!empty()) {
        delete pop();
      }
    }
    
    void release(T * s) {
      DCHECK(s->allocated == 0);
      s->next = top;
      top = s;
    }
    
    T* take() {
      T* r = nullptr;
      if (!empty()) {
        r = pop();
      } else if ((max_alloc == -1) || (allocated < (max_alloc-emergency_alloc))
                || (is_emergency() && allocated < max_alloc)) {
        allocated++;
        shared_message_pools_allocated++;
        r = new (Allocator(sizeof(T))) T();
      }
      // DCHECK(r != nullptr);
      // new (r) T();
      return r;
    }
    
    bool empty() { return top == nullptr; }
    
    long find(T *o) {
      long id = 0;
      for (auto *i = top; i; i = i->next, id++) {
        if (i == o) return id;
      }
      return -1;
    }
    
    long size() {
      long id = 0;
      for (auto *i = top; i; i = i->next, id++);
      return id;
    }
    
  };

  SharedMessagePool * shared_pool = nullptr;
  
  PiggybackStack<SharedMessagePool,locale_alloc> *pool_stack;
  
  ConditionVariable blocked_senders;

  void* _shared_pool_alloc_message(size_t sz) {
    DCHECK_GE(shared_pool->remaining(), sz);
    DCHECK( !shared_pool->emptying );
    shared_pool->to_send++;
    return shared_pool->PoolAllocator::allocate(sz);
  }
  
  void init_shared_pool() {
    if (FLAGS_shared_pool_max == -1) {
      size_t mem_remaining = impl::locale_shared_memory.get_free_memory() / locale_cores();
      long num_pools = static_cast<long>(FLAGS_shared_pool_memory_fraction * static_cast<double>(mem_remaining)) / FLAGS_shared_pool_size;
      FLAGS_shared_pool_max = num_pools;
    }
    double total_gb = static_cast<double>(FLAGS_shared_pool_max*FLAGS_shared_pool_size) / (1L<<30);
    if (mycore() == 0) VLOG(1) << "shared_pool -> size = " << FLAGS_shared_pool_size << ", max = " << FLAGS_shared_pool_max << ", total = " << total_gb << " GB";
    pool_stack = new PiggybackStack<SharedMessagePool,locale_alloc>(FLAGS_shared_pool_max, FLAGS_shared_pool_max/4);
    shared_pool = pool_stack->take();
  }
  
  void* _shared_pool_alloc(size_t sz) {
// #ifdef DEBUG
    // auto i = pool_stack->find(shared_pool);
    // if (i >= 0) VLOG(0) << "found: " << shared_pool << ": " << i << " / " << pool_stack->size();
// #endif
    DCHECK(!shared_pool || shared_pool->next == nullptr);
    if (shared_pool && !shared_pool->emptying && shared_pool->remaining() >= sz) {
      return _shared_pool_alloc_message(sz);
    } else {
      // if not emptying already, do it
      auto *o = shared_pool;
      shared_pool = nullptr;
      if (o && !o->emptying) {
        CHECK_GT(o->allocated, 0);
        o->emptying = true;
        if (o->to_send == 0) {
          o->on_empty();
        }
      }
      DCHECK(pool_stack->empty() || pool_stack->top->allocated == 0);
      // find new shared_pool
      SharedMessagePool *p = pool_stack->take();
      if (p) {
        CHECK_EQ(p->allocated, 0) << "not a fresh pool!";
        shared_pool = p;
        return _shared_pool_alloc_message(sz);
      } else {
        // can't allocate any more
        // try to block until a pool frees up
        do {
          Grappa::wait(&blocked_senders);
          if (shared_pool && !shared_pool->emptying && shared_pool->remaining() >= sz) {
            p = shared_pool;
          } else {
            p = pool_stack->take();
          }
        } while (!p);
        
        // p must be able to allocate
        shared_pool = p;
        return _shared_pool_alloc_message(sz);
      }
    }
  }
  
  void SharedMessagePool::message_sent(impl::MessageBase* m) {
    DCHECK(this->next == nullptr);
#ifdef DEBUG
    validate_in_pool(m);
#endif
    this->to_send--;
    if (this->emptying && this->to_send == 0) {
      // CHECK(this != shared_pool);
      DCHECK_GT(this->allocated, 0);
      this->on_empty();
    }
  }
  
  void SharedMessagePool::on_empty() {
    CHECK(this->next == nullptr);
    CHECK(this != shared_pool);
    
    DCHECK_EQ(this->to_send, 0);
    DVLOG(3) << "empty and emptying, to_send:"<< to_send << ", allocated:" << allocated << "/" << buffer_size << " @ " << this << ", buf:" << (void*)buffer << "  completions_received:" << completions_received << ", allocated_count:" << allocated_count;
// #ifdef DEBUG
    // verify everything sent
    // this->iterate([](impl::MessageBase* m) {
    //   if (!m->is_sent_ || !m->is_delivered_ || !m->is_enqueued_) {
    //     CHECK(false) << "!! message(" << m << ", is_sent_:" << m->is_sent_ << ", is_delivered_:" << m->is_delivered_ << ", m->is_enqueued_:" << m->is_enqueued_ << ", extra:" << m->pool << ")";
    //   }
    // });
  
    memset(buffer, (0x11*locale_mycore()) | 0xf0, buffer_size); // poison
    
// #endif
    DCHECK( pool_stack->find(this) == -1 );
    DCHECK( this->next == nullptr );
    
    this->reset();
    pool_stack->release(this);
    broadcast(&blocked_senders);
  }

} // namespace Grappa
