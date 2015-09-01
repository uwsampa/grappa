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

#ifndef __MESSAGEBASE_HPP__
#define __MESSAGEBASE_HPP__

#include <cstring>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "common.hpp"
#include "ConditionVariableLocal.hpp"
#include "Mutex.hpp"
#include "LocaleSharedMemory.hpp"

typedef int16_t Core;

namespace Grappa {
  
  // forward declarations
  namespace impl { class MessageBase; }
  namespace SharedMessagePool { void free(impl::MessageBase * m, size_t sz); }
  
  /// Internal messaging functions
  namespace impl {
  
  union MessageFPAddr {
    struct {
      Core dest : 16;
      intptr_t fp : 48;
    };
    intptr_t raw;
  };

    /// @addtogroup Communication
    /// @{

    /// Base class for messages. Includes support for the three main features of a message:
    ///  - pointers and flags to support storing messages in a linked list
    ///  - interface for message serialization
    ///  - storage for blocking worker until message is sent
    /// This is a virtual class.
    class MessageBase {
    public:
      MessageBase * next_;     ///< what's the next message in the list of messages to be sent? 
      MessageBase * prefetch_; ///< what's the next message to prefetch?

      ConditionVariable cv_;   ///< condition variable for sleep/wake

      union {
        struct {
          bool delete_after_send_ : 1; ///< Is this a heap message? Should it be deleted after it's sent?
          bool is_enqueued_ : 1;       ///< Have we been added to the send queue?
          bool is_sent_ : 1;           ///< Is our payload no longer needed?
          bool is_delivered_ : 1;      ///< Are we waiting to mark the message sent?
          bool is_moved_ : 1;          ///< HACK: make sure we don't try to send ourselves if we're just a temporary
          Core source_ : 16;           ///< What core is this message coming from? (TODO: probably unneccesary)
          Core destination_ : 16;      ///< What core is this message aimed at?
        };
        uint64_t raw_;
      };
      
      //uint64_t reset_count_;    ///< How many times have we been reset? (for debugging only)

      friend class RDMAAggregator;
      friend class ReuseMessageList;

      /// Mark message as sent
      virtual void mark_sent() {
        DVLOG(5) << __func__ << ": " << this << " Marking sent with is_enqueued_=" << is_enqueued_ 
                 << " is_delivered_=" << is_delivered_ 
                 << " is_sent_=" << is_sent_;
        next_ = NULL;
        prefetch_ = NULL;

        //if( Grappa::mycore() != source_ ) {
        if( (is_delivered_ == true) && (Grappa::mycore() != source_) ) {
          DVLOG(5) << __func__ << ": " << this << " Re-enqueuing to " << source_;
          DCHECK_EQ( this->is_sent_, false );
          enqueue( source_ );

        } else {
          DVLOG(5) << __func__ << ": " << this << " Final mark_sent";
          DCHECK_EQ( Grappa::mycore(), this->source_ );
          is_sent_ = true;

          //Grappa::broadcast( this );
          if( 0 != cv_.waiters_ ) {
            Grappa::broadcast( &cv_ );
          }

          if( delete_after_send_ ) {
            size_t sz = this->size();
            this->~MessageBase();
            SharedMessagePool::free(this, sz);
          }
        }
      }

      virtual const char * typestr() = 0;

      /// Active message handler to support sending messages through old aggregator
      static void legacy_send_message_am( char * buf, size_t size, void * payload, size_t payload_size );

      /// Send message through old aggregator
      void legacy_send();



      /// Interface for message serialization.
      ///  @param p Address in buffer at which to write:
      ///    -# A 2D global address of a function that knows how to
      ///       deserialize and execute the message functor/payload
      ///    -# the message functor/payload
      /// @param max_size  Largest possible serialized size
      /// @return address of the byte following the serialized message in the buffer
      inline virtual char * serialize_to( char * p, size_t max_size = -1 ) {
        DCHECK_EQ( is_sent_, false ) << "Sending same message " << this << " multiple times?";
        is_delivered_ = true;
        return p + max_size;
      }


      /// Walk a buffer of received deserializers/functors and call them.
      static inline char * deserialize_and_call( char * buffer ) {
        DVLOG(5) << "Deserializing message from " << (void*) buffer;
        typedef char * (*Deserializer)(char *);       // generic deserializer type

        // intptr_t gfp = *(reinterpret_cast< intptr_t* >( buffer ));
        // Core dest = gfp & ((1 << 16) - 1);
        // Deserializer fp = *(reinterpret_cast< Deserializer* >( gfp >> 16 ));

        // CHECK_EQ( dest, Grappa::mycore() ) << "Delivered to wrong node";

        // buffer = fp( buffer + sizeof( Deserializer ) );

        MessageFPAddr gfp = *(reinterpret_cast< MessageFPAddr* >(buffer));
        Deserializer fp = reinterpret_cast< Deserializer >( gfp.fp );

        DVLOG(5) << "Receiving message from " << gfp.dest << " with deserializer " << (void*) fp;
        
        CHECK_NOTNULL( fp );
        CHECK_EQ( gfp.dest, Grappa::mycore() ) << "Delivered to wrong core! buffer=" << (void*) buffer;

        buffer = fp( buffer + sizeof( MessageFPAddr ) );

        return buffer;
      }

