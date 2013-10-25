#include "SharedMessagePool.hpp"
#include "Statistics.hpp"
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

  SharedMessagePool * shared_pool = nullptr;
  std::stack<SharedMessagePool*> unused_pools;

  void init_shared_pool() {
    void * p = impl::locale_shared_memory.allocate_aligned(sizeof(SharedMessagePool), 8);
    shared_pool = new (p) SharedMessagePool(FLAGS_shared_pool_size);
    shared_message_pools_allocated++;
    VLOG(3) << "initialized shared_pool @ " << shared_pool << ", buf:" << (void*)shared_pool->buffer;
  }

  void* SharedMessagePool::allocate(size_t size) {
    VLOG(5) << "allocating on shared pool " << this;
    CHECK_EQ(this, shared_pool) << "not allocating from global shared_pool!";
  
    if (this->remaining() < size) {
      CHECK(size <= this->buffer_size) << "Pool (" << this->buffer_size << ") is not large enough to allocate " << size << " bytes.";
      VLOG(3) << "MessagePool full (" << this << ", to_send:"<< this->to_send << "), finding a new one;    completions_received:" << completions_received << ", allocated_count:" << allocated_count;
    
      // first try and find one from the unused_pools pool
      if (unused_pools.size() > 0) {
        shared_pool = unused_pools.top();
        VLOG(4) << "found pool @ " << shared_pool << " allocated:" << shared_pool->allocated << "/" << shared_pool->buffer_size << ", count=" << allocated_count << " @ " << this;
        unused_pools.pop();
      } else {
        // allocate a new one, have the old one delete itself when all previous have been sent
        void * p = impl::locale_shared_memory.allocate_aligned(sizeof(SharedMessagePool), 8);
        shared_pool = new (p) SharedMessagePool(this->buffer_size);
        shared_message_pools_allocated++;
        VLOG(4) << "created new shared_pool @ " << shared_pool << ", buf:" << shared_pool->buffer;
      }
    
      this->start_emptying(); // start working on freeing up the previous one
    
      return shared_pool->allocate(size); // actually satisfy the allocation request
    } else {
      allocated_count++;
      this->to_send++;
      return PoolAllocator<impl::MessageBase>::allocate(size);
      // return impl::MessagePoolBase::allocate(size);
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
    CHECK( reinterpret_cast<char*>(m) >= this->buffer && reinterpret_cast<char*>(m)+m->size() <= this->buffer+this->buffer_size )
        << "message is outside this pool!! message(" << m << ", extra:" << m->pool << "), "
        << "pool(" << this << ", buf:" << (void*)this->buffer << ", size:" << this->buffer_size << ")";
    to_send--;
    completions_received++;
    if (to_send == 0 && emptying) {
      VLOG(3) << "empty! so put back on unused stack message(" << m << ", is_sent_:" << m->is_sent_ << ")";
      on_empty();
    }
  }

  void SharedMessagePool::on_empty() {
    
    VLOG(3) << "empty and emptying, to_send:"<< to_send << ", allocated:" << allocated << "/" << buffer_size << " @ " << this << ", buf:" << (void*)buffer << "  completions_received:" << completions_received << ", allocated_count:" << allocated_count;
  
    // verify everything sent
    this->iterate([](impl::MessageBase* m) {
      if (!m->is_sent_ || !m->is_delivered_ || !m->is_enqueued_) {
        VLOG(1) << "!! message(" << m << ", is_sent_:" << m->is_sent_ << ", is_delivered_:" << m->is_delivered_ << ", m->is_enqueued_:" << m->is_enqueued_ << ", extra:" << m->pool << ")";
      }
    });
  
    this->reset();
  
    memset(buffer, (0x11*locale_mycore()) | 0xf0, buffer_size); // poison
  
    unused_pools.push(this);
  }

} // namespace Grappa
