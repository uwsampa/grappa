
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

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
  
  /// Internal messaging functions
  namespace impl {

    /// @addtogroup Communication
    /// @{

    /// Base class for messages. Includes support for the three main features of a message:
    ///  - pointers and flags to support storing messages in a linked list
    ///  - interface for message serialization
    ///  - storage for blocking worker until message is sent
    /// This is a virtual class.
    class MessageBase {
    protected:
      MessageBase * next_;     ///< what's the next message in the list of messages to be sent? 
      MessageBase * prefetch_; ///< what's the next message to prefetch?

      Core source_;            ///< What core is this message coming from? (TODO: probably unneccesary)
      Core destination_;       ///< What core is this message aimed at?

      bool is_enqueued_;       ///< Have we been added to the send queue?
      bool is_sent_;           ///< Is our payload no longer needed?
      bool is_delivered_;      ///< Are we waiting to mark the message sent?
      
      ConditionVariable cv_;   ///< condition variable for sleep/wake
      
      bool is_moved_;           ///< HACK: make sure we don't try to send ourselves if we're just a temporary

      uint64_t reset_count_;    ///< How many times have we been reset? (for debugging only)

      friend class RDMAAggregator;

      /// Mark message as sent
      virtual void mark_sent() {
        DVLOG(5) << __func__ << ": " << this << " Marking sent with is_enqueued_=" << is_enqueued_ 
                 << " is_delivered_=" << is_delivered_ 
                 << " is_sent_=" << is_sent_;
        next_ = NULL;
        prefetch_ = NULL;

        if( (is_delivered_ == true) && (Grappa::mycore() != source_) ) {
          DVLOG(5) << __func__ << ": " << this << " Re-enqueuing to " << source_;
          DCHECK_EQ( this->is_sent_, false );
          enqueue( source_ );

        } else {
          DVLOG(5) << __func__ << ": " << this << " Final mark_sent";
          DCHECK_EQ( Grappa::mycore(), this->source_ );
          is_sent_ = true;
          ConditionVariable old = cv_;
          if( 0 != cv_.waiters_ ) {
            Grappa::broadcast( &cv_ );
          }
          if( delete_after_send_ ) {
            this->~MessageBase();
            Grappa::impl::locale_shared_memory.segment.deallocate( this );
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
      /// @return address of the byte following the serialized message in the buffer
      inline virtual char * serialize_to( char * p, size_t max_size = -1 ) {
        DCHECK_EQ( is_sent_, false ) << "Sending same message " << this << " multiple times?";
      }


      /// Walk a buffer of received deserializers/functors and call them.
      static inline char * deserialize_and_call( char * buffer ) {
        DVLOG(5) << "Deserializing message from " << (void*) buffer;
        typedef char * (*Deserializer)(char *);       // generic deserializer type
        Deserializer fp = *(reinterpret_cast< Deserializer* >( buffer ));
        buffer = fp( buffer + sizeof( Deserializer ) );
        return buffer;
      }

    protected:
      bool delete_after_send_;  ///< Is this a heap message? Should it be deleted after it's sent?

    public:
      MessageBase( )
        : next_( NULL )
        , is_enqueued_( false )
        , is_sent_( false )
        , is_delivered_( false )
        , source_( -1 )
        , destination_( -1 )
        , cv_()
        , is_moved_( false )
        , reset_count_(0)
        , delete_after_send_( false ) 
      { DVLOG(9) << "construct " << this; }

      MessageBase( Core dest )
        : next_( NULL )
        , is_enqueued_( false )
        , is_sent_( false )
        , is_delivered_( false )
        , source_( -1 )
        , destination_( dest )
        , cv_()
        , is_moved_( false )
        , reset_count_(0)
        , delete_after_send_( false ) 
      { DVLOG(9) << "construct " << this; }

      // Ensure we are sent before leaving scope
      virtual ~MessageBase() {
        DVLOG(9) << "destruct " << this;
        if (is_enqueued_) {
          block_until_sent();
        }
      }

      MessageBase( const MessageBase& ) = delete;
      MessageBase& operator=( const MessageBase& ) = delete;
      MessageBase& operator=( MessageBase&& ) = delete;

      /// Move constructor throws an error if the message has already been enqueue to be sent.
      /// This is intended to allow use of temporary message objects for initialization as long ast 
      MessageBase( MessageBase&& m ) 
        : next_( m.next_ )
        , is_enqueued_( m.is_enqueued_ )
        , is_sent_( m.is_sent_ )
        , is_delivered_( m.is_delivered_ )
        , source_( m.source_ )
        , destination_( m.destination_ )
        , cv_( m.cv_ )
        , is_moved_( false ) // this only tells us if the current message has been moved
        , reset_count_(0)
        , delete_after_send_( m.delete_after_send_ ) 
      {
        DVLOG(9) << "move " << this;
        CHECK_EQ( is_enqueued_, false ) << "Shouldn't be moving a message that has been enqueued to be sent!"
                                        << " Your compiler's return value optimization failed you here.";
        m.is_moved_ = true; // mark message as having been moved so sending will fail
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
      inline void send_immediate();
      inline void send_immediate( Core c );

      virtual void deliver_locally() = 0;

      inline void delete_after_send() { 
        delete_after_send_ = true;
      }


      virtual void reset() {
        if( reset_count_ > 0 ) {
          DCHECK_EQ( is_enqueued_, true ) << this << " on " << global_scheduler.get_current_thread()
                                          << " how did we end up with is_enqueued_=" << is_enqueued_ << " and is_sent_= " << is_sent_
                                          << " without a reset call?";
        }
        reset_count_++;
        DVLOG(5) << this << " on " << global_scheduler.get_current_thread()
                 << " entering reset with is_enqueued_=" << is_enqueued_ << " and is_sent_= " << is_sent_;
        if( is_enqueued_ ) {
          block_until_sent();
        }
        DVLOG(5) << this << " on " << global_scheduler.get_current_thread()
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

    };
    
    /// @}
  }
}

#endif
