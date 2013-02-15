
#ifndef __MESSAGEBASE_HPP__
#define __MESSAGEBASE_HPP__

#include <cstring>
#include "common.hpp"
#include "ConditionVariableLocal.hpp"
#include "Mutex.hpp"

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

      Core destination_;       ///< What core is this message aimed at?

      bool is_enqueued_;       ///< Have we been added to the send queue?
      bool is_sent_;           ///< Is our payload no longer needed?

      Mutex mutex_;
      ConditionVariable cv_;   ///< condition variable for sleep/wake
      
      bool is_moved_;           ///< HACK: make sure we don't try to send ourselves if we're just a temporary

      Worker * last_woken_;
      uint64_t reset_count_;

      inline bool interesting() {
        return false; (NULL != last_woken_) && (last_woken_ != global_scheduler.get_current_thread()); 
      }

      friend class RDMAAggregator;

      /// Mark message as sent
      inline void mark_sent() {
        //Grappa::lock( &mutex_ );
        if( interesting() ) {
          LOG(INFO) << this << " on " << global_scheduler.get_current_thread()
                    << " marked sent";
        }
        is_sent_ = true;
        next_ = NULL;
        prefetch_ = NULL;
        ConditionVariable old = cv_;
        if( 0 != cv_.waiters_ ) {
          Grappa::broadcast( &cv_ );
          last_woken_ = get_waiters( &old );
          // LOG(INFO) << this << " on " << global_scheduler.get_current_thread()
          //           << " mark_sent woke " << (void*) old.waiters_ 
          //           << " with is_enqueued_=" << is_enqueued_ << " and is_sent_=" << is_sent_
          //           << " remaining " << (void*) cv_.waiters_;
        }
        //Grappa::broadcast( &cv_ );
        //Grappa::unlock( &mutex_ );
        if( delete_after_send_ ) delete this;
      }

      virtual const char * typestr() = 0;

      /// Active message handler to support sending messages through old aggregator
      static void legacy_send_message_am( char * buf, size_t size, void * payload, size_t payload_size );

      /// Send message through old aggregator
      void legacy_send() {
        const size_t message_size = this->serialized_size();
        char buf[ message_size ];
        this->serialize_to( buf, message_size );
        Grappa_call_on( destination_, legacy_send_message_am, buf, message_size );
        mark_sent();
      }



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
        , destination_( -1 )
        , mutex_()
        , cv_()
        , is_moved_( false )
        , last_woken_( NULL )
        , reset_count_(0)
        , delete_after_send_( false ) 
      { DVLOG(9) << "construct " << this; }

      MessageBase( Core dest )
        : next_( NULL )
        , is_enqueued_( false )
        , is_sent_( false )
        , destination_( dest )
        , mutex_()
        , cv_()
        , is_moved_( false )
        , last_woken_( NULL )
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
        , destination_( m.destination_ )
        , mutex_( m.mutex_ )
        , cv_( m.cv_ )
        , is_moved_( false ) // this only tells us if the current message has been moved
        , last_woken_( m.last_woken_ )
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

      inline void delete_after_send() { 
        delete_after_send_ = true;
      }


      virtual void reset() {
        reset_count_++;
        //Grappa::lock( &mutex_ );
        if( interesting() ) {
          LOG(INFO) << this << " on " << global_scheduler.get_current_thread()
                    << " reset with is_enqueued_=" << is_enqueued_ << " and is_sent_= " << is_sent_
                    << " remaining " << (void*) cv_.waiters_;
        }
        if( is_enqueued_ ) {
          block_until_sent();
        }
        next_ = NULL;
        prefetch_ = NULL;
        destination_ =  -1;
        is_enqueued_ = false;
        // if( 0 != cv_.waiters_ ) {
        //   LOG(INFO) << this << " on " << global_scheduler.get_current_thread()
        //             << " reset with is_enqueued_=" << is_enqueued_ << " and is_sent_= " << is_sent_ << "->0"
        //             << " remaining " << (void*) cv_.waiters_;
        // }
        is_sent_ = false;
        //Grappa::unlock( &mutex_ );
      }
      
      /// Block until message can be deallocated.
      void block_until_sent();

      inline void check_ready() { CHECK_EQ( is_enqueued_, false ); CHECK_EQ( is_sent_, false ); }

    };
    
    /// @}
  }
}

#endif
