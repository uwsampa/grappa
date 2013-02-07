
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

    void idle_flush_rdma_aggregator() {
      global_rdma_aggregator.idle_flush();
    }

    void RDMAAggregator::deserialize_buffer_am( gasnet_token_t token, void * buf, size_t size ) {
#ifdef DEBUG
      gasnet_node_t src = -1;
      gasnet_AMGetMsgSource(token,&src);
      DVLOG(5) << "Receiving buffer of size " << size << " through gasnet from " << src;
#endif
      deaggregate_buffer( static_cast< char * >( buf ), size );
    }

    void RDMAAggregator::deserialize_first_am( gasnet_token_t token, void * buf, size_t size ) {
#ifdef DEBUG
      gasnet_node_t src = -1;
      gasnet_AMGetMsgSource(token,&src);
      DVLOG(5) << "Receiving buffer of size " << size << " from " << src << " through gasnet; deserializing first entry";
#endif
      Grappa::impl::MessageBase::deserialize_and_call( static_cast< char * >( buf ) );
    }


    char * RDMAAggregator::aggregate_to_buffer( char * buffer, Grappa::impl::MessageBase ** message_ptr, size_t max ) {
      size_t size = 0;
      Grappa::impl::MessageBase * message = *message_ptr;
      DVLOG(5) << "Serializing messages from " << message;

      while( message ) {
        DVLOG(5) << "Serializing message " << message;
        messages_serialized_++;

        // issue prefetch for next message
        __builtin_prefetch( message->prefetch_, 0, prefetch_type );

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
        global_rdma_aggregator.messages_deserialized_++;
        DVLOG(5) << "Deserializing and calling at " << (void*) buffer;
        char * next = Grappa::impl::MessageBase::deserialize_and_call( buffer );
        DVLOG(5) << "Deserializing and called at " << (void*) buffer << " with next " << (void*) next;
        buffer = next;
      }
      DVLOG(5) << "Done deaggregating buffer";
      return buffer; /// how far did we get before we had to stop?
    }




    void RDMAAggregator::deaggregation_task( GlobalAddress< FullEmpty < ReceiveBufferInfo > > callback_ptr ) {
      DVLOG(5) << __func__ << ": " << "Deaggregation task running to receive from " << callback_ptr.node();
      global_rdma_aggregator.receive_start_++;
      global_rdma_aggregator.current_src_ = callback_ptr.node();

      // allocate buffer for received messages
      size_t max_size = STACK_SIZE / 2;
      char buf[ max_size ]  __attribute__ ((aligned (16)));
      char * buffer_ptr = &buf[0];
      buf[0] = 'x';

      // allocate response full bit
      FullEmpty< SendBufferInfo > info_fe;
      FullEmpty< SendBufferInfo > * info_ptr = &info_fe;

      // tell sending node about it
      DVLOG(5) << __func__ << ": " << "Sending info for buffer " << (void*) buf << " to " << callback_ptr.node();
      auto m = Grappa::message( callback_ptr.node(), [callback_ptr, buffer_ptr, info_ptr] {
          DCHECK_EQ( callback_ptr.pointer()->full(), false ) << "Whoops! Buffer FE in use already!";
          callback_ptr.pointer()->writeXF( { buffer_ptr, info_ptr } );
        } );
      m.send_immediate(); // must bypass aggregator since we're implementing it

      // now the sending node RDMA-writes our buffer and signals our CV.
      DVLOG(5) << __func__ << ": " << "Maybe waiting on response from " << callback_ptr.node() << " for buffer " << (void*) buf;
      SendBufferInfo info = info_fe.readFE();

      // deaggregate and execute
      DVLOG(5) << __func__ << ": " << "Deaggregating " << info.actual_size << " bytes from buffer " << (void*)(buf) << " offset " << (int) info.offset << " from node " << callback_ptr.node();
      deaggregate_buffer( buf + info.offset, info.actual_size );
      DVLOG(5) << __func__ << ": " << "Deaggregated " << info.actual_size << " bytes from buffer " << (void*)(buf) << " offset " << (int) info.offset << " from node " << callback_ptr.node();

      // done
      global_rdma_aggregator.current_src_ = -1;
      global_rdma_aggregator.receive_end_++;
    }

      

    void RDMAAggregator::send_rdma( Core core ) {
      char * buf = 0;

      DVLOG(5) << __func__ << ": " << "Sending messages via RDMA from " << (void*)cores_[core].messages_.raw_ << " to " << core << " using buffer " << (void*) buf;
      send_start_++;
      current_dest_ = core;

      // don't bother continuing if we have nothing to send.
      if( 0 == cores_[core].messages_.raw_ ) {
        DVLOG(5) << __func__ << ": " << "Aborting send to " << core << " since there's nothing to do";
        return;
      }

      // grab list of messages
      DVLOG(5) << __func__ << ": " << "Grabbing messages for " << core << " later buffer " << (void*) buf;
      Grappa::impl::MessageList old_ml, new_ml;
      do {
        old_ml = cores_[core].messages_;
        new_ml.raw_ = 0;
      } while( !__sync_bool_compare_and_swap( &(cores_[core].messages_.raw_), old_ml.raw_, new_ml.raw_ ) );
      // old_ml = cores_[core].messages_;
      // cores_[core].messages_.raw_ = new_ml.raw_ = 0;
      Grappa::impl::MessageBase * messages_to_send = get_pointer( &old_ml );

      DVLOG(5) << __func__ << ": " << "Grabbing messages for " << core << " got " << (void*) old_ml.raw_ << " later buffer " << (void*) buf;

      do {
        FullEmpty< ReceiveBufferInfo > destbuf;
        GlobalAddress< FullEmpty< ReceiveBufferInfo > > global_destbuf = make_global( &destbuf );
        
        // do we have a buffer already?
        if( !destbuf.full() ) {
          // no, so go request one
          DVLOG(5) << __func__ << ": " << "Requesting buffer from " << core << " later buffer " << (void*) buf;
          auto request = Grappa::message( core, [this, global_destbuf] {
              DVLOG(5) << __func__ << ": " << "Spawning deaggregation task for " << global_destbuf.node() << " associated with buffer " << (void*) global_destbuf.pointer();
              Grappa::privateTask( [this, global_destbuf] {
                  RDMAAggregator::deaggregation_task( global_destbuf );
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
        char bufarr[ max_size ]  __attribute__ ((aligned (16)));
        buf = &bufarr[0];
        char * payload = buf + response.size();
        
        
        // issue initial prefetches
        for( int i = 0; i < prefetch_dist; ++i ) {
          Grappa::impl::MessageBase * pre = get_pointer( &cores_[core].prefetch_queue_[i] );
          __builtin_prefetch( pre, 0, prefetch_type ); // prefetch for read
          cores_[core].prefetch_queue_[i].raw_ = 0;    // clear out for next time around
        }
        
        // serialize into buffer
        // TODO: support larger than buffer
        DVLOG(5) << __func__ << ": " << "Serializing messages for " << core << " into buffer " << (void*) buf;
        char * end = aggregate_to_buffer( payload, &messages_to_send, max_size - response.size() );
        DVLOG(5) << __func__ << ": " << "Wrote " << end - payload << " bytes into buffer " << (void*) buf << " for " << core;
        
        // wait until we have a buffer
        DVLOG(5) << __func__ << ": " << "Maybe blocking for response from " << core << " for buffer " << (void*) buf;
        ReceiveBufferInfo dest = destbuf.readFE();
        DVLOG(5) << __func__ << ": " << "Got buffer info with buffer address " << (void*) dest.buffer << " and fullbit address " << dest.info_ptr << " from " << core << " for buffer " << (void*) buf;
        
        // Fill in response message fields
        response_functor.info_ptr = dest.info_ptr;
        response_functor.offset = response.size();
        response_functor.actual_size = end - payload;
        
        // Serialize message into head of buffer
        Grappa::impl::MessageBase * response_ptr = &response;
        char * other_end = response.serialize_to( buf, response.size() );
        response.mark_sent();
        DVLOG(5) << __func__ << ": " << "Wrote " << other_end - buf << " additional bytes into buffer " << (void*) buf << " for " << core;
        
        // send buffer to other side and run the first message in the buffer
        DVLOG(5) << __func__ << ": " << "Sending " << end - buf << " bytes of aggregated messages to " << core << " address " << (void*)dest.buffer << " from buffer " << (void*) buf;
        GASNET_CHECK( gasnet_AMRequestLong0( core, deserialize_first_handle_, 
                                             buf, end - buf, dest.buffer ) );
        
        // maybe wait for ack, with potential payload

        DVLOG(5) << __func__ << ": " << "Sent buffer of size " << end - buf << " through RDMA to " << core;
      } while( messages_to_send != static_cast<Grappa::impl::MessageBase*>(nullptr) );
        // potentially loop
        
      current_dest_ = -1;
      send_end_++;
    }




    // performance sucks
    void RDMAAggregator::send_medium( Core core ) {
      DVLOG(5) << "Sending messages from " << cores_[core].messages_.pointer_;

      if( cores_[core].messages_.pointer_ == 0 )
        return;
      
      // create temporary buffer
      const size_t size = 4024;
      char buf[ size ] __attribute__ ((aligned (16)));
      
      // grab list of messages
      DVLOG(5) << "Grabbing messages for " << core;
      Grappa::impl::MessageList old_ml, new_ml;
      do {
        old_ml = cores_[core].messages_;
        new_ml.raw_ = 0;
      } while( !__sync_bool_compare_and_swap( &(cores_[core].messages_.raw_), old_ml.raw_, new_ml.raw_ ) );
      Grappa::impl::MessageBase * messages_to_send = get_pointer( &old_ml );

      // issue initial prefetches
      for( int i = 0; i < prefetch_dist; ++i ) {
        Grappa::impl::MessageBase * pre = get_pointer( &cores_[core].prefetch_queue_[i] );
        __builtin_prefetch( pre, 0, prefetch_type ); // prefetch for read
        cores_[core].prefetch_queue_[i].raw_ = 0;    // clear out for next time around
      }
      

      // serialize to buffer
      while( messages_to_send != nullptr ) {
        DVLOG(5) << "Serializing message from " << messages_to_send;
        char * end = aggregate_to_buffer( &buf[0], &messages_to_send, size );
        DVLOG(5) << "After serializing, pointer was " << messages_to_send;
        size_t aggregated_size = end - &buf[0];
        
        // send
        GASNET_CHECK( gasnet_AMRequestMedium0( core, deserialize_buffer_handle_, &buf[0], aggregated_size ) );
      }
    }



    /// @}
  };
};
