#include "SharedMessagePool.hpp"
#include "Statistics.hpp"
#include <stack>

DEFINE_int64(shared_pool_size, 1L << 20, "Size (in bytes) of global SharedMessagePool (on each Core)");

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
  }
  
  void SharedMessagePool::start_emptying() {
    emptying = true;
    // CHECK_GT(to_send, 0);
    if (to_send == 0 && emptying) { on_empty(); }
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
