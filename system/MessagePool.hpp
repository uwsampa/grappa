#pragma once

#include "Grappa.hpp"
#include "PoolAllocator.hpp"
#include "Message.hpp"

namespace Grappa {
  
  class MessagePoolExternal: public PoolAllocator<impl::MessageBase> {
  public:
    MessagePoolExternal(char * buffer, size_t sz): PoolAllocator<impl::MessageBase>(buffer, sz) {}
    
    /// Just calls `block_until_sent` on each message in the pool
    /// TODO: don't wake until all have been sent
    void block_until_all_sent() {
      this->iterate([](impl::MessageBase* msg){
        msg->block_until_sent();
      });
    }
    
    ///
    /// Templated message creating functions, all taken straight from Message.hpp
    ///
    
    template<typename T>
    inline Message<T>* message(Core dest, T t) {
      return new (*this) Message<T>(dest, t);
    }
    
    /// Message with payload
    template< typename T >
    inline PayloadMessage<T>* message( Core dest, T t, void * payload, size_t payload_size ) {
      return new (*this) PayloadMessage<T>( dest, t, payload, payload_size );
    }
    
    /// Message with contents stored outside object
    template< typename T >
    inline ExternalMessage<T>* message( Core dest, T * t ) {
      return new (*this) ExternalMessage<T>( dest, t );
    }
    
    /// Message with contents stored outside object as well as payload
    template< typename T >
    inline ExternalPayloadMessage<T>* message( Core dest, T * t, void * payload, size_t payload_size ) {
      return new (*this) ExternalPayloadMessage<T>( dest, t, payload, payload_size );
    }
    
    /// Same as message, but immediately enqueued to be sent.
    template< typename T >
    inline Message<T> * send_message( Core dest, T t ) {
      Message<T> * m = new (*this) Message<T>( dest, t );
      m->enqueue();
      return m;
    }
    
    /// Message with payload, immediately enqueued to be sent.
    template< typename T >
    inline PayloadMessage<T> * send_message( Core dest, T t, void * payload, size_t payload_size ) {
      PayloadMessage<T> * m = new (*this) PayloadMessage<T>( dest, t, payload, payload_size );
      m->enqueue();
      return m;
    }
    
    /// Message with contents stored outside object, immediately enqueued to be sent.
    template< typename T >
    inline ExternalMessage<T> * send_message( Core dest, T * t ) {
      ExternalMessage<T> * m = new (*this) ExternalMessage<T>( dest, t );
      m->enqueue();
      return m;
    }
    
    /// Message with contents stored outside object as well as payload, immediately enqueued to be sent.
    template< typename T >
    inline ExternalPayloadMessage<T> * send_message( Core dest, T * t, void * payload, size_t payload_size ) {
      ExternalPayloadMessage<T> * m = new (*this) ExternalPayloadMessage<T>( dest, t, payload, payload_size );
      m->enqueue();
      return m;
    }
 
  };

  
  template<size_t Bytes>
  class MessagePool : public MessagePoolExternal {
    char _buffer[Bytes];
  public:
    MessagePool(): MessagePoolExternal(_buffer, Bytes) {}
  };
}