    public:
      MessageBase( )
        : next_( NULL )
        , prefetch_( NULL )
        , cv_()
        , source_( -1 )
        , destination_( -1 )
        , is_enqueued_( false )
        , is_sent_( false )
        , is_delivered_( false )
        , is_moved_( false )
        // , reset_count_(0)
        , delete_after_send_( false ) 
      { 
        DVLOG(9) << "construct " << this;
      }

      MessageBase( Core dest )
        : next_( NULL )
        , prefetch_( NULL )
        , cv_()
        , is_enqueued_( false )
        , is_sent_( false )
        , is_delivered_( false )
        , is_moved_( false )
        , source_( -1 )
        , destination_( dest )
        // , reset_count_(0)
        , delete_after_send_( false ) 
      {
        CHECK( destination_ < cores() ) << "dest core out of bounds";
        DVLOG(9) << "construct " << this;
      }

      // Ensure we are sent before leaving scope
      virtual ~MessageBase() {
        CHECK_EQ( is_enqueued_, is_sent_ ) << "make sure if we were enqueued we were also sent";
      }

      MessageBase( const MessageBase& ) = delete;
      MessageBase& operator=( const MessageBase& ) = delete;
      MessageBase& operator=( MessageBase&& ) = delete;

      /// Move constructor throws an error if the message has already been enqueue to be sent.
      /// This is intended to allow use of temporary message objects for initialization as long ast 
      MessageBase( MessageBase&& m ) 
        : next_( m.next_ )
        , prefetch_( m.prefetch_ )
        , cv_( m.cv_ )
        , is_enqueued_( m.is_enqueued_ )
        , is_sent_( m.is_sent_ )
        , is_delivered_( m.is_delivered_ )
        , is_moved_( false ) // this only tells us if the current message has been moved
        , source_( m.source_ )
        , destination_( m.destination_ )
        // , reset_count_(0)
        , delete_after_send_( m.delete_after_send_ ) 
      {
        DVLOG(9) << "move " << this;
        m.is_moved_ = true; // mark message as having been moved so sending will fail
        CHECK_EQ( is_enqueued_, false ) << "Shouldn't be moving a message that has been enqueued to be sent!"
                                        << " Your compiler's return value optimization failed you here.";
      }

      inline bool waiting_to_send() {
        return is_enqueued_ && !is_sent_;
      }
      
      /// Make sure we know how big this message is
      virtual const size_t serialized_size() const = 0;
      
      /// Unserialized message size
      virtual const size_t size() const = 0;
      
      /// Implemented in MessageBaseImpl.hpp
      inline void enqueue();
      inline void enqueue( Core c );

      inline void locale_enqueue();
      inline void locale_enqueue( Core c );

      inline void send_immediate();
      inline void send_immediate( Core c );

      virtual void deliver_locally() = 0;

      inline void delete_after_send() { 
        delete_after_send_ = true;
      }


      virtual void reset() {
        // if( reset_count_ > 0 ) {
        //   DCHECK_EQ( is_enqueued_, true ) << this << " on " << Grappa::impl::global_scheduler.get_current_thread()
        //                                   << " how did we end up with is_enqueued_=" << is_enqueued_ << " and is_sent_= " << is_sent_
        //                                   << " without a reset call?";
        // }
        // reset_count_++;
        DVLOG(5) << this << " on " << Grappa::impl::global_scheduler.get_current_thread()
                 << " entering reset with is_enqueued_=" << is_enqueued_ << " and is_sent_= " << is_sent_;
        if( is_enqueued_ ) {
          block_until_sent();
        }
        DVLOG(5) << this << " on " << Grappa::impl::global_scheduler.get_current_thread()
                 << " doing reset with is_enqueued_=" << is_enqueued_ << " and is_sent_= " << is_sent_;
        next_ = NULL;
        prefetch_ = NULL;
        source_ =  -1;
        destination_ =  -1;
        is_enqueued_ = false;
        is_sent_ = false;
        is_delivered_ = false;
      }
      
      /// Block until message can be deallocated.
      void block_until_sent();

    } __attribute__((aligned(64)));
    
    /// @}
  }
}

#endif
