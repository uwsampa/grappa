
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "RDMAAggregator.hpp"
#include "Message.hpp"

DEFINE_int64( target_size, 1 << 12, "Target size for aggregated messages" );

/// stats for application messages
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, app_messages_enqueue, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, app_messages_enqueue_cas, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, app_messages_serialized, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, app_messages_deserialized, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, app_messages_immediate, 0 );

/// stats for aggregated messages
GRAPPA_DEFINE_STAT( SummarizingStatistic<int64_t>, rdma_message_bytes, 0 );

/// stats for RDMA Aggregator events
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_receive_start, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_receive_end, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_send_start, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_send_end, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_capacity_flushes, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_idle_flushes, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_requested_flushes, 0 );


// defined in Grappa.cpp
extern bool Grappa_done_flag;


namespace Grappa {

  /// Internal messaging functions
  namespace impl {

    /// @addtogroup Communication
    /// @{

    /// global RDMAAggregator instance
    Grappa::impl::RDMAAggregator global_rdma_aggregator;

    /// proxy call to make it easier to integrate with scheduler
    void idle_flush_rdma_aggregator() {
      global_rdma_aggregator.idle_flush();
    }

    /// Task that is constantly waiting to do idle flushes. This
    /// ensures we always have some sending resource available.
    void RDMAAggregator::idle_flush_task() {
      while( !Grappa_done_flag ) {
        Grappa::wait( &flush_cv_ );
        rdma_idle_flushes++;
        for( int i = 0; i < total_cores_; ++i ) {
          flush_one( i );
        }
      }
    }

    void RDMAAggregator::init() {
#ifdef LEGACY_SEND
#warning RDMA Aggregator is bypassed!
#endif
      cores_.resize( global_communicator.nodes() );
      mycore_ = global_communicator.mynode();
      total_cores_ = global_communicator.nodes();
      deserialize_buffer_handle_ = global_communicator.register_active_message_handler( &deserialize_buffer_am );
      deserialize_first_handle_ = global_communicator.register_active_message_handler( &deserialize_first_am );

      // spawn flusher
      Grappa::privateTask( [this] {
          idle_flush_task();
        });
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
      app_messages_deserialized++;
      Grappa::impl::MessageBase::deserialize_and_call( static_cast< char * >( buf ) );
    }


