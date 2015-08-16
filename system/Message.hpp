////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#ifndef __MESSAGE_HPP__
#define __MESSAGE_HPP__

#include "MessageBase.hpp"
#include "MessageBaseImpl.hpp"

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

    Message( const Message& m ) = delete; ///< Not allowed.
    Message& operator=( const Message& m ) = delete;         ///< Not allowed.
    Message& operator=( Message&& m ) = delete;

    Message( Message&& m ) = default;

    virtual ~Message() {
      block_until_sent();
    }

    virtual const char * typestr() {
      return "Message()"; //typename_of(*this);
    }

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
    virtual const size_t serialized_size( ) const {
      return sizeof( &deserialize_and_call ) + sizeof( T );
    }
    
    virtual const size_t size() const { return sizeof(*this); }

    ///
    /// These are used by the aggregator to send and receive messages.
    ///

    /// Deserialize and call one of these messages from a buffer. This
    /// is run on the remote machine.
    ///
    /// @param t address of message functor/contents in buffer
    /// @return address of byte following message functor/contents in buffer
    static char * deserialize_and_call( char * t ) {
      DVLOG(5) << "In " << __PRETTY_FUNCTION__;
      T * obj = reinterpret_cast< T * >( t );
      (*obj)();
      return t + sizeof( T );
    }

    virtual void deliver_locally() {
      DVLOG(5) << __func__ << ": " << this << " Delivering locally with is_enqueued_=" << this->is_enqueued_ 
             << " is_delivered_=" << this->is_delivered_ 
             << " is_sent_=" << this->is_sent_;
      if( !is_delivered_ ) {
        storage_();
        is_delivered_ = true;
      }
      this->mark_sent();
    }

    /// Copy this message into a buffer.
    virtual char * serialize_to( char * p, size_t max_size ) {
      Grappa::impl::MessageBase::serialize_to( p, max_size );
      // copy deserialization function pointer
      auto fp = &deserialize_and_call;
      if( serialized_size() > max_size ) {
        return p;
      } else {
        // // turn into 2D pointer
        //auto gfp = make_global( fp, destination_ );
        // // write to buffer
        //*(reinterpret_cast< intptr_t* >(p)) = reinterpret_cast< intptr_t >( gfp );
        //*(reinterpret_cast< intptr_t* >(p)) = reinterpret_cast< intptr_t >( fp );
        //intptr_t gfp = reinterpret_cast< intptr_t >( fp ) << 16;
        //gfp |= destination_; // TODO: watch out for sign extension
        //*(reinterpret_cast< intptr_t* >(p)) = gfp;
        // p += sizeof( fp );

        MessageFPAddr gfp = { destination_, reinterpret_cast< intptr_t >( fp ) };
        *(reinterpret_cast< MessageFPAddr* >(p)) = gfp;
        static_assert( sizeof(gfp) == 8, "gfp wrong size?" );
        p += sizeof( gfp );

        
        // copy contents
        std::memcpy( p, &storage_, sizeof(storage_) );
        
        DVLOG(5) << __PRETTY_FUNCTION__ << " serialized message of size " << sizeof(fp) + sizeof(storage_) << " to " << gfp.dest << " with deserializer " << fp;
        
        // return pointer following message
        return p + sizeof( T );
      }
    }

  } __attribute__((aligned(64)));

  /// A message with dynamic payload. Storage for message contents is
  /// internal, but payload is stored externally. Destructor blocks
  /// until message is sent. Best used through @ref message function.
  template< typename T >
  class PayloadMessage : public Grappa::impl::MessageBase {
  private:
    T storage_;                  ///< Storage for message contents.
    
    void * payload_;
    size_t payload_size_;

    bool delete_payload_after_send_;

  public:
    /// Construct a message.
    inline PayloadMessage( )
      : MessageBase()
      , storage_()
      , payload_( nullptr )
      , payload_size_( 0 )
      , delete_payload_after_send_( false )
    { }
    
    /// Construct a message.
    /// @param dest ID of destination core.
    /// @param t Contents of message to send.
    /// @param payload pointer to payload buffer
    /// @param payload_size size of payload (in bytes)
    inline PayloadMessage( Core dest, T t, void * payload, size_t payload_size )
      : MessageBase( dest )
      , storage_( t )
      , payload_( payload )
      , payload_size_( payload_size )
      , delete_payload_after_send_( false )
    { 
      Grappa::impl::locale_shared_memory.validate_address( payload );
    }

    PayloadMessage( const PayloadMessage& m ) = delete; ///< Not allowed.
    PayloadMessage& operator=( const PayloadMessage& m ) = delete;         ///< Not allowed.
    PayloadMessage& operator=( PayloadMessage&& m ) = delete;         ///< Not allowed.

    PayloadMessage( PayloadMessage&& m ) = default;
    
    virtual ~PayloadMessage() {
      block_until_sent();
      //if( delete_payload_after_send_ ) delete payload_;
    }

    inline void set_payload( void * payload, size_t size ) {
      payload_ = payload;
      payload_size_ = size;
      Grappa::impl::locale_shared_memory.validate_address( payload );
    }

    inline void delete_payload_after_send() { delete_payload_after_send_ = true; }

    virtual void reset() {
      Grappa::impl::MessageBase::reset();
      payload_ = nullptr;
      payload_size_ = 0;
      delete_payload_after_send_ = false;
    }

    virtual const char * typestr() {
      return "Message"; //typename_of(*this);
    }

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
    virtual const size_t serialized_size( ) const {
      return sizeof( &deserialize_and_call ) + sizeof( T ) + sizeof( int16_t ) + payload_size_;
    }

    virtual const size_t size() const { return sizeof(*this); }
    
    ///
    /// These are used by the aggregator to send and receive messages.
    ///

    /// Deserialize and call one of these messages from a buffer. This
    /// is run on the remote machine.
    ///
    /// @param t address of message functor/contents in buffer
    /// @return address of byte following message functor/contents in buffer
    static char * deserialize_and_call( char * t ) {
      DVLOG(5) << "In " << __PRETTY_FUNCTION__;
      T * obj = reinterpret_cast< T * >( t );
      t += sizeof( T );

      int16_t payload_size = *(reinterpret_cast< int16_t* >(t));
      t += sizeof( int16_t );

      (*obj)( t, payload_size );

      return t + payload_size;
    }

    virtual void deliver_locally() {
      if( !is_delivered_ ) {
        storage_( payload_, payload_size_ );
        is_delivered_ = true;
      }
      this->mark_sent();
    }

    /// Copy this message into a buffer.
    virtual char * serialize_to( char * p, size_t max_size ) {
      Grappa::impl::MessageBase::serialize_to( p, max_size );
      // copy deserialization function pointer
      auto fp = &deserialize_and_call;
      if( serialized_size() > max_size ) {
        return p;
      } else {
        // // turn into 2D pointer
        // auto gfp = make_global( fp, destination_ );
        // // write to buffer
        // *(reinterpret_cast< intptr_t* >(p)) = reinterpret_cast< intptr_t >( gfp );
        //*(reinterpret_cast< intptr_t* >(p)) = reinterpret_cast< intptr_t >( fp );

        // intptr_t gfp = reinterpret_cast< intptr_t >( fp ) << 16;
        // gfp |= destination_;
        // *(reinterpret_cast< intptr_t* >(p)) = gfp;
        // p += sizeof( fp );

        MessageFPAddr gfp = { destination_, reinterpret_cast< intptr_t >( fp ) };
        *(reinterpret_cast< MessageFPAddr* >(p)) = gfp;
        static_assert( sizeof(gfp) == 8, "gfp wrong size?" );
        p += sizeof( gfp );

        
        // copy contents
        std::memcpy( p, &storage_, sizeof(storage_) );
        p += sizeof( storage_ );

        *(reinterpret_cast< int16_t* >(p)) = static_cast< int16_t >( payload_size_ );
        p += sizeof( int16_t );

        std::memcpy( p, payload_, payload_size_);

        DVLOG(5) << __PRETTY_FUNCTION__ << " serialized message of size " << sizeof(fp) + sizeof(storage_) + sizeof(int16_t) + payload_size_ << " to " << gfp.dest << " with deserializer " << fp;

        // return pointer following message
        return p + payload_size_;
      }
    }

  } __attribute__((aligned(64)));






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






  /// Same as message, but immediately enqueued to be sent.
  template< typename T >
  inline Message<T> send_message( Core dest, T t ) {
    Message<T> m( dest, t );
    m.enqueue();
    return m;
  }
  
  /// Message with payload, immediately enqueued to be sent.
  template< typename T >
  inline PayloadMessage<T> send_message( Core dest, T t, void * payload, size_t payload_size ) {
    PayloadMessage<T> m( dest, t, payload, payload_size );
    m.enqueue();
    return m;
  }

  
  /// @}

}

#include "SharedMessagePool.hpp" // get heap_message stuff...

#endif
