
#ifndef __MESSAGEBASE_HPP__
#define __MESSAGEBASE_HPP__

#include <cstring>

#include "ConditionVariableLocal.hpp"

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
    private:
      MessageBase * next_;     ///< what's the next message in the list of messages to be sent? 
      MessageBase * prefetch_; ///< what's the next message to prefetch?

      Core destination_;       ///< What core is this message aimed at?

      bool is_enqueued_;       ///< Have we been added to the send queue?
      bool is_sent_;           ///< Is our payload no longer needed?
      ConditionVariable cv_;   ///< condition variable for sleep/wake
      
      bool is_moved_;           ///< HACK: make sure we don't try to send ourselves if we're just a temporary

      friend class RDMAAggregator;


      /// Active message handler to support sending messages through old aggregator
      static void legacy_send_message_am( char * buf, size_t size, void * payload, size_t payload_size ) {
        Grappa::impl::MessageBase::deserialize_buffer( static_cast< char * >( buf ), size );
      }

      /// Send message through old aggregator
      void legacy_send() {
        const size_t message_size = this->size();
        char buf[ message_size ];
        this->serialize_to( buf, message_size );
        Grappa_call_on( destination_, legacy_send_message_am, buf, message_size );
        is_sent_ = true;
      }



      /// Interface for message serialization.
      ///  @param p Address in buffer at which to write:
      ///    -# A 2D global address of a function that knows how to
      ///       deserialize and execute the message functor/payload
      ///    -# the message functor/payload
      /// @return address of the byte following the serialized message in the buffer
      virtual char * serialize_to( char * p, size_t max_size = -1 ) = 0;

      /// Add a message to this message's list
      inline void stitch( MessageBase * m ) {
        m->next_ = next_;
        next_ = m;
      }

      /// Chase a list of messages and serialize them into a buffer.
      /// Modifies pointer to list to support size-limited-ish aggregation
      /// TODO: make this a hard limit?
      static char * serialize_to_buffer( char * buffer, Grappa::impl::MessageBase ** message_ptr, size_t max = -1 ) {
        size_t size = 0;
        Grappa::impl::MessageBase * message = *message_ptr;
        //DVLOG(5) << "Serializing messages from " << message;
        while( message ) {
          //DVLOG(5) << "Serializing message " << message;
          // add message to buffer
          char * new_buffer = message->serialize_to( buffer, max - size );
          if( new_buffer == buffer ) { // if it was too big
            break;                     // quit
          } else {
            // mark as sent
            message->is_sent_ = true;
            // go to next messsage and track total size
            Grappa::impl::MessageBase * next = message->next_;
            message->next_ = NULL;
            size += next - message;
            message = next;
          }
          buffer = new_buffer;
        }
        *message_ptr = message;
        return buffer;
      }
      
      /// Walk a buffer of received deserializers/functors and call them.
      static char * deserialize_buffer( char * buffer, size_t size ) {
        char * end = buffer + size;
        while( buffer < end ) {
          typedef char * (*Deserializer)(char *);       // generic deserializer type
          Deserializer fp = *(reinterpret_cast< Deserializer* >( buffer ));
          buffer = fp( buffer + sizeof( Deserializer ) );
        }
        return buffer; /// how far did we get before we had to stop?
      }


    public:
      MessageBase( )
        : next_( NULL )
        , is_enqueued_( false )
        , is_sent_( false )
        , destination_( -1 )
        , cv_()
	, is_moved_( false )
      { LOG(INFO) << "construct " << this; }

      MessageBase( Core dest )
        : next_( NULL )
        , is_enqueued_( false )
        , is_sent_( false )
        , destination_( dest )
        , cv_()
	, is_moved_( false )
      { LOG(INFO) << "construct " << this; }

      // Ensure we are sent before leaving scope
      inline virtual ~MessageBase() {
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
        , cv_( m.cv_ )
	, is_moved_( false ) // this only tells us if the current message has been moved
      {
	LOG(INFO) << "move " << this; 
	CHECK_EQ( is_enqueued_, false ) << "Shouldn't be moving a message that has been enqueued to be sent!"
					<< " Your compiler's return value optimization failed you here.";
	m.is_moved_ = true; // mark message as having been moved so sending will fail
      }

      /// Make sure we know how big this message is
      virtual const size_t size() const = 0;


      inline void send() {
	CHECK( !is_moved_ ) << "Shouldn't be sending a message that has been moved!"
			    << " Your compiler's return value optimization failed you here.";
        //Grappa::impl::global_rdma_aggregator.send( this );
        is_enqueued_ = true;
        legacy_send();
      }

      /// Block until message can be deallocated.
      inline void block_until_sent() {
        // block until message is sent
        while( !is_sent_ ) {
          Grappa::wait( &cv_ );
        }
      }
      
    };
    
    /// @}
  }
}

#endif
