#include "Grappa.hpp"
#include "PoolAllocator.hpp"
#include "Message.hpp"

namespace Grappa {
  
  template<size_t Bytes>
  class MessagePool : public PoolAllocator<Bytes, impl::MessageBase> {
  public:
    
    
    
    ///
    /// Templated message creating functions, all taken straight from Message.hpp
    ///
    
    template<typename T>
    inline Message<T>* message(Core dest, T t) {
      return new (*this) Message<T>(dest, t);
    }
    
    /// Message with payload
    template< typename T >
    inline PayloadMessage<T> message( Core dest, T t, void * payload, size_t payload_size ) {
      return new (*this) PayloadMessage<T>( dest, t, payload, payload_size );
    }
    
    /// Message with contents stored outside object
    template< typename T >
    inline ExternalMessage<T> message( Core dest, T * t ) {
      return new (*this) ExternalMessage<T>( dest, t );
    }
    
    /// Message with contents stored outside object as well as payload
    template< typename T >
    inline ExternalPayloadMessage<T> message( Core dest, T * t, void * payload, size_t payload_size ) {
      return new (*this) ExternalPayloadMessage<T>( dest, t, payload, payload_size );
    }
    
    /// Same as message, but immediately enqueued to be sent.
    template< typename T >
    inline SendMessage<T> send_message( Core dest, T t ) {
      return new (*this) SendMessage<T>( dest, t );
    }
    
    /// Message with payload, immediately enqueued to be sent.
    template< typename T >
    inline SendPayloadMessage<T> send_message( Core dest, T t, void * payload, size_t payload_size ) {
      return new (*this) SendPayloadMessage<T>( dest, t, payload, payload_size );
    }
    
    /// Message with contents stored outside object, immediately enqueued to be sent.
    template< typename T >
    inline SendExternalMessage<T> send_message( Core dest, T * t ) {
      return new (*this) SendExternalMessage<T>( dest, t );
    }
    
    /// 
    template< typename T >
    inline SendExternalPayloadMessage<T> send_message( Core dest, T * t, void * payload, size_t payload_size ) {
      return new (*this) SendExternalPayloadMessage<T>( dest, t, payload, payload_size );
    }
 
  };

}
