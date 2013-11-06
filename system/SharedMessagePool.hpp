#pragma once

#include "PoolAllocator.hpp"
#include "MessageBase.hpp"
#include "MessageBaseImpl.hpp"
#include "Message.hpp"
#include <stack>
#include <glog/logging.h>

DECLARE_int64(shared_pool_size);
DECLARE_int64(shared_pool_max);

namespace Grappa {
/// @addtogroup Communication
/// @{

void* _shared_pool_alloc(size_t sz);

class SharedMessagePool;
extern SharedMessagePool * shared_pool;

class SharedMessagePool: public PoolAllocator<impl::MessageBase> {
public:
  SharedMessagePool *next; // for PiggybackStack
  
  bool emptying;
  int64_t to_send;
  
  // debugging
  int64_t completions_received;
  int64_t allocated_count;
  
  SharedMessagePool(size_t bytes = FLAGS_shared_pool_size)
    : PoolAllocator<impl::MessageBase>(locale_alloc<char>(bytes), bytes, true)
  { reset(); }
  
  void reset() {
    PoolAllocator::reset();
    emptying = false;
    to_send = 0;
    completions_received = 0;
    allocated_count = 0;
    next = nullptr;
  }
  
  void message_sent(impl::MessageBase* m);
  void on_empty();
  
  void validate_in_pool(impl::MessageBase* m) {
    CHECK( reinterpret_cast<char*>(m) >= this->buffer && reinterpret_cast<char*>(m)+m->size() <= this->buffer+this->buffer_size )
        << "message is outside this pool!! message(" << m << ", extra:" << m->pool << "), "
        << "pool(" << this << ", buf:" << (void*)this->buffer << ", size:" << this->buffer_size << ")";
  }
  
};

template<typename T>
class PoolMessage: public Message<T> {
public:  
  inline SharedMessagePool * shpool() {
    return reinterpret_cast<SharedMessagePool*>(this->pool);
  }
  virtual void mark_sent() {
    Message<T>::mark_sent();
    
// #ifdef DEBUG
//     if (get_pool().emptying) {
//       VLOG(5) << "pool == " << &get_pool() << " to_send:" << get_pool().to_send << "  message(" << this << ", src:" << this->source_ << ", dst:" << this->destination_ << ", sent:" << this->is_sent_  << ", enq:" << this->is_enqueued_ << ", deli:" << this->is_delivered_ << ")";
//     }
// #endif
    DCHECK_NE(this->destination_, 0x5555);
    if (Grappa::mycore() == this->source_) {
      // if (get_pool().emptying) VLOG(5) << "emptying @ " << &get_pool() << "(buf:" << (void*)get_pool().buffer << ")" << " to_send:" << get_pool().to_send << ", completions_received:" << get_pool().completions_received << ", allocated_count:" << get_pool().allocated_count << ", sent message(" << this << ")";
      
      DCHECK_NE(this->source_, 0x5555) << " sent:" << this->is_sent_ << ", pool(" << shpool() << ")";
      this->source_ = 0x5555;
      
      shpool()->message_sent(this); // may trash things
    }
  }
  
  inline PoolMessage(): Message<T>() {}
  inline PoolMessage(SharedMessagePool * pool, Core dest, T t)
    : Message<T>(dest, t)
  { this->pool = pool; shpool()->validate_in_pool(this); }
  
  virtual const size_t size() const { return sizeof(*this); }
} GRAPPA_BLOCK_ALIGNED;

template<typename T>
class PoolPayloadMessage: public PayloadMessage<T> {
public:
  inline SharedMessagePool * shpool() { return reinterpret_cast<SharedMessagePool*>(this->pool); }
  
  virtual void mark_sent() {
    PayloadMessage<T>::mark_sent();
    if (Grappa::mycore() == this->source_) {
      shpool()->message_sent(this); // may delete the pool
    }
  }
  inline PoolPayloadMessage(): PayloadMessage<T>() {}
  inline PoolPayloadMessage(SharedMessagePool * pool, Core dest, T t, void * payload, size_t payload_size)
    : PayloadMessage<T>(dest, t, payload, payload_size)
  { this->pool = pool;  shpool()->validate_in_pool(this); }
  virtual const size_t size() const { return sizeof(*this); }
} GRAPPA_BLOCK_ALIGNED;

void init_shared_pool();

// Same as message, but allocated on heap
template< typename T >
inline PoolMessage<T> * heap_message(Core dest, T t) {
  return new (_shared_pool_alloc(sizeof(PoolMessage<T>))) PoolMessage<T>(shared_pool, dest, t);
}

/// Message with payload, allocated on heap
template< typename T >
inline PoolPayloadMessage<T> * heap_message(Core dest, T t, void * payload, size_t payload_size) {
  return new (_shared_pool_alloc(sizeof(PoolPayloadMessage<T>)))
    PoolPayloadMessage<T>(shared_pool, dest, t, payload, payload_size);
}

/// Same as message, but allocated on heap and immediately enqueued to be sent.
template< typename T >
inline PoolMessage<T> * send_heap_message(Core dest, T t) {
  auto *m = new (_shared_pool_alloc(sizeof(PoolMessage<T>))) PoolMessage<T>(shared_pool, dest, t);
  m->enqueue();
  return m;
}

/// Message with payload, allocated on heap and immediately enqueued to be sent.
template< typename T >
inline PoolPayloadMessage<T> * send_heap_message(Core dest, T t, void * payload, size_t payload_size) {
  auto *m = new (_shared_pool_alloc(sizeof(PoolPayloadMessage<T>)))
    PoolPayloadMessage<T>(shared_pool, dest, t, payload, payload_size);
  m->enqueue();
  return m;
}

} // namespace Grappa
