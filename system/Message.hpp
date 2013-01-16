
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __MESSAGE_HPP__
#define __MESSAGE_HPP__

#include "MessageBase.hpp"

#include <glog/logging.h>

namespace Grappa {

  /// @addtogroup Communication
  /// @{

  /// A standard message. Storage is internal. Destructor blocks until message is sent.
  /// Best used through @ref message function.
  template< typename T >
  class Message : public Grappa::impl::MessageBase {
  private:
    T storage_;                  ///< Storage for message contents.
    
  public:
    /// Construct a message.
    inline Message( )
      : MessageBase()
      , storage_()
    { }
    
    /// Construct a message.
    /// @param dest ID of destination core.
    /// @param t Contents of message to send.
    inline Message( Core dest, T t )
      : MessageBase( dest )
      , storage_( t )
    { }

    /// Default copy constructor is fine.
    Message( const Message& m ) = delete; ///< Not allowed.
    Message operator=( const Message& m ) = delete;         ///< Not allowed.

    ///
    /// for Messages with modifiable contents. Don't use with lambdas.
    ///

    /// Access message contents.
    inline T& operator*() {
      return storage_;
    }
    
    /// Access message contents.
    inline T* operator->() {
      return &storage_;
    }
    
    /// How much storage do we need to send this message?
    const size_t size( ) const {
      return sizeof( &deserialize_and_call ) + sizeof( T );
    }


    ///
    /// These are used by the aggregator to send and receive messages.
    ///

    /// Deserialize and call one of these messages from a buffer. This
    /// is run on the remote machine.
    ///
    /// @param t address of message functor/contents in buffer
    /// @return address of byte following message functor/contents in buffer
    static char * deserialize_and_call( char * t ) {
      DVLOG(5) << "Deserializing message";
      T * obj = reinterpret_cast< T * >( t );
      (*obj)();
      return t + sizeof( T );
    }

