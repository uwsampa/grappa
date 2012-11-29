
#ifndef __MESSAGEBASE_HPP__
#define __MESSAGEBASE_HPP__

#include <cstring>

//#include "ConditionVariable.hpp"

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
      MessageBase * next_; ///< what's the next message in the list of messages to be sent? 
      bool is_enqueued_;   ///< Have we been added to the send queue?
      bool is_sent_;       ///< Is our payload no longer needed?

      Core destination_;   ///< What core is this message aimed at?

      //      ConditionVariable cv_; ///< condition variable for sleep/wake

    public:
      MessageBase( Core dest )
	: next_( NULL )
	, is_enqueued_( false )
	, is_sent_( false )
	, destination_( dest )
	  //	, cv_()
      { }

      virtual ~MessageBase() {
	block_until_sent();
      }

      inline void block_until_sent() {
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
	while( message ) {
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
	}
	*message_ptr = message;
	return buffer;
      }
      
      /// Walk a buffer of received deserializers/functors and call them.
      static void deserialize_buffer( char * buffer, size_t size ) {
	char * end = buffer + size;
	while( buffer != end ) {
	  typedef char * (*Deserializer)(char *);	// generic deserializer type
	  Deserializer fp = *(reinterpret_cast< Deserializer* >( buffer ));
	  buffer = fp( buffer + sizeof( Deserializer ) );
	}
      }
    };
    
    /// @}
  }
}

#endif
