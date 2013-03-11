
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "RDMAAggregator.hpp"
#include "Message.hpp"

DEFINE_int64( target_size, 1 << 12, "Target size for aggregated messages" );

DEFINE_int64( rdma_workers_per_core, 1 << 4, "Number of RDMA deaggregation worker threads" );
DEFINE_int64( rdma_buffers_per_core, 16, "Number of RDMA aggregated message buffers per core" );

DEFINE_int64( rdma_threshold, 4024, "Threshold in bytes below which we send immediately instead of using RDMA" );


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
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_core_idle_flushes, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_requested_flushes, 0 );

GRAPPA_DEFINE_STAT( SummarizingStatistic<int64_t>, rdma_buffers_inuse, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_buffers_blocked, 0 );


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
    void RDMAAggregator::idle_flusher() {
      while( !Grappa_done_flag ) {
        Grappa::wait( &flush_cv_ );
        rdma_idle_flushes++;
        for( int i = 0; i < total_cores_; ++i ) {
          if( flush_one( i ) ) {
            rdma_core_idle_flushes++;
          }
        }
      }
    }


    void RDMAAggregator::init() {
#ifdef LEGACY_SEND
#warning RDMA Aggregator is bypassed!
#endif
#ifdef ENABLE_RDMA_AGGREGATOR
      //cores_.resize( global_communicator.nodes() );
      mycore_ = global_communicator.mynode();
      mynode_ = -1; // gasnet supernode
      total_cores_ = global_communicator.nodes();
      deserialize_buffer_handle_ = global_communicator.register_active_message_handler( &deserialize_buffer_am );
      deserialize_first_handle_ = global_communicator.register_active_message_handler( &deserialize_first_am );
      enqueue_buffer_handle_ = global_communicator.register_active_message_handler( &enqueue_buffer_am );

      // fill pool of buffers
      auto new_buffers = new RDMABuffer[ FLAGS_rdma_buffers_per_core * Grappa::cores()];
      for( int i = 0; i < FLAGS_rdma_buffers_per_core * Grappa::cores(); ++i ) {
        free_buffer_list_.push( &new_buffers[i] );
      }

      // create additional tasks for receive workers and flusher
      //global_scheduler.createWorkers( FLAGS_rdma_workers + 1 );

      // spawn receive workers
      for( int i = 0; i < FLAGS_rdma_workers_per_core; ++i ) {
        Grappa::privateTask( [this] { 
            receive_worker();
          });
      }

      // spawn flusher
      Grappa::privateTask( [this] {
          idle_flusher();
        });
#endif
    }

    void RDMAAggregator::activate() {
#ifdef ENABLE_RDMA_AGGREGATOR
      // allocate core message list structs
      if( global_communicator.locale_mycore() == 0 ) {
        cores_ = Grappa::impl::node_shared_memory.segment.construct<CoreData>("Cores")[global_communicator.cores()]();
      }
      global_communicator.barrier();
      if( global_communicator.locale_mycore() != 0 ) {
        std::pair< CoreData *, boost::interprocess::managed_shared_memory::size_type > p;
        p = Grappa::impl::node_shared_memory.segment.find<CoreData>("Cores");
        CHECK_EQ( p.second, global_communicator.cores() );
        cores_ = p.first;
      }

      // precache buffers
      LOG(INFO) << "Precaching buffers";
      size_t expected_buffers = (Grappa::cores() - 1) * remote_buffer_pool_size;
      for( int i = 0; false && i < Grappa::locales(); ++i ) {
        if( i != Grappa::mylocale() && Grappa::mycore() == global_communicator.source_core_for(i) ) {
          Core dest = global_communicator.dest_core_for(i);
          LOG(INFO) << "Core " << Grappa::mycore() << " precaching to " << dest;
          Core requestor = Grappa::mycore();
          auto request = Grappa::message( dest, [dest, requestor, &expected_buffers] {
              for( int j = 0; j < remote_buffer_pool_size; ++j ) {
                RDMABuffer * b = global_rdma_aggregator.free_buffer_list_.try_pop();
                CHECK_NOTNULL( b );
                auto reply = Grappa::message( requestor, [dest, b, &expected_buffers] {
                    global_rdma_aggregator.cores_[dest].remote_buffers_.push( b );
                    expected_buffers--;
                  });
                reply.send_immediate();
              }
            });
          request.send_immediate();
        }
      }


      for( int i = 0; i < Grappa::cores(); ++i ) {
        if( global_communicator.locale_of(i) != Grappa::mylocale() ) {
        }  
      }
      while( expected_buffers > 0 ) global_communicator.poll();
      LOG(INFO) << "Done precaching buffers";
      for( int i = 0; i < Grappa::cores(); ++i ) {
        if( i != Grappa::mycore() ) {
          CHECK_EQ( cores_[i].remote_buffers_.count(), remote_buffer_pool_size );
        }
      }
#endif
    }

    void RDMAAggregator::finish() {
#ifdef ENABLE_RDMA_AGGREGATOR
      global_communicator.barrier();
      if( global_communicator.locale_mycore() == 0 ) {
        Grappa::impl::node_shared_memory.segment.destroy<CoreData>("Cores");
      }
      cores_ = NULL;
#endif
    }

    void RDMAAggregator::deserialize_buffer_am( gasnet_token_t token, void * buf, size_t size ) {
#ifdef DEBUG
      gasnet_node_t src = -1;
      gasnet_AMGetMsgSource(token,&src);
      DVLOG(5) << __func__ << ": Receiving buffer of size " << size << " through gasnet from " << src;
#endif
      deaggregate_buffer( static_cast< char * >( buf ), size );
    }

    void RDMAAggregator::deserialize_first_am( gasnet_token_t token, void * buf, size_t size ) {
#ifdef DEBUG
      gasnet_node_t src = -1;
      gasnet_AMGetMsgSource(token,&src);
      DVLOG(5) << __func__ << ": Receiving buffer of size " << size << " from " << src << " through gasnet; deserializing first entry";
#endif
      app_messages_deserialized++;
      Grappa::impl::MessageBase::deserialize_and_call( static_cast< char * >( buf ) );
    }

    void RDMAAggregator::enqueue_buffer_am( gasnet_token_t token, void * buf, size_t size ) {
#ifdef DEBUG
      gasnet_node_t src = -1;
      gasnet_AMGetMsgSource(token,&src);
      DVLOG(5) << __func__ << ": Receiving buffer of size " << size << " from " << src << " through gasnet; enqueuing for deserialization";
#endif
      global_rdma_aggregator.received_buffer_list_.push( reinterpret_cast< RDMABuffer * >( buf ) );
    }


    char * RDMAAggregator::aggregate_to_buffer( char * buffer, Grappa::impl::MessageBase ** message_ptr, size_t max, size_t * count ) {
      size_t size = 0;
      Grappa::impl::MessageBase * message = *message_ptr;
      DVLOG(5) << "Serializing messages from " << message;

      // make space for terminator byte
      max -= 1;

      while( message ) {
        DVLOG(5) << __func__ << ": Serializing message " << message << ": " << message->typestr();

        // issue prefetch for next message
        __builtin_prefetch( message->prefetch_, 0, prefetch_type );

        // add message to buffer
        char * new_buffer = message->serialize_to( buffer, max - size);
        if( new_buffer == buffer ) { // if it was too big
          DVLOG(5) << __func__ << ": Message too big: aborting serialization";
          break;                     // quit
        } else {
          app_messages_serialized++;
          if( count != NULL ) (*count)++;

          DVLOG(5) << __func__ << ": Serialized message " << message << " with size " << new_buffer - buffer;

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

      // return rest of message list
      *message_ptr = message;

      // write terminating byte
      *buffer++ = -1;

      return buffer;
    }

    char * RDMAAggregator::deaggregate_buffer( char * buffer, size_t size ) {
      DVLOG(5) << __func__ << ": Deaggregating buffer at " << (void*) buffer << " of max size " << size;
      char * end = buffer + size;
      while( buffer < end && *buffer != -1 ) {
        app_messages_deserialized++;
        DVLOG(5) << __func__ << ": Deserializing and calling at " << (void*) buffer << " with " << end - buffer << " remaining";
        char * next = Grappa::impl::MessageBase::deserialize_and_call( buffer );
        DVLOG(5) << __func__ << ": Deserializing and called at " << (void*) buffer << " with next " << (void*) next;
        buffer = next;
      }
      DVLOG(5) << __func__ << ": Done deaggregating buffer";
      return buffer; /// how far did we get before we had to stop?
    }




    ///
    /// new approach to send/recv
    ///

    /// block until there's something to be received and deaggregate it
    void RDMAAggregator::receive_worker() {
      while( !Grappa_done_flag ) {
        // block until we have a buffer to deaggregate
        RDMABuffer * receiver_buffer = received_buffer_list_.block_until_pop();

        Core sender = receiver_buffer->get_core();
        RDMABuffer * sender_buffer = receiver_buffer->get_ack();
        Core receiver = Grappa::mycore();

        // ack with a replacement buffer
        RDMABuffer * new_receiver_buffer_for_sender = global_rdma_aggregator.free_buffer_list_.try_pop();
        if( new_receiver_buffer_for_sender == NULL ) {
          LOG(WARNING) << __func__ << "/" << global_scheduler.get_current_thread()
                       << " No buffer available for ack to sender " << sender << " whose buffer count is " 
                       << cores_[sender].remote_buffers_.count() << " on " << receiver << "; blocking";
          new_receiver_buffer_for_sender = global_rdma_aggregator.free_buffer_list_.block_until_pop();
        }
        auto m = Grappa::message( sender, [sender, receiver, sender_buffer, new_receiver_buffer_for_sender] {
            if( sender_buffer ) global_rdma_aggregator.free_buffer_list_.push( sender_buffer );
            global_rdma_aggregator.cache_buffer_for_core( receiver, new_receiver_buffer_for_sender );
          });
        m.send_immediate();


        // // record that the sender has used one of its cached buffers
        // cores_[b->get_core()].remote_buffers_held_.increment();

        // // get a buffer to return to the other side
        // RDMABuffer * sender_buffer = cores_[sender].remote_buffers_.try_pop();

        // LOG(WARNING) << __func__ << "/" << global_scheduler.get_current_thread()
        //              << "START receive: Sender " << sender << " buffer count is " << cores_[sender].remote_buffers_.count() << " on " << receiver;
        // global_scheduler.set_no_switch_region( false ); //true );
        // interesting_ = true;


        DVLOG(5) << __func__ << " Sender " << sender << " buffer count is " << cores_[sender].remote_buffers_.count() << " from " << receiver << " new buffer " << new_receiver_buffer_for_sender;

        // deaggregate and execute
        // there should be a message in here delivering a new remote buffer
        DVLOG(5) << __func__ << ": " << "Deaggregating"
                 << " from buffer " << receiver_buffer;
        // we will stop deaggregating at terminator byte
        CHECK_EQ( disable_flush_, false );
        disable_flush_ = false;
        global_scheduler.set_no_switch_region( false );

        deaggregate_buffer( receiver_buffer->get_payload(), receiver_buffer->get_max_size() );

        global_scheduler.set_no_switch_region( false );
        disable_flush_ = false;
        DVLOG(5) << __func__ << ": " << "Deaggregated"
                 << " from buffer " << receiver_buffer;

        global_rdma_aggregator.free_buffer_list_.push( receiver_buffer );

        // // if we didn't get a buffer before, try now.
        // if( sender_buffer == NULL ){
        //   sender_buffer = cores_[sender].remote_buffers_.try_pop();
        //   //CHECK_NOTNULL( sender_buffer );
        // }

        // interesting_ = false;
        // global_scheduler.set_no_switch_region( false );
        // LOG(WARNING) << __func__ << "/" << global_scheduler.get_current_thread()
        //              << "END receive: Sender " << sender << " buffer count is " << cores_[sender].remote_buffers_.count() << " on " << receiver;

        // // // return buffer to free list
        // // free_buffer_list_.push( receiver_buffer );

        // // ack buffers
        // auto m = Grappa::message( sender, [sender, receiver, sender_buffer, receiver_buffer] {
        //     if( sender_buffer ) global_rdma_aggregator.free_buffer_list_.push( sender_buffer );
        //     global_rdma_aggregator.cache_buffer_for_core( sender, receiver, receiver_buffer );
        //   });
        // m.send_immediate();
      }
    }

    /// Accept an empty buffer sent by a remote host
    void RDMAAggregator::cache_buffer_for_core( Core owner, RDMABuffer * b ) {
      // try to accept this buffer
      if( !cores_[owner].remote_buffers_.try_push( b ) ) {
        LOG(WARNING) << __func__ << "/" << global_scheduler.get_current_thread()
                     << " Core " << owner << " buffer count is " << cores_[owner].remote_buffers_.count() << "; returning " << b;
        // too many already! return it
        auto m = message( owner, [b] {
            global_rdma_aggregator.free_buffer_list_.push(b);
          });
        m.send_immediate();
      }
    }


  void RDMAAggregator::send_rdma( Core core, Grappa::impl::MessageList ml, size_t estimated_size ) {
      CHECK_LT( core, Grappa::cores() ) << "WTF?";


      size_t serialized_count = 0;
      size_t buffers_sent = 0;
      size_t serialize_calls = 0;
      size_t serialize_null_returns = 0;



      CoreData * dest = &cores_[ core ];

      size_t grabbed_count = ml.count_;

      if( 0 == ml.raw_ ) {
        DVLOG(5) << __func__ << ": " << "Aborting send to " << core << " since there's nothing to do";
        return;
      }

      if( estimated_size < FLAGS_rdma_threshold ) {
        send_medium( core, ml );
        return;
      }

      rdma_send_start++;
      Grappa::impl::MessageBase * messages_to_send = get_pointer( &ml );

      // maybe deliver locally
      if( core == Grappa::mycore() ) {
        deliver_locally( core, messages_to_send );
        return;
      }

      DVLOG(5) << __func__ << ": " << "Sending messages via RDMA from " << (void*)cores_[core].messages_.raw_ 
               << " to " << core;
      
      // send as many buffers as we need for this message list
      do {

        // // wait until there's capacity to send
        // cores_[core].remote_buffers_held_.decrement();

        //
        // Get a remote buffer
        //

        // // prepare request message just in case
        Core local_core = Grappa::mycore();
        // auto request_buffer_message = Grappa::message( core, [local_core] {
        //     global_rdma_aggregator.request_buffer_for_src( local_core );
        //   });

        // // try to get a remote buffer
        // RDMABuffer * remote_buf = dest->remote_buffers_.try_pop();

        // block until we have a remote buffer
        RDMABuffer * remote_buf = dest->remote_buffers_.block_until_pop();
        
        // // if one wasn't available, send request
        // if( NULL == remote_buf ) {
        //   request_buffer_message.send_immediate();
        // }

        //
        // get a local buffer
        //

        // get a local buffer for aggregation
        RDMABuffer * local_buf = free_buffer_list_.block_until_pop();

        // mark it as being from us
        local_buf->set_core( local_core );
        // local_buf->set_ack( NULL ); //local_buf );
        local_buf->set_ack( local_buf );

        // // we first use the local buffer to serialize our
        // // messages. after they're copied to the remote host, we give
        // // the remote host ownership of the buffer. It may then use it
        // // to reply, or simply return it as an ack. Here we build the
        // // message delivering this buffer to the remote core.
        // auto deliver_buffer_message = Grappa::message( core, [core, local_core, local_buf] {
        //     global_rdma_aggregator.cache_buffer_for_core( core, local_core, local_buf );
        //   });

        // // stitch message onto head of list
        // deliver_buffer_message.next_ = messages_to_send;
        // deliver_buffer_message.prefetch_ = messages_to_send;
        // messages_to_send = &deliver_buffer_message;
        // grabbed_count++;

        send_with_buffers( core, &messages_to_send, local_buf, remote_buf,
                           &serialized_count, &serialize_calls, &serialize_null_returns );
        buffers_sent++;
      } while( messages_to_send != static_cast<Grappa::impl::MessageBase*>(nullptr) );


      // this is a handy check except for when message lists get > 64K....
      // DCHECK_EQ( grabbed_count, serialized_count ) << "Did we not serialize everything?" 
      //                                              << " buffers_sent=" << buffers_sent
      //                                              << " messages_to_send=" << messages_to_send
      //                                              << " serialize_calls=" << serialize_calls
      //                                              << " serialize_null_returns=" << serialize_null_returns;

      rdma_send_end++;
    }

    void RDMAAggregator::deliver_locally( Core core,
                                          MessageBase * messages_to_send ) {
      CoreData * dest = &cores_[ core ];

      //
      // serialize
      //
      
      // issue initial prefetches
      // (these may have been overwritten by another sender, so hope for the best.)
      for( int i = 0; i < prefetch_dist; ++i ) {
        Grappa::impl::MessageBase * pre = get_pointer( &dest->prefetch_queue_[i] );
        __builtin_prefetch( pre, 0, prefetch_type ); // prefetch for read
      }

      while( messages_to_send != NULL ) {
        MessageBase * next = messages_to_send->next_;
        __builtin_prefetch( messages_to_send->prefetch_, 0, prefetch_type );
        messages_to_send->deliver_locally();
        messages_to_send->mark_sent();
        messages_to_send = next;
      }
    }

    void RDMAAggregator::send_with_buffers( Core core,
                                            MessageBase ** messages_to_send_ptr,
                                            RDMABuffer * local_buf,
                                            RDMABuffer * remote_buf,
                                            size_t * serialized_count_p,
                                            size_t * serialize_calls_p,
                                            size_t * serialize_null_returns_p ) {
      CoreData * dest = &cores_[ core ];

      //
      // serialize
      //
      
      // issue initial prefetches
      // (these may have been overwritten by another sender, so hope for the best.)
      for( int i = 0; i < prefetch_dist; ++i ) {
        Grappa::impl::MessageBase * pre = get_pointer( &dest->prefetch_queue_[i] );
        __builtin_prefetch( pre, 0, prefetch_type ); // prefetch for read
      }
      
      // serialize into buffer
      DVLOG(5) << __func__ << ": " << "Serializing messages for " << core 
               << " into buffer " << local_buf;
      (*serialize_calls_p)++;
      // deliver_buffer_message is now the head of the message list
      char * end = aggregate_to_buffer( local_buf->get_payload(), messages_to_send_ptr, 
                                        local_buf->get_max_size(), serialized_count_p );
      if( *messages_to_send_ptr == NULL ) (*serialize_null_returns_p)++;
      DVLOG(5) << __func__ << ": " << "Wrote " << end - local_buf->get_payload() << " bytes"
               << " into buffer " << local_buf << " for " << core;
      
      //
      // send
      //
      
      // // if we still need to, block until we have a buffer
      // if( remote_buf == NULL ) {
      //   DVLOG(5) << __func__ << ": " << "Maybe blocking for response from " << core << " for buffer " << local_buf;
      //   remote_buf = dest->remote_buffers_.block_until_pop();
      // }
      // DVLOG(5) << __func__ << ": " << "Got buffer info with buffer address " << remote_buf
      //          << " from " << core << " for buffer " << local_buf;
      
      // send buffer to other side. We copy the whole buffer so the
      // right pointer ends up in the active message.
      DVLOG(5) << __func__ << ": " << "Sending " << end - local_buf->get_payload() 
               << " bytes of aggregated messages to " << core 
               << " address " << remote_buf << " from buffer " << local_buf;
      // global_communicator.send( core, enqueue_buffer_handle_, 
      //                           local_buf->get_base(), end - local_buf->get_base(), 
      //                           remote_buf->get_base() );
      global_communicator.send_async( core, enqueue_buffer_handle_, 
                                      local_buf->get_base(), end - local_buf->get_base(), 
                                      reinterpret_cast< char * >( remote_buf ) );

      rdma_message_bytes += end - local_buf->get_payload();
      DVLOG(5) << __func__ << ": " << "Sent buffer of size " << end - local_buf->get_payload()
               << " through RDMA to " << core;

    }












    ///
    /// initial approach to communication
    ///

    void RDMAAggregator::deaggregation_task( GlobalAddress< FullEmpty < ReceiveBufferInfo > > callback_ptr ) {
      DVLOG(5) << __func__ << ": " << "Deaggregation task running to receive from " << callback_ptr.node();
      rdma_receive_start++;

      // allocate buffer for received messages
      size_t max_size = FLAGS_target_size + 1024;
      char buf[ max_size ]  __attribute__ ((aligned (16)));
      char * buffer_ptr = &buf[0];

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

    void RDMAAggregator::send_rdma_old( Core core, Grappa::impl::MessageList ml ) {
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
      
      // send as many buffers as we need for this message list
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
          DCHECK_LE( request.serialized_size(), global_communicator.inline_limit() ) 
            << "Ideally, message would be small enough to be immediately copied to the HCA.";
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
        char * end = aggregate_to_buffer( payload, &messages_to_send, max_size - response.serialized_size() - 1, &serialized_count );
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
        global_communicator.send( core, deserialize_first_handle_, 
                                  buf, end - buf, dest.buffer );
        // global_communicator.send_async( core, deserialize_first_handle_, 
        //                                 buf, end - buf, dest.buffer );
        
        // TODO: maybe wait for ack, with potential payload

        rdma_message_bytes += end - buf;
        buffers_sent++;
        DVLOG(5) << __func__ << ": " << "Sent buffer of size " << end - buf << " through RDMA to " << core;
      } while( messages_to_send != static_cast<Grappa::impl::MessageBase*>(nullptr) );
      
      DCHECK_EQ( grabbed_count, serialized_count ) << "Did we not serialize everything?" 
                                                   << " ml=" << ml.count_ << "/" << get_pointer( &ml )
                                                   << " buffers_sent=" << buffers_sent
                                                   << " messages_to_send=" << messages_to_send
                                                   << " serialize_calls=" << serialize_calls
                                                   << " serialize_null_returns=" << serialize_null_returns;
      rdma_send_end++;
    }













    ///
    /// old approach to communication
    ///

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
        global_communicator.send( core, deserialize_buffer_handle_, &buf[0], aggregated_size );
      }
    }

    /// @}
  };
};