    /// Copy this message into a buffer.
    virtual char * serialize_to( char * p, size_t max_size ) {
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
	
	// copy contents
	std::memcpy( p, &storage_, sizeof(storage_) );
	
	//DVLOG(5) << "serialized message of size " << sizeof(fp) + sizeof(T);
	
	// return pointer following message
	return p + sizeof( T );
      }
    }

  };


  /// A message with dynamic payload. Storage for message contents is
  /// internal, but payload is stored externally. Destructor blocks
  /// until message is sent. Best used through @ref message function.
  template< typename T >
  class PayloadMessage : public Grappa::impl::MessageBase {
  private:
    T storage_;                  ///< Storage for message contents.
    
    void * payload_;
    size_t payload_size_;

  public:
    /// Construct a message.
    inline PayloadMessage( )
      : MessageBase()
      , storage_()
      , payload_( nullptr )
      , payload_size_( 0 )
    { }
    
    /// Construct a message.
    /// @param dest ID of destination core.
    /// @param t Contents of message to send.
    inline PayloadMessage( Core dest, T t, void * payload, size_t payload_size )
      : MessageBase( dest )
      , storage_( t )
      , payload_( payload )
      , payload_size_( payload_size )
    { }

    // /// Construct a message and enqueue to be sent immediately.
    // /// @param dest ID of destination core.
    // /// @param t Contents of message to send.
    // inline PayloadMessage Send( Core dest, T t )
    //   : MessageBase( dest )
    //   , storage_( t )
    // { 
    //   LOG(FAIL) << "Probably doesn't work properly yet.";
    //   send();
    // }

    /// Default copy constructor is fine.
    //PayloadMessage( const PayloadMessage& m ) = delete; ///< Not allowed.
    PayloadMessage operator=( const PayloadMessage& m ) = delete;         ///< Not allowed.

    ///
    /// for Messages with modifiable contents. Don't use with lambdas.
    ///

    /// Access message contents.
    inline T& operator*() {
      return storage_;
    }
    
    /// Access message contents.
    inline T* operator->() {
      return &storage_;
    }



    /// How much storage do we need to send this message?
    const size_t size( ) const {
      return sizeof( &deserialize_and_call ) + sizeof( T ) + sizeof( int16_t ) + payload_size_;
    }


    ///
    /// These are used by the aggregator to send and receive messages.
    ///

    /// Deserialize and call one of these messages from a buffer. This
    /// is run on the remote machine.
    ///
    /// @param t address of message functor/contents in buffer
    /// @return address of byte following message functor/contents in buffer
    static char * deserialize_and_call( char * t ) {
      // DVLOG(5) << "Deserializing message";
      T * obj = reinterpret_cast< T * >( t );
      t += sizeof( T );

      int16_t payload_size = *(reinterpret_cast< int16_t* >(t));
      t += sizeof( int16_t );

      (*obj)( t, payload_size );

      return t + payload_size;
    }

    /// Copy this message into a buffer.
    virtual char * serialize_to( char * p, size_t max_size ) {
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
	
	// copy contents
	std::memcpy( p, &storage_, sizeof(storage_) );
	p += sizeof( storage_ );

	*(reinterpret_cast< int16_t* >(p)) = static_cast< int16_t >( payload_size_ );
	p += sizeof( int16_t );

	std::memcpy( p, payload_, payload_size_);

	// return pointer following message
	return p + payload_size_;
      }
    }

  };

  /// A message with the contents stored outside the object. Destructor blocks until message is sent.
  /// Best used through @ref message function.
  template< typename T >
  class ExternalMessage : public Grappa::impl::MessageBase {
  private:
    T * pointer_;                ///< Pointer to message contents.

  public:
    /// Construct a message.
    inline ExternalMessage( )
      : MessageBase()
      , pointer_( nullptr )
    { }
    
    /// Construct a message.
    /// @param dest ID of destination core.
    /// @param t Pointer to contents of message to send.
    inline ExternalMessage( Core dest, T * t )
      : MessageBase( dest )
      , pointer_( t )
    { }

    // /// Construct a message and enqueue to be sent immediately.
    // /// @param dest ID of destination core.
    // /// @param t Contents of message to send.
    // inline ExternalMessage Send( Core dest, T * t )
    //   : MessageBase( dest )
    //   , pointer_( t )
    // { 
    //   LOG(FAIL) << "Probably doesn't work properly yet.";
    //   send();
    // }

    /// Default copy constructor is fine.
    //ExternalMessage( const ExternalMessage& m ) = delete; ///< Not allowed.
    ExternalMessage operator=( const ExternalMessage& m ) = delete;         ///< Not allowed.

    ///
    /// for Messages with modifiable contents. Don't use with lambdas.
    ///

    /// Access message contents.
    inline T&  operator*() {
      return *pointer_;
    }
    
    /// Access message contents.
    inline T* operator->() {
      return pointer_;
    }



    /// How much storage do we need to send this message?
    const size_t size( ) const {
      return sizeof( &deserialize_and_call ) + sizeof( T );
    }


    ///
    /// These are used by the aggregator to send and receive messages.
    ///

    /// Deserialize and call one of these messages from a buffer. This
    /// is run on the remote machine.
    ///
    /// @param t address of message functor/contents in buffer
    /// @return address of byte following message functor/contents in buffer
    static char * deserialize_and_call( char * t ) {
      // DVLOG(5) << "Deserializing message";
      T * obj = reinterpret_cast< T * >( t );
      (*obj)();
      return t + sizeof( T );
    }

    /// Copy this message into a buffer.
    virtual char * serialize_to( char * p, size_t max_size ) {
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
	
	// copy contents
	std::memcpy( p, pointer_, sizeof(T) );
	
	//DVLOG(5) << "serialized message of size " << sizeof(fp) + sizeof(T);
	
	// return pointer following message
	return p + sizeof( T );
      }
    }

  };


  /// A message with dynamic payload. Storage for message contents and
  /// payload is external. Destructor blocks until message is
  /// sent. Best used through @ref message function.
  template< typename T >
  class ExternalPayloadMessage : public Grappa::impl::MessageBase {
  private:
    T * pointer_;                  ///< Pointer to message contents.
    
    void * payload_;
    size_t payload_size_;

  public:
    /// Construct a message.
    inline ExternalPayloadMessage( )
      : MessageBase()
      , pointer_( nullptr )
      , payload_( nullptr )
      , payload_size_( 0 )
    { }
    
    /// Construct a message.
    /// @param dest ID of destination core.
    /// @param t Contents of message to send.
    inline ExternalPayloadMessage( Core dest, T * t, void * payload, size_t payload_size )
      : MessageBase( dest )
      , pointer_( t )
      , payload_( payload )
      , payload_size_( payload_size )
    { }

    // /// Construct a message and enqueue to be sent immediately.
    // /// @param dest ID of destination core.
    // /// @param t Contents of message to send.
    // inline PayloadMessage Send( Core dest, T t )
    //   : MessageBase( dest )
    //   , storage_( t )
    // { 
    //   LOG(FAIL) << "Probably doesn't work properly yet.";
    //   send();
    // }

    /// Default copy constructor is fine.
    //ExternalPayloadMessage( const ExternalPayloadMessage& m ) = delete; ///< Not allowed.
    ExternalPayloadMessage operator=( const ExternalPayloadMessage& m ) = delete;         ///< Not allowed.

    ///
    /// for Messages with modifiable contents. Don't use with lambdas.
    ///

    /// Access message contents.
    inline T& operator*() {
      return *pointer_;
    }
    
    /// Access message contents.
    inline T* operator->() {
      return pointer_;
    }



    /// How much storage do we need to send this message?
    virtual const size_t size( ) const {
      return sizeof( &deserialize_and_call ) + sizeof( T ) + sizeof( int16_t ) + payload_size_;
    }



    ///
    /// These are used by the aggregator to send and receive messages.
    ///

    /// Deserialize and call one of these messages from a buffer. This
    /// is run on the remote machine.
    ///
    /// @param t address of message functor/contents in buffer
    /// @return address of byte following message functor/contents in buffer
    static char * deserialize_and_call( char * t ) {
      T * obj = reinterpret_cast< T * >( t );
      t += sizeof( T );

      int16_t payload_size = *(reinterpret_cast< int16_t* >(t));
      t += sizeof( int16_t );

      (*obj)( t, payload_size );

      return t + payload_size;
    }

    /// Copy this message into a buffer.
    virtual char * serialize_to( char * p, size_t max_size ) {
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
	
	// copy contents
	std::memcpy( p, pointer_, sizeof(T) );
	p += sizeof(T);

	*(reinterpret_cast< int16_t* >(p)) = static_cast< int16_t >( payload_size_ );
	p += sizeof( int16_t );

	std::memcpy( p, payload_, payload_size_);

	// return pointer following message
	return p + payload_size_;
      }
    }

  };






  /// Construct a message allocated on the stack. Can be used with
  /// lambdas, functors, or function pointers.
  /// Example:
  /// @code 
  ///   // using a lambda
  ///   auto m1 = Grappa::message( destination, [] { blah(); } );
  ///
  ///   // using a functor
  ///   class Functor { ... };
  ///   auto m2 = Grappa::message( destination, Functor( args ) );
  ///
  ///   // using a function pointer (via std::bind functor)
  ///   void foo( int bar, double baz ) { blah(); }
  ///   auto m3 = Grappa::message( destination, std::bind( &foo, 1, 2.3 ) );
  /// @endcode
  /// @param dest Core to which message will be delivered
  /// @param t Functor or function pointer that will be called on destination core
  /// @return Message object
  template< typename T >
  inline Message<T> message( Core dest, T t ) {
    return Message<T>( dest, t );
  }

  /// Message with payload
  template< typename T >
  inline PayloadMessage<T> message( Core dest, T t, void * payload, size_t payload_size ) {
    return PayloadMessage<T>( dest, t, payload, payload_size );
  }

  /// Message with contents stored outside object
  template< typename T >
  inline ExternalMessage<T> message( Core dest, T * t ) {
    return ExternalMessage<T>( dest, t );
  }

  /// Message with contents stored outside object as well as payload
  template< typename T >
  inline ExternalPayloadMessage<T> message( Core dest, T * t, void * payload, size_t payload_size ) {
    return ExternalPayloadMessage<T>( dest, t, payload, payload_size );
  }





  template< typename T >
  class SendMessage : public Message<T> {
  public:
    /// Construct a message and enqueue to be sent immediately.
    /// @param dest ID of destination core.
    /// @param t Contents of message to send.
    inline SendMessage( Core dest, T t )
      : Message<T>( dest, t )
    { 
      Message<T>::send();
    }
  };

  template< typename T >
  class SendPayloadMessage : public PayloadMessage<T> {
  public:
    /// Construct a message and enqueue to be sent immediately.
    /// @param dest ID of destination core.
    /// @param t Contents of message to send.
    /// @param payload Pointer to payload
    /// @param payload_size Size of payload
    inline SendPayloadMessage( Core dest, T t, void * payload, size_t payload_size )
      : PayloadMessage<T>( dest, t, payload, payload_size )
    { 
      PayloadMessage<T>::send();
    }
  };

  template< typename T >
  class SendExternalMessage : public ExternalMessage<T> {
  public:
    /// Construct a message and enqueue to be sent immediately.
    /// @param dest ID of destination core.
    /// @param t Pointer to contents of message to send.
    inline SendExternalMessage( Core dest, T * t )
      : ExternalMessage<T>( dest, t )
    { 
      ExternalMessage<T>::send();
    }
  };

  template< typename T >
  class SendExternalPayloadMessage : public ExternalPayloadMessage<T> {
  public:
    /// Construct a message and enqueue to be sent immediately.
    /// @param dest ID of destination core.
    /// @param t Pointer to contents of message to send.
    /// @param payload Pointer to payload
    /// @param payload_size Size of payload
    inline SendExternalPayloadMessage( Core dest, T * t, void * payload, size_t payload_size )
      : ExternalPayloadMessage<T>( dest, t, payload, payload_size )
    { 
      ExternalPayloadMessage<T>::send();
    }
  };


  /// Same as message, but immediately enqueued to be sent.
  template< typename T >
  inline Message<T> send_message( Core dest, T t ) {
    LOG(ERROR) << "Probably doesn't work properly yet.";
    return SendMessage<T>( dest, t );
  }

  /// Message with payload, immediately enqueued to be sent.
  template< typename T >
  inline PayloadMessage<T> send_message( Core dest, T t, void * payload, size_t payload_size ) {
    LOG(ERROR) << "Probably doesn't work properly yet.";
    return SendPayloadMessage<T>( dest, t );
  }

  /// Message with contents stored outside object, immediately enqueued to be sent.
  template< typename T >
  inline ExternalMessage<T> send_message( Core dest, T * t ) {
    LOG(ERROR) << "Probably doesn't work properly yet.";
    return SendExternalMessage<T>( dest, t );
  }

  /// Message with contents stored outside object as well as payload, immediately enqueued to be sent.
  template< typename T >
  inline ExternalPayloadMessage<T> send_message( Core dest, T * t, void * payload, size_t payload_size ) {
    LOG(ERROR) << "Probably doesn't work properly yet.";
    return SendExternalPayloadMessage<T>( dest, t, payload, payload_size );
  }

  /// @}

}

#endif
