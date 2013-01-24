
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "RDMAAggregator.hpp"
#include "Message.hpp"

namespace Grappa {

  /// Internal messaging functions
  namespace impl {

    /// @addtogroup Communication
    /// @{


    /// global RDMAAggregator instance
    Grappa::impl::RDMAAggregator global_rdma_aggregator;


    void RDMAAggregator::deserialize_buffer_am( gasnet_token_t token, void * buf, size_t size ) {
      DVLOG(5) << "Receiving buffer of size " << size << " through gasnet";
      deaggregate_buffer( static_cast< char * >( buf ), size );
    }

    void RDMAAggregator::deserialize_first_am( gasnet_token_t token, void * buf, size_t size ) {
      DVLOG(5) << "Receiving buffer of size " << size << " through gasnet; deserializing first entry";
      Grappa::impl::MessageBase::deserialize_and_call( static_cast< char * >( buf ) );
    }


    char * RDMAAggregator::aggregate_to_buffer( char * buffer, Grappa::impl::MessageBase ** message_ptr, size_t max ) {
      size_t size = 0;
      Grappa::impl::MessageBase * message = *message_ptr;
      DVLOG(5) << "Serializing messages from " << message;
      while( message ) {
        DVLOG(5) << "Serializing message " << message;
        
        // issue prefetch for next message
        __builtin_prefetch( message->prefetch_, 1, 3 );

        // add message to buffer
        char * new_buffer = message->serialize_to( buffer, max - size );
        if( new_buffer == buffer ) { // if it was too big
          DVLOG(5) << "Message too big: aborting serialization";
          break;                     // quit
        } else {
          DVLOG(5) << "Serialized message " << message << " with size " << new_buffer - buffer;

          // track total size
          size += new_buffer - buffer;

          // go to next messsage 
          Grappa::impl::MessageBase * next = message->next_;
          message->next_ = NULL;

          // mark as sent
          message->mark_sent();

          message = next;
        }
        buffer = new_buffer;
      }
      *message_ptr = message;
      return buffer;
    }

    char * RDMAAggregator::deaggregate_buffer( char * buffer, size_t size ) {
      DVLOG(5) << "Deaggregating buffer at " << (void*) buffer << " of size " << size;
      char * end = buffer + size;
      while( buffer < end ) {
        DVLOG(5) << "Deserializing and calling at " << (void*) buffer;
        char * next = Grappa::impl::MessageBase::deserialize_and_call( buffer );
        DVLOG(5) << "Deserializing and called at " << (void*) buffer << " with next " << (void*) next;
        buffer = next;
      }
      DVLOG(5) << "Done deaggregating buffer";
      return buffer; /// how far did we get before we had to stop?
    }




    void RDMAAggregator::deaggregation_task( GlobalAddress< FullEmpty < ReceiveBufferInfo > > callback_ptr ) {
      DVLOG(5) << "Deaggregation task running to receive from " << callback_ptr.node();
      // allocate buffer for received messages
      size_t max_size = STACK_SIZE / 2;
      char buf[ max_size ]  __attribute__ ((aligned (16)));
      char * buffer_ptr = &buf[0];
      buf[0] = 'x';

      // allocate response full bit
      FullEmpty< SendBufferInfo > info_fe;
      FullEmpty< SendBufferInfo > * info_ptr = &info_fe;

      // tell sending node about it
      DVLOG(5) << "Sending buffer into to " << callback_ptr.node();
      auto m = Grappa::message( callback_ptr.node(), [callback_ptr, buffer_ptr, info_ptr] {
          callback_ptr.pointer()->writeXF( { buffer_ptr, info_ptr } );
        } );
      m.send_immediate(); // must bypass aggregator since we're implementing it

      // now the sending node RDMA-writes our buffer and signals our CV.
      DVLOG(5) << "Maybe waiting on response";
      SendBufferInfo info = info_fe.readFE();

      // deaggregate and execute
      // TODO: skip first message
      DVLOG(5) << "Deaggregating buffer of size " << info.actual_size;
      deaggregate_buffer( buf + info.offset, info.actual_size );

      // done
    }

      


    void RDMAAggregator::send_rdma( Core core ) {
      DVLOG(5) << "Sending messages via RDMA from " << cores_[core].messages_.pointer_;

      FullEmpty< ReceiveBufferInfo > & destbuf = cores_[core].remote_buffer_info_;
      GlobalAddress< FullEmpty< ReceiveBufferInfo > > global_destbuf = make_global( &destbuf );

      // do we have a buffer already?
      if( !destbuf.full() ) {
        // no, so go request one
        // BUG: this pointer is not required here.
        DVLOG(5) << "Requesting buffer from " << core;
        auto request = Grappa::message( core, [&, global_destbuf] {
            DVLOG(5) << "Spawning deaggregation task";
            Grappa::privateTask( [&, global_destbuf] {
                RDMAAggregator::deaggregation_task( global_destbuf );
                //DVLOG(5) << global_destbuf;
              } );
          } );
        request.send_immediate();  // must bypass aggregator since we're implementing it
      }

      // prepare message to wake remote thread
      struct Response {
        FullEmpty< SendBufferInfo > * info_ptr;
        int8_t offset;
        int32_t actual_size; 
        void operator()() {
          info_ptr->writeXF( { offset, actual_size } );
        };
      } response_functor;
      Grappa::ExternalMessage< Response > response( core, &response_functor );

      // allocate local buffer
      size_t max_size = STACK_SIZE / 2;
      char buf[ max_size ]  __attribute__ ((aligned (16)));
      char * payload = buf + response.size();

      // grab list of messages
      DVLOG(5) << "Grabbing messages for " << core;
      Grappa::impl::MessageList old_ml, new_ml;
      do {
        old_ml = cores_[core].messages_;
        new_ml.raw_ = 0;
      } while( !__sync_bool_compare_and_swap( &(cores_[core].messages_.raw_), old_ml.raw_, new_ml.raw_ ) );
      Grappa::impl::MessageBase * messages_to_send = get_pointer( &old_ml );


      // serialize into buffer
      // TODO: support larger than buffer
      DVLOG(5) << "Serializing messages for " << core;
      char * end = aggregate_to_buffer( payload, &messages_to_send, max_size - response.size() );
      DVLOG(5) << "Wrote " << end - payload << " bytes into buffer";

      // wait until we have a buffer
      DVLOG(5) << "Maybe blocking for response from " << core;
      ReceiveBufferInfo dest = destbuf.readFE();
      DVLOG(5) << "Got buffer info with buffer address " << (void*) dest.buffer << " and fullbit address " << dest.info_ptr;

      // Fill in response message fields
      response_functor.info_ptr = dest.info_ptr;
      response_functor.offset = response.size();
      response_functor.actual_size = end - payload;

      // Serialize message into head of buffer
      Grappa::impl::MessageBase * response_ptr = &response;
      char * other_end = response.serialize_to( buf, response.size() );
      response.mark_sent();
      DVLOG(5) << "Wrote " << other_end - buf << " additional bytes into buffer";

      // send buffer to other side and run the first message in the buffer
      DVLOG(5) << "Sending aggregated messages to " << core;
      GASNET_CHECK( gasnet_AMRequestLong0( core, deserialize_first_handle_, 
                                           buf, end - buf, dest.buffer ) );

      // maybe wait for ack, with potential payload

      // potentially loop

      DVLOG(5) << "Sent buffer of size " << end - buf << " through RDMA";
    }

    // /// Construct RDMA Aggregator
    // RDMAAggregator::RDMAAggregator( ) 
    //   :
    // { }

    /// @}

  };
};
