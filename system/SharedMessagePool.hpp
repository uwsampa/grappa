#pragma once

#include "PoolAllocator.hpp"
#include "MessageBase.hpp"
#include "MessageBaseImpl.hpp"
#include "Message.hpp"
#include <stack>
#include <glog/logging.h>

namespace Grappa {
/// @addtogroup Communication
/// @{

template<typename T> class PoolMessage;
template<typename T> class PoolPayloadMessage;

class SharedMessagePool;
extern SharedMessagePool * shared_pool;


class SharedMessagePool: public PoolAllocator<impl::MessageBase> {
public:
  bool emptying;
  int64_t to_send;
  
  // debugging
  int64_t completions_received;
  int64_t allocated_count;
  
  SharedMessagePool(size_t bytes)
    : PoolAllocator<impl::MessageBase>(
        reinterpret_cast<char*>(Grappa::impl::locale_shared_memory.allocate_aligned(bytes, 8)), bytes, true)
    , emptying(false)
    , to_send(0)
    , completions_received(0)
    , allocated_count(0)
  { }
  
  void reset() {
    PoolAllocator<impl::MessageBase>::reset();
    emptying = false;
    to_send = 0;
    completions_received = 0;
    allocated_count = 0;
  }
  
  void start_emptying();
  virtual void * allocate(size_t size);
  void message_sent(impl::MessageBase* m);
  void on_empty();
  
  void validate_in_pool(impl::MessageBase* m) {
    CHECK( reinterpret_cast<char*>(m) >= this->buffer && reinterpret_cast<char*>(m)+m->size() <= this->buffer+this->buffer_size )
        << "message is outside this pool!! message(" << m << ", extra:" << m->pool << "), "
        << "pool(" << this << ", buf:" << (void*)this->buffer << ", size:" << this->buffer_size << ")";
  }
  
  ///
  /// Templated message creating functions, all taken straight from Message.hpp
  ///
  
  /// Send message, allocating from the pool. @see Grappa::message
  template<typename T>
  inline PoolMessage<T>* message(Core dest, T t) {
    void* p = this->allocate(sizeof(PoolMessage<T>));
    return new (p) PoolMessage<T>(shared_pool, dest, t);
  }
  
  /// Message with payload. @see Grappa::message
  template< typename T >
  inline PoolPayloadMessage<T>* message(Core dest, T t, void * payload, size_t payload_size) {
    void* p = this->allocate(sizeof(PoolPayloadMessage<T>));
    return new (p) PoolPayloadMessage<T>(shared_pool, dest, t, payload, payload_size);
  }
  
  /// Same as message, but immediately enqueued to be sent. @see Grappa::send_message
  template< typename T >
  inline PoolMessage<T> * send_message(Core dest, T t) {
    auto* m = this->message(dest, t);
    m->enqueue();
    return m;
  }
  
  /// Message with payload, immediately enqueued to be sent. @see Grappa::send_message
  template< typename T >
  inline PoolPayloadMessage<T> * send_message(Core dest, T t, void * payload, size_t payload_size) {
    auto* m = this->message(dest, t, payload, payload_size);
    m->enqueue();
    return m;
  }
  
};

template<typename T>
class PoolMessage: public Message<T> {
public:  
  inline SharedMessagePool& get_pool() { return *reinterpret_cast<SharedMessagePool*>(this->pool); }
  virtual void mark_sent() {
    Message<T>::mark_sent();
    
#ifdef DEBUG
    if (get_pool().emptying) {
      VLOG(5) << "pool == " << &get_pool() << " to_send:" << get_pool().to_send << "  message(" << this << ", src:" << this->source_ << ", dst:" << this->destination_ << ", sent:" << this->is_sent_  << ", enq:" << this->is_enqueued_ << ", deli:" << this->is_delivered_ << ")";
    }
#endif
    DCHECK_NE(this->destination_, 0x5555);
    if (Grappa::mycore() == this->source_) {
      if (get_pool().emptying) VLOG(5) << "emptying @ " << &get_pool() << "(buf:" << (void*)get_pool().buffer << ")" << " to_send:" << get_pool().to_send << ", completions_received:" << get_pool().completions_received << ", allocated_count:" << get_pool().allocated_count << ", sent message(" << this << ")";
      
      DCHECK_NE(this->source_, 0x5555) << " sent:" << this->is_sent_ << ", pool(" << &get_pool() << ")";
      this->source_ = 0x5555;
      
      get_pool().message_sent(this); // may trash things
    }
  }
  
  inline PoolMessage(): Message<T>() {}
  inline PoolMessage(SharedMessagePool * pool, Core dest, T t)
    : Message<T>(dest, t)
  { this->pool = pool;  get_pool().validate_in_pool(this); }
  
  virtual const size_t size() const { return sizeof(*this); }
} __attribute__((aligned(64)));

template<typename T>
class PoolPayloadMessage: public PayloadMessage<T> {
public:
  inline SharedMessagePool& get_pool() { return *reinterpret_cast<SharedMessagePool*>(this->pool); }
  
  virtual void mark_sent() {
    PayloadMessage<T>::mark_sent();
    if (Grappa::mycore() == this->source_) {
      get_pool().message_sent(this); // may delete the pool
    }
  }
  inline PoolPayloadMessage(): PayloadMessage<T>() {}
  inline PoolPayloadMessage(SharedMessagePool * pool, Core dest, T t, void * payload, size_t payload_size)
    : PayloadMessage<T>(dest, t, payload, payload_size)
  { this->pool = pool;  get_pool().validate_in_pool(this); }
  virtual const size_t size() const { return sizeof(*this); }
} __attribute__((aligned(64)));


void init_shared_pool();

// Same as message, but allocated on heap
template< typename T >
inline PoolMessage<T> * heap_message(Core dest, T t) {
  return shared_pool->message(dest, t);
}

/// Message with payload, allocated on heap
template< typename T >
inline PoolPayloadMessage<T> * heap_message(Core dest, T t, void * payload, size_t payload_size) {
  return shared_pool->message(dest, t, payload, payload_size);
}

/// Same as message, but allocated on heap and immediately enqueued to be sent.
template< typename T >
inline PoolMessage<T> * send_heap_message(Core dest, T t) {
  return shared_pool->send_message(dest, t);
}

/// Message with payload, allocated on heap and immediately enqueued to be sent.
template< typename T >
inline PoolPayloadMessage<T> * send_heap_message(Core dest, T t, void * payload, size_t payload_size) {
  return shared_pool->send_message(dest, t, payload, payload_size);
}

} // namespace Grappa