    char * RDMAAggregator::aggregate_to_buffer( char * buffer, Grappa::impl::MessageBase ** message_ptr, size_t max, size_t * count ) {
      size_t size = 0;
      Grappa::impl::MessageBase * message = *message_ptr;
      DVLOG(5) << "Serializing messages from " << message;

      while( message ) {
        DVLOG(5) << "Serializing message " << message;
        DVLOG(4) << "Serializing message " << message << ": " << message->typestr();

        // issue prefetch for next message
        __builtin_prefetch( message->prefetch_, 0, prefetch_type );

        // add message to buffer
        char * new_buffer = message->serialize_to( buffer, max - size );
        if( new_buffer == buffer ) { // if it was too big
          DVLOG(5) << "Message too big: aborting serialization";
          break;                     // quit
        } else {
          app_messages_serialized++;
          if( count != NULL ) (*count)++;

          DVLOG(5) << "Serialized message " << message << " with size " << new_buffer - buffer;

          // track total size
          size += new_buffer - buffer;

          // go to next messsage 
          Grappa::impl::MessageBase * next = message->next_;

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
        app_messages_deserialized++;
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
      rdma_receive_start++;

      // allocate buffer for received messages
      size_t max_size = FLAGS_target_size + 1024;
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
      rdma_receive_end++;
    }

      

    // void RDMAAggregator::send_rdma( Core core ) {
    //   char * buf = 0;

    //   DVLOG(5) << __func__ << ": " << "Sending messages via RDMA from " << (void*)cores_[core].messages_.raw_ << " to " << core << " using buffer " << (void*) buf;
    //   rdma_send_start++;

    //   // don't bother continuing if we have nothing to send.
    //   if( 0 == cores_[core].messages_.raw_ ) {
    //     DVLOG(5) << __func__ << ": " << "Aborting send to " << core << " since there's nothing to do";
    //     return;
    //   }

    //   // grab list of messages
    //   DVLOG(5) << __func__ << ": " << "Grabbing messages for " << core << " later buffer " << (void*) buf;
    //   Grappa::impl::MessageList old_ml, new_ml;
    //   do {
    //     old_ml = cores_[core].messages_;
    //     new_ml.raw_ = 0;
    //   } while( !__sync_bool_compare_and_swap( &(cores_[core].messages_.raw_), old_ml.raw_, new_ml.raw_ ) );
    //   // old_ml = cores_[core].messages_;
    //   // cores_[core].messages_.raw_ = new_ml.raw_ = 0;
    //   Grappa::impl::MessageBase * messages_to_send = get_pointer( &old_ml );

    //   DVLOG(5) << __func__ << ": " << "Grabbing messages for " << core << " got " << (void*) old_ml.raw_ << " later buffer " << (void*) buf;
    //   send_rdma( ml );
    // }

    void RDMAAggregator::send_rdma( Core core, Grappa::impl::MessageList ml ) {
      char * buf = nullptr;
      size_t grabbed_count = ml.count_;
      size_t serialized_count = 0;
      size_t buffers_sent = 0;
      size_t serialize_calls = 0;
      size_t serialize_null_returns = 0;

      if( 0 == ml.raw_ ) {
        DVLOG(5) << __func__ << ": " << "Aborting send to " << core << " since there's nothing to do";
        return;
      }

      rdma_send_start++;
      Grappa::impl::MessageBase * messages_to_send = get_pointer( &ml );

#ifdef DEBUG
      Grappa::impl::MessageBase * debug_ptr = get_pointer( &ml );
      int64_t debug_count = grabbed_count;
      while( debug_count > 0 ) {
        debug_ptr = debug_ptr->next_;
        debug_count--;
      }
      if( debug_ptr != NULL ) {
        Grappa::impl::MessageBase * ptr = get_pointer( &ml );
        int64_t main_count = grabbed_count;
        int64_t longer_count = grabbed_count+10;
        LOG(INFO) << "Too-long message list:";
        while( longer_count > 0 ) {
          LOG(INFO) << "message " << ptr;
          ptr = ptr->next_;
          if( main_count == 0 ) LOG(INFO) << "---supposed end---";
          main_count--;
          longer_count--;
        }
      }
      CHECK_EQ( debug_ptr, static_cast<Grappa::impl::MessageBase*>(NULL) ) << "Count didn't match message list length."
                                                                           << " debug_ptr=" << debug_ptr
                                                                           << " debug_ptr->next=" << debug_ptr->next_
                                                                           << " count=" << grabbed_count;

#endif

    DVLOG(5) << __func__ << ": " << "Sending messages via RDMA from " << (void*)cores_[core].messages_.raw_ << " to " << core << " using buffer " << (void*) buf;
      
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
        //size_t max_size = STACK_SIZE / 4;
        size_t max_size = FLAGS_target_size + 1024;
        char bufarr[ max_size ]  __attribute__ ((aligned (16)));
        buf = &bufarr[0];
        char * payload = buf + response.serialized_size();
        
        
        // issue initial prefetches
        // (these may have been overwritten by another sender, so hope for the best.)
        for( int i = 0; i < prefetch_dist; ++i ) {
          Grappa::impl::MessageBase * pre = get_pointer( &cores_[core].prefetch_queue_[i] );
          __builtin_prefetch( pre, 0, prefetch_type ); // prefetch for read
        }
        
        // serialize into buffer
        DVLOG(5) << __func__ << ": " << "Serializing messages for " << core << " into buffer " << (void*) buf;
        serialize_calls++;
        char * end = aggregate_to_buffer( payload, &messages_to_send, max_size - response.serialized_size(), &serialized_count );
        if( messages_to_send == NULL ) serialize_null_returns++;
        DVLOG(5) << __func__ << ": " << "Wrote " << end - payload << " bytes into buffer " << (void*) buf << " for " << core;
        
        // wait until we have a buffer
        DVLOG(5) << __func__ << ": " << "Maybe blocking for response from " << core << " for buffer " << (void*) buf;
        ReceiveBufferInfo dest = destbuf.readFE();
        DVLOG(5) << __func__ << ": " << "Got buffer info with buffer address " << (void*) dest.buffer << " and fullbit address " << dest.info_ptr << " from " << core << " for buffer " << (void*) buf;
        
        // Fill in response message fields
        response_functor.info_ptr = dest.info_ptr;
        response_functor.offset = response.serialized_size();
        response_functor.actual_size = end - payload;
        
        // Serialize message into head of buffer
        Grappa::impl::MessageBase * response_ptr = &response;
        char * other_end = response.serialize_to( buf, response.serialized_size() );
        response.mark_sent();
        DVLOG(5) << __func__ << ": " << "Wrote " << other_end - buf << " additional bytes into buffer " << (void*) buf << " for " << core;
        
        // send buffer to other side and run the first message in the buffer
        DVLOG(5) << __func__ << ": " << "Sending " << end - buf << " bytes of aggregated messages to " << core << " address " << (void*)dest.buffer << " from buffer " << (void*) buf;
        GASNET_CHECK( gasnet_AMRequestLong0( core, deserialize_first_handle_, 
                                             buf, end - buf, dest.buffer ) );
        
        // maybe wait for ack, with potential payload

        rdma_message_bytes += end - buf;
        buffers_sent++;
        DVLOG(5) << __func__ << ": " << "Sent buffer of size " << end - buf << " through RDMA to " << core;
      } while( messages_to_send != static_cast<Grappa::impl::MessageBase*>(nullptr) );
        // potentially loop
      
      DCHECK_EQ( grabbed_count, serialized_count ) << "Did we not serialize everything?" 
                                                   << " ml=" << ml.count_ << "/" << get_pointer( &ml )
                                                   << " buffers_sent=" << buffers_sent
                                                   << " messages_to_send=" << messages_to_send
                                                   << " serialize_calls=" << serialize_calls
                                                   << " serialize_null_returns=" << serialize_null_returns;
      rdma_send_end++;
    }




    // performance sucks
    void RDMAAggregator::send_medium( Core core, Grappa::impl::MessageList ml ) {

      if( cores_[core].messages_.raw_ == 0 )
        return;
      
      // create temporary buffer
      const size_t size = 4024;
      char buf[ size ] __attribute__ ((aligned (16)));
      
      Grappa::impl::MessageBase * messages_to_send = get_pointer( &ml );

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
