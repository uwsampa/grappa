#include "Grappa.hpp"
#include "Message.hpp"

namespace Grappa {

  template<size_t Bytes, typename Base>
  class Pool {
    char buffer[Bytes];
    size_t allocated;
  protected:
    Base* allocate(size_t sz) {
      LOG(ERROR) << "allocating " << sz;
      Base* p = reinterpret_cast<Base*>(buffer+allocated);
      allocated += sz;
      
      // TODO: implement heap allocation fallback
      if (allocated > Bytes) { LOG(ERROR) << "MessagePool overflow!"; exit(1); }
      return p;
    }
    
  public:
    Pool(): allocated(0) {}
    
    virtual ~Pool() {
      // call destructors of everything in Pool
      iterate([](Base* bp){ bp->~Base(); });
    }
    
    /// Takes a lambda (or really any callable) that is called repeated for
    /// each allocated Base*
    template<typename F>
    void iterate(F f) {
      char * p = buffer;
      while (p < buffer+allocated) {
        Base* bp = reinterpret_cast<Base*>(p);
        p += bp->size();
        f(bp);
      }
    }
    
    template<size_t OtherBytes, typename OtherBase>
    friend void* ::operator new(size_t, Grappa::Pool<OtherBytes,OtherBase>&);
  };
  
  template<size_t Bytes>
  class MessagePool : public Pool<Bytes, impl::MessageBase> {
  public:
    
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

template<size_t Bytes,typename Base>
void* operator new(size_t size, Grappa::Pool<Bytes,Base>& a) {
  return a.allocate(size);
}
