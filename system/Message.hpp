
#ifndef __MESSAGE_HPP__
#define __MESSAGE_HPP__

#include "MessageBase.hpp"

namespace Grappa {

  /// @addtogroup Communication
  /// @{

  /// A standard message. Storage is internal. Destrutor blocks until message is sent.
  /// Best used through @ref scopedMessage function.
  template< typename T >
  class Message : public Grappa::impl::MessageBase {
  private:
    T payload_;                  ///< Storage for message payload.
    
  public:
    Message();                   ///< Not allowed.
    Message( const Message& m ); ///< Not allowed.


    /// Construct a message.
    /// @param dest ID of destination core.
    /// @param t Payload of message to send.
    inline Message( Core dest, T t )
      : MessageBase( dest )
      , payload_( t )
    { }
    
    // Message( const Message<T>& m t )
    //   : next_( NULL )
    //   , payload_( t )
    //   , is_enqueued_( false )
    //   , is_sent_( false )
    // { }

    // Message()
    // 	: payload_( NULL )
    // 	, next_( NULL )
    // 	, is_enqueued_( false )
    // 	, is_sent_( false )
    // { }

    // operator T*() {
    // 	return &payload_;
    // }
    
    /// Deserialize and call one of these messages from a buffer.
    /// @param t address of message functor/payload in buffer
    /// @return address of byte following message functor/payload in buffer
    static char * deserialize_and_call( char * t ) {
      // DVLOG(5) << "Deserializing message";
      T * obj = reinterpret_cast< T * >( t );
      (*obj)();
      return t + sizeof( T );
    }

    /// Copy this message into a buffer.
    char * serialize_to( char * p, size_t max_size ) {
      // copy deserialization function pointer
      auto fp = &deserialize_and_call;
      if( sizeof( fp ) + sizeof( T ) > max_size ) {
	std::cout << "break" << std::endl;
	return p;
      } else {
	// // turn into 2D pointer
	// auto gfp = make_global( fp, destination_ );
	// // write to buffer
	// *(reinterpret_cast< intptr_t* >(p)) = reinterpret_cast< intptr_t >( gfp );
	*(reinterpret_cast< intptr_t* >(p)) = reinterpret_cast< intptr_t >( fp );
	p += sizeof( fp );
	
	// copy payload
	std::memcpy( p, &payload_, sizeof(payload_) );
	
	//DVLOG(5) << "serialized message of size " << sizeof(fp) + sizeof(T);
	
	// return pointer following message
	return p + sizeof( T );
      }
    }

    size_t size( ) {
      return sizeof( &deserialize_and_call ) + sizeof( T );
    }

  };

  /// Construct a message allocated on the stack. Can be used with
  /// lambdas, functors, or function pointers.
  /// Example:
  /// @code 
  ///   // using a lambda
  ///   auto m1 = scopedMessage( destination, [] { blah(); } );
  ///
  ///   // using a functor
  ///   class Functor { ... };
  ///   auto m2 = scopedMessage( destination, Functor( args ) );
  ///
  ///   // using a function pointer (via std::bind functor)
  ///   void foo( int bar, double baz ) { blah(); }
  ///   auto m3 = scopedMessage( destination, std::bind( &foo, 1, 2.3 ) );
  /// @endcode
  /// @param dest Core to which message will be delivered
  /// @param t Functor or function pointer that will be called on destination core
  /// @return Message object
  template< typename T >
  inline Message<T> scopedMessage( Core dest, T t ) {
    return Message<T>( dest, t );
  }

  /// @}

}

#endif
