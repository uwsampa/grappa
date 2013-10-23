#include "SharedMessagePool.hpp"
#include "Statistics.hpp"
#include "ConditionVariable.hpp"
#include <stack>

DEFINE_int64(shared_pool_size, 1L << 20, "Size (in bytes) of global SharedMessagePool (on each Core)");

/// Note: max is enforced only for blockable callers, and is enforced early to avoid callers
/// that cannot block (callers from message handlers) to have a greater chance of proceeding.
DEFINE_int64(shared_pool_max, -1, "Maximum number of shared pools allowed to be allocated (per core).");

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
    volatile T * top;
  
    PiggybackStack(long max_alloc = -1, long emergency_alloc = 0): allocated(0), max_alloc(max_alloc), emergency_alloc(emergency_alloc), top(nullptr) {}
    
    ~PiggybackStack() {
      CHECK(false) << "didn't think this was being cleaned up.";
      while (!empty()) {
        delete pop();
      }
    }
    
    void release(T * s) {
      CHECK(s->allocated == 0);
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

  volatile SharedMessagePool * shared_pool = nullptr;
  
  volatile PiggybackStack<SharedMessagePool,locale_alloc<void>> *pool_stack;
  
  ConditionVariable blocked_senders;

  void* _shared_pool_alloc_message(size_t sz) {
    DCHECK_GE(shared_pool->remaining(), sz);
    DCHECK( !shared_pool->emptying );
    shared_pool->to_send++;
    return shared_pool->PoolAllocator::allocate(sz);
  }
  
  void init_shared_pool() {
    pool_stack = new PiggybackStack<SharedMessagePool,locale_alloc<void>>(FLAGS_shared_pool_max, FLAGS_shared_pool_max/4);
    shared_pool = pool_stack->take();
  }
  
  void* _shared_message_pool_alloc(size_t sz) {
#ifdef DEBUG
    auto i = pool_stack->find(shared_pool);
    if (i >= 0) VLOG(0) << "found: " << shared_pool << ": " << i << " / " << pool_stack->size();
#endif
    // CHECK(shared_pool->next == nullptr);
    if (shared_pool && !shared_pool->emptying && shared_pool->remaining() >= sz) {
      return _shared_pool_alloc_message(sz);
    } else {
      // if not emptying already, do it
      auto *o = shared_pool;
      if (shared_pool && !shared_pool->emptying) {
        CHECK_GT(shared_pool->allocated, 0);
        shared_pool->emptying = true;
        if (shared_pool->to_send == 0) {
          shared_pool->on_empty();
        }
      }
      if (!pool_stack->empty()) CHECK(pool_stack->top->allocated == 0);
      // find new shared_pool
      SharedMessagePool *p = pool_stack->take();
      if (p) {
        shared_pool = p;
        return _shared_pool_alloc_message(sz);
      } else {
        // can't allocate any more
        // try to block until a pool frees up
        do {
          Grappa::wait(&blocked_senders);
          if (!shared_pool->emptying && shared_pool->remaining() >= sz) {
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
  
  void* SharedMessagePool::allocate(size_t sz) {
    CHECK_EQ(this, shared_pool) << "not allocating from current shared_pool!";
    // CHECK(this->next == nullptr);
    return _shared_message_pool_alloc(sz);
  }
  
  void SharedMessagePool::message_sent(impl::MessageBase* m) {
    CHECK(next == nullptr);
    validate_in_pool(m);
    to_send--;
    if (emptying && to_send == 0) {
      // CHECK(this != shared_pool);
      CHECK_GT(shared_pool->allocated, 0);
      on_empty();
    }
  }
  
  void SharedMessagePool::on_empty() {
    DCHECK_EQ(to_send, 0);
    VLOG(3) << "empty and emptying, to_send:"<< to_send << ", allocated:" << allocated << "/" << buffer_size << " @ " << this << ", buf:" << (void*)buffer << "  completions_received:" << completions_received << ", allocated_count:" << allocated_count;
// #ifdef DEBUG
    // verify everything sent
    this->iterate([](impl::MessageBase* m) {
      if (!m->is_sent_ || !m->is_delivered_ || !m->is_enqueued_) {
        CHECK(false) << "!! message(" << m << ", is_sent_:" << m->is_sent_ << ", is_delivered_:" << m->is_delivered_ << ", m->is_enqueued_:" << m->is_enqueued_ << ", extra:" << m->pool << ")";
      }
    });
  
    memset(buffer, (0x11*locale_mycore()) | 0xf0, buffer_size); // poison
    
// #endif
    CHECK( pool_stack->find(this) == -1 );
    CHECK( this->next == nullptr );
    
    this->reset();
    pool_stack->release(this);
    broadcast(&blocked_senders);
  }

} // namespace Grappa
