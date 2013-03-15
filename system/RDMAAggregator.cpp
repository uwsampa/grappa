
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <fstream>
#include <algorithm>

#include "RDMAAggregator.hpp"
#include "Message.hpp"

DEFINE_int64( target_size, 1 << 12, "Target size for aggregated messages" );

DEFINE_int64( rdma_workers_per_core, 1 << 4, "Number of RDMA deaggregation worker threads" );
DEFINE_int64( rdma_buffers_per_core, 16, "Number of RDMA aggregated message buffers per core" );

DEFINE_int64( rdma_threshold, 64, "Threshold in bytes below which we send immediately instead of using RDMA" );

DEFINE_string( route_graph_filename, "routing.dot", "Name of file for routing graph" );


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

GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_poll, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_poll_send, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_poll_receive, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_poll_send_success, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_poll_receive_success, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_poll_yields, 0 );

GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_flush_send, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, rdma_flush_receive, 0 );


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

        Core c = Grappa::mycore();

        // see if we have anything to receive
        if( cores_[ c ].messages_.raw_ != 0 ) {
          rdma_flush_receive++;
          Grappa::impl::MessageList ml = grab_messages( c );
          deliver_locally( c, get_pointer( &ml ) );
        }

        // see if we have anything at all to send
        for( int i = 0; i < core_partner_locale_count_; ++i ) {
          Locale locale = core_partner_locales_[i];
          if( check_for_any_work_on( locale ) ) {
            rdma_flush_send++;
            send_locale_medium( locale );
          }
        }
      }
    }

  /// allocate and initialize locale-to-core translation
  void RDMAAggregator::compute_route_map() {

    Core locale_first_core = Grappa::mylocale() * Grappa::locale_cores();
    Core locale_last_core = Grappa::mylocale() * Grappa::locale_cores() + Grappa::locale_cores();
    
    // how many locales should seach core handle?
    // (assume all locales have same core count)
    int probable_locales_per_core = Grappa::locales() / Grappa::locale_cores();
    // (make we have at least one locale per core)
    int locales_per_core = probable_locales_per_core > 0 ? probable_locales_per_core : 1;
    LOG(INFO) << "probable_locales_per_core " << probable_locales_per_core
              << " locales_per_core " << locales_per_core;




    // initialize source cores
    //source_core_for_locale_ = new Core[ Grappa::locales() ];
    for( int i = 0; i < Grappa::locales(); ++i ) {
      if( i == Grappa::mylocale() ) {
        // locally, cores are responsible for their own messages
        source_core_for_locale_[i] = -1;
      } else {
        // guess at correct source core on the node
        Core offset = i / locales_per_core;
        if( offset >= Grappa::locale_cores() ) { // if we guess too high
          // give it to the core that would have been responsible for the local locale.
          offset = Grappa::mylocale() / locales_per_core;
        }
        source_core_for_locale_[i] = Grappa::mylocale() * Grappa::locale_cores() + offset;
      }
    }

    // initialize destination cores.
    // this should match up with source assignments.
    //dest_core_for_locale_ = new Core[ Grappa::locales() ];
    for( int i = 0; i < Grappa::locales(); ++i ) {
      if( i == Grappa::mylocale() ) {
        dest_core_for_locale_[i] = -1;
      } else {
        // guess at correct destination core on the node
        Core offset = Grappa::mylocale() / locales_per_core;
        if( offset >= Grappa::locale_cores() ) { // if we guess too high
          // give it to the core that would have been responsible for
          // the destination's locale.
          offset = i / locales_per_core;
        }
        dest_core_for_locale_[i] = i * Grappa::locale_cores() + offset;
     
      }
    }

    for( int i = 0; i < Grappa::locales(); ++i ) {
      LOG(INFO) << "From locale " << Grappa::mylocale() << " to locale " << i 
                << " source core " << source_core_for_locale_[i]
                << " dest core " << dest_core_for_locale_[i];
    }


    // initialize core data structures
    for( int i = 0; i < Grappa::cores(); ++i ) {
      Locale l = Grappa::locale_of(i);
      cores_[ l ].mylocale_ = l;
      cores_[ l ].representative_core_ = source_core_for_locale_[ l ];
    }
  }

  
    void RDMAAggregator::init() {
#ifdef LEGACY_SEND
#warning RDMA Aggregator is bypassed!
#endif
#ifdef ENABLE_RDMA_AGGREGATOR
      //cores_.resize( global_communicator.nodes() );
      mycore_ = global_communicator.mycore();
      mynode_ = -1; // gasnet supernode
      total_cores_ = global_communicator.cores();


      // register active messages
      deserialize_buffer_handle_ = global_communicator.register_active_message_handler( &deserialize_buffer_am );
      deserialize_first_handle_ = global_communicator.register_active_message_handler( &deserialize_first_am );
      enqueue_buffer_handle_ = global_communicator.register_active_message_handler( &enqueue_buffer_am );
      copy_enqueue_buffer_handle_ = global_communicator.register_active_message_handler( &copy_enqueue_buffer_am );
#endif
    }

    void RDMAAggregator::activate() {
#ifdef ENABLE_RDMA_AGGREGATOR
      // initialize shared data
      if( global_communicator.locale_mycore() == 0 ) {
        // allocate core message list structs
        cores_ = Grappa::impl::locale_shared_memory.segment.construct<CoreData>("Cores")[global_communicator.cores()]();
        // allocate routing info
        source_core_for_locale_ = Grappa::impl::locale_shared_memory.segment.construct<Core>("SourceCores")[global_communicator.locales()]();
        dest_core_for_locale_ = Grappa::impl::locale_shared_memory.segment.construct<Core>("DestCores")[global_communicator.locales()]();
        compute_route_map();
      }

      global_communicator.barrier();

      // attach to shared data
      if( global_communicator.locale_mycore() != 0 ) {
        std::pair< CoreData *, boost::interprocess::managed_shared_memory::size_type > p;
        p = Grappa::impl::locale_shared_memory.segment.find<CoreData>("Cores");
        CHECK_EQ( p.second, global_communicator.cores() );
        cores_ = p.first;

        std::pair< Core *, boost::interprocess::managed_shared_memory::size_type > q;
        q = Grappa::impl::locale_shared_memory.segment.find<Core>("SourceCores");
        CHECK_EQ( q.second, global_communicator.locales() );
        source_core_for_locale_ = q.first;
        q = Grappa::impl::locale_shared_memory.segment.find<Core>("DestCores");
        CHECK_EQ( q.second, global_communicator.locales() );
        dest_core_for_locale_ = q.first;
      }

      // what locales is my core responsible for?
      Locale locales_per_core = Grappa::locales() / Grappa::locale_cores();
      if( locales_per_core == 0 ) locales_per_core = 1;
      core_partner_locales_ = new Locale[ locales_per_core ];
      int partner_index = 0;
      for( int i = 0; i < Grappa::locales(); ++i ) {
        if( source_core_for_locale_[i] == Grappa::mycore() ) {
          CHECK_LT( partner_index, locales_per_core ) << "this core is responsible for more locales than expected";
          core_partner_locales_[ partner_index++ ] = i;
          LOG(INFO) << "Core " << Grappa::mycore() << " responsible for locale " << i;
        }
      }
      core_partner_locale_count_ = partner_index--;
      LOG(INFO) << "Partner locale count is " << core_partner_locale_count_;

      // draw route map if enabled
      draw_routing_graph();

      // fill pool of buffers
      const int num_buffers = Grappa::cores() * FLAGS_rdma_buffers_per_core;
      void * p = Grappa::impl::locale_shared_memory.segment.allocate_aligned( sizeof(RDMABuffer) * num_buffers, 8 );
      CHECK_NOTNULL( p );
      rdma_buffers_ = reinterpret_cast< RDMABuffer * >( p );
      for( int i = 0; i < num_buffers; ++i ) {
        free_buffer_list_.push( &rdma_buffers_[i] );
      }

      // create additional tasks for receive workers and flusher
      //global_scheduler.createWorkers( FLAGS_rdma_workers + 1 );

      // spawn receive workers
      for( int i = 0; i < FLAGS_rdma_workers_per_core; ++i ) {
        Grappa::privateTask( [this] { 
            medium_receive_worker();
          });
      }

      // spawn flusher
      Grappa::privateTask( [this] {
          idle_flusher();
        });

      // // precache buffers
      // LOG(INFO) << "Precaching buffers";
      // size_t expected_buffers = core_partner_locale_count_ * remote_buffer_pool_size;
      // for( int i = 0; i < core_partner_locale_count_; ++i ) {
      //   Core dest = dest_core_for_locale_[i];
      //   LOG(INFO) << "Core " << Grappa::mycore() << " precaching to " << dest;
      //   Core requestor = Grappa::mycore();
      //   auto request = Grappa::message( dest, [dest, requestor, &expected_buffers] {
      //       for( int j = 0; j < remote_buffer_pool_size; ++j ) {
      //         RDMABuffer * b = global_rdma_aggregator.free_buffer_list_.try_pop();
      //         CHECK_NOTNULL( b );
      //         auto reply = Grappa::message( requestor, [dest, b, &expected_buffers] {
      //             global_rdma_aggregator.cores_[dest].remote_buffers_.push( b );
      //             expected_buffers--;
      //           });
      //         reply.send_immediate();
      //       }
      //     });
      //   request.send_immediate();
      // }

      // LOG(INFO) << "Waiting for responses";
      // while( expected_buffers > 0 ) global_communicator.poll();
      // LOG(INFO) << "Done precaching buffers";
      // for( int i = 0; i < core_partner_locale_count_; ++i ) {
      //   Core partner = dest_core_for_locale_[ core_partner_locales_[i] ];
      //   CHECK_EQ( cores_[partner].remote_buffers_.count(), remote_buffer_pool_size );
      // }
#endif
    }

    void RDMAAggregator::finish() {
#ifdef ENABLE_RDMA_AGGREGATOR
      global_communicator.barrier();
      if( global_communicator.locale_mycore() == 0 ) {
        Grappa::impl::locale_shared_memory.segment.destroy<CoreData>("Cores");
        Grappa::impl::locale_shared_memory.segment.destroy<Core>("SourceCores");
        Grappa::impl::locale_shared_memory.segment.destroy<Core>("DestCores");
      }
      cores_ = NULL;
      source_core_for_locale_ = NULL;
      dest_core_for_locale_ = NULL;

      if( core_partner_locales_ ) delete [] core_partner_locales_;
      Grappa::impl::locale_shared_memory.segment.deallocate( rdma_buffers_ );
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

    void RDMAAggregator::copy_enqueue_buffer_am( gasnet_token_t token, void * buf, size_t size ) {
#ifdef DEBUG
      gasnet_node_t src = -1;
      gasnet_AMGetMsgSource(token,&src);
      DVLOG(5) << __func__ << ": Receiving buffer of size " << size << " from " << src << " through gasnet; enqueuing for deserialization";
#endif

      // grab a bufer
      RDMABuffer * b = global_rdma_aggregator.free_buffer_list_.try_pop();
      CHECK_NOTNULL( b ); // for now, die if we don't get one. later, block

      // copy payload into buffer
      memcpy( b->get_base(), buf, size );

      uint64_t sequence_number = reinterpret_cast< uint64_t >( b->get_ack() );
      uint64_t my_size = reinterpret_cast< uint64_t >( b->get_next() );
      
      DVLOG(4) << __func__ << "/" << sequence_number << ": Received buffer " << b << " sequence " << sequence_number
               << " size " << my_size
               << " core0 count " << (b->get_counts())[0]
               << " core1 count " << (b->get_counts())[1]
               << " from core " << b->get_core();

      // enqueue to be dequeued
      global_rdma_aggregator.received_buffer_list_.push( b );
    }






void RDMAAggregator::draw_routing_graph() {
  {
    if( Grappa::mycore() == 0 ) {
      std::ofstream o( FLAGS_route_graph_filename, std::ios::trunc );
      o << "digraph Routing {\n";
      o << "    node [shape=record];\n";
      o << "    splines=true;\n";
    }
  }
  global_communicator.barrier();

  for( int n = 0; n < Grappa::locales(); ++n ) {
    if( Grappa::locale_mycore() == 0 && n == Grappa::mylocale() ) {
      std::ofstream o( FLAGS_route_graph_filename, std::ios::app );

      o << "    n" << n << " [label=\"n" << n;
      for( int c = n * Grappa::locale_cores(); c < (n+1) * Grappa::locale_cores(); ++c ) {
        o << "|";
        if( c == n * Grappa::locale_cores() ) o << "{";
        o << "<c" << c << "> c" << c;
      }
      o << "}\"];\n";

      //for( int c = n * Grappa::locale_cores(); c < (n+1) * Grappa::locale_cores(); ++c ) {
      for( int d = 0; d < Grappa::locales(); ++d ) {
        if( d != Grappa::mylocale() ) {
          o << "    n" << n << ":c" << source_core_for_locale_[d] << ":e"
            << " -> n" << d << ":c" << dest_core_for_locale_[d] << ":w"
            << " [headlabel=\"c" << source_core_for_locale_[d] << "\"]"
            << ";\n";
        }
      }

    }
    global_communicator.barrier();
  }

  {
    if( Grappa::mycore() == 0 ) {
      std::ofstream o( FLAGS_route_graph_filename, std::ios::app );
      o << "}\n";
    }
  }
}







    char * RDMAAggregator::aggregate_to_buffer( char * buffer, Grappa::impl::MessageBase ** message_ptr, size_t max, size_t * count ) {
      size_t size = 0;
      Grappa::impl::MessageBase * message = *message_ptr;
      DVLOG(5) << "Serializing messages from " << message;

      int expected_dest = -1;

      // make space for terminator byte
      max -= 1;

      while( message ) {
        DVLOG(5) << __func__ << ": Serializing message " << message << ": " << message->typestr();

        // issue prefetch for next message
        __builtin_prefetch( message->prefetch_, 0, prefetch_type );

        if( expected_dest == -1 ) {
          expected_dest = message->destination_;
        } else {
          CHECK_EQ( message->destination_, expected_dest );
        }

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

        // done with this buffer. put it back on free list
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












  void RDMAAggregator::receive_buffer( RDMABuffer * b, size_t size ) {
    // deaggregate and execute
    // there should be a message in here delivering a new remote buffer
    DVLOG(5) << __func__ << ": " << "Deaggregating"
             << " from buffer " << b;
    // we will stop deaggregating at terminator byte
    CHECK_EQ( disable_flush_, false );
    disable_flush_ = false;
    global_scheduler.set_no_switch_region( false );

    //deaggregate_buffer( b->get_payload(), b->get_max_size() );
    deaggregate_buffer( b->get_payload(), size );

    global_scheduler.set_no_switch_region( false );
    disable_flush_ = false;
    DVLOG(5) << __func__ << ": " << "Deaggregated"
             << " from buffer " << b;

  }

    void RDMAAggregator::medium_receive_worker() {
      while( !Grappa_done_flag ) {

        // block until we have a buffer to deaggregate
        RDMABuffer * buf = NULL;
        while( buf == NULL ) {
          buf = received_buffer_list_.block_until_pop();
        }

        uint64_t sequence_number = reinterpret_cast< uint64_t >( buf->get_ack() );
        
        DVLOG(4) << __func__ << "/" << sequence_number << ": Now deaggregating buffer " << buf << " sequence " << sequence_number
                 << " core0 count " << (buf->get_counts())[0]
                 << " core1 count " << (buf->get_counts())[1]
                 << " from core " << buf->get_core();

        {
          // now send messages to deaggregate everybody's chunks

          // message to send
          struct ReceiveBuffer {
            char * buf;
            uint16_t size;
            uint64_t sequence_number;
            void operator()() {
              DVLOG(5) << __PRETTY_FUNCTION__ << "/" << sequence_number << ": received " << size << "-byte buffer slice at " << (void*) buf << " to deaggregate";
              global_rdma_aggregator.deaggregate_buffer( buf, size );
              DVLOG(5) << __PRETTY_FUNCTION__ << "/" << sequence_number << ": done deaggregating " << size << "-byte buffer slice at " << (void*) buf;
            }
          };

          // deaggregate messages to send
          Grappa::Message< ReceiveBuffer > msgs[ Grappa::locale_cores() ];

          // index array from buffer
          uint16_t * counts = buf->get_counts();
          char * current_buf = buf->get_payload();
          char * my_buf = NULL;
          int outstanding = 0;


          // fill and send deaggregate messages
          for( Core locale_core = 0; locale_core < Grappa::locale_cores(); ++locale_core ) {
            DVLOG(5) << __func__ << "/" << sequence_number << ": found " << counts[ locale_core ] << " bytes for core " << locale_core << " at " << current_buf;
            if( counts[locale_core] > 0 ) {
              msgs[locale_core]->buf = current_buf;
              msgs[locale_core]->size = counts[locale_core];
              msgs[locale_core]->sequence_number = sequence_number;
              if( locale_core != Grappa::locale_mycore() ) {
                msgs[locale_core].enqueue( locale_core + Grappa::mylocale() * Grappa::locale_cores() );
                outstanding++;
              }
              current_buf += counts[locale_core];
            }
          }

          // deaggregate my messages
          if( counts[ Grappa::locale_mycore() ] > 0 ) {
            DVLOG(5) << __func__ << "/" << sequence_number << ": deaggregating my own " << counts[ Grappa::locale_mycore() ] << "-byte buffer slice at " << (void*) buf;
            deaggregate_buffer( msgs[ Grappa::locale_mycore() ]->buf, counts[ Grappa::locale_mycore() ] );
            DVLOG(5) << __func__ << "/" << sequence_number << ": done deaggregating my own " << counts[ Grappa::locale_mycore() ] << "-byte buffer slice at " << (void*) buf;
          }

          // if we're lucky, responses are now waiting
          receive_poll();

          // block here until messages are sent (i.e., delivered and deaggregated)
          DVLOG(5) << __func__ << "/" << global_scheduler.get_current_thread() << "/" << sequence_number << ": maybe blocking until my " << outstanding << " outstanding messages are delivered";
        }
        DVLOG(5) << __func__ << "/" << sequence_number << ": continuting; my outstanding messages are all delivered";

        // done with this buffer. put it back on free list
        free_buffer_list_.push( buf );
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

  bool RDMAAggregator::send_would_block( Core core ) {
    CoreData * dest = &cores_[ core ];
    return ( !dest->remote_buffers_.available() || 
             free_buffer_list_.empty() );
  }        


  void RDMAAggregator::send_rdma_old( Core core, Grappa::impl::MessageList ml, size_t estimated_size ) {
      CHECK_LT( core, Grappa::cores() ) << "WTF?";

      CoreData * dest = &cores_[ core ];

      size_t serialized_count = 0;
      size_t buffers_sent = 0;
      size_t serialize_calls = 0;
      size_t serialize_null_returns = 0;

      size_t grabbed_count = ml.count_;
      Grappa::impl::MessageBase * messages_to_send = get_pointer( &ml );

      if( 0 == ml.raw_ ) {
        DVLOG(5) << __func__ << ": " << "Aborting send to " << core << " since there's nothing to do";
        return;
      }

      // maybe deliver locally
      if( core == Grappa::mycore() ) {
        deliver_locally( core, messages_to_send );
        return;
      }

      // if( estimated_size < FLAGS_rdma_threshold ) {
      //   send_medium( core, ml );
      //   return;
      // }

      rdma_send_start++;

      DVLOG(5) << __func__ << ": " << "Sending messages via RDMA from " << (void*)cores_[core].messages_.raw_ 
               << " to " << core;
      
      // send as many buffers as we need for this message list
      do {

        // block until we have a remote buffer
        RDMABuffer * remote_buf = dest->remote_buffers_.block_until_pop();
        
        // get a local buffer for aggregation
        RDMABuffer * local_buf = free_buffer_list_.block_until_pop();

        // mark it as being from us
        local_buf->set_core( Grappa::mycore() );
        // local_buf->set_ack( NULL ); //local_buf );
        local_buf->set_ack( local_buf );

        send_with_buffers( core, &messages_to_send, local_buf, remote_buf,
                           &serialized_count, &serialize_calls, &serialize_null_returns );
        buffers_sent++;
      } while( messages_to_send != static_cast<Grappa::impl::MessageBase*>(nullptr) );

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
        DVLOG(4) << __func__ << "/" << global_scheduler.get_current_thread() << ": Delivered message " << messages_to_send 
                 << " with is_delivered_=" << messages_to_send->is_delivered_ 
                 << ": " << messages_to_send->typestr();
        MessageBase * next = messages_to_send->next_;
        __builtin_prefetch( messages_to_send->prefetch_, 0, prefetch_type );

        CHECK_EQ( messages_to_send->destination_, Grappa::mycore() );


        messages_to_send->deliver_locally();
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

    void RDMAAggregator::send_rdma( Core core, Grappa::impl::MessageList ml, size_t expected_size ) {
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
  /// first try at multicore aggregation
  ///


  
  /// See if there are messages to send
  inline bool RDMAAggregator::maybe_has_messages( Core c ) {
    return cores_[ c ].messages_.raw_ != 0;
  }
  
  /// issue prefetches if available
  inline void RDMAAggregator::issue_initial_prefetches( Core core ) {
    for( int i = 0; i < prefetch_dist; ++i ) {
      Grappa::impl::MessageBase * pre = get_pointer( &cores_[core].prefetch_queue_[i] );
      __builtin_prefetch( pre, 0, prefetch_type ); // prefetch for read
      cores_[core].prefetch_queue_[i].raw_ = 0;    // clear out for next time around
    }
  }




  void RDMAAggregator::send_locale_medium( Locale locale ) {
    rdma_send_start++;
    static uint64_t sequence_number = Grappa::mycore();


    Core source_core = source_core_for_locale_[ locale ];
    Core dest_core = dest_core_for_locale_[ locale ];
    Core first_core = locale * Grappa::locale_cores();
    Core max_core = first_core + Grappa::locale_cores();
    Core current_core = first_core;

    DVLOG(4) << __func__ << "/" << sequence_number << ": " << "Sending for locale " << locale;
    CHECK_EQ( source_core, Grappa::mycore() ) << "why am I sending this?";

    // clear out estimated size
    cores_[ first_core ].locale_byte_count_ = 0;

    // grab a temporary buffer
    RDMABuffer * b = free_buffer_list_.try_pop();
    CHECK_NOTNULL( b ); // for now, die if we don't get one. later, block
    
    // but only use enough bytes that we can fit in a medium AM.
    const size_t max_size = 4024;

    // list of messages we're working on
    Grappa::impl::MessageBase * messages_to_send = NULL;

    // send all message lists, maybe in multiple AMs
    while( (current_core < max_core) || (messages_to_send != NULL) ) {
        
      // clear out buffer's count array
      for( Core i = first_core; i < max_core; ++i ) {
        (b->get_counts())[i - first_core] = 0;
      }

      // mark as being from this core
      b->set_core( Grappa::mycore() );

      char * current_buf = b->get_payload();
      size_t remaining_size = std::min( max_size, b->get_max_size() );
      size_t aggregated_size = 0;
      bool ready_to_send = false;

      DVLOG(4) << __func__ << "/" << sequence_number << ": " << "Preparing an active message in buffer " << b << " for locale " << locale;

      // fill a buffer
      while( (!ready_to_send) && 
             ( (current_core < max_core) || (messages_to_send != NULL) ) ) {
        DVLOG(4) << __func__ << ": " << "Grabbing messages for buffer " << b 
                 << " for locale " << locale << " from core " << current_core;


        // if we don't have more messages, grab the next messagelist with stuff to do
        for( ; (messages_to_send == NULL) && (current_core < max_core) ; ++current_core ) {
          DVLOG(4) << __func__ << "/" << sequence_number << ": " << "Trying to grab a new message list for buffer " << b 
                   << " for locale " << locale << " from core " << current_core;

          // does the current core have any messages for us?
          if( maybe_has_messages( current_core ) ) {
            // sure, so grab them and move to next, since we will break later
            Grappa::impl::MessageList ml = grab_messages( current_core );
            if( ml.raw_ != 0 ) { // still have messages
              DVLOG(4) << __func__ << "/" << sequence_number << ": " << "Grabbed messages for buffer " << b << " for locale " << locale << " from core " << current_core;
              // prefetch and grab message list
              issue_initial_prefetches( current_core );
              messages_to_send = get_pointer( &ml );
            }
          }
        }
        // at this point, current_core is the *next* core to process.

        DVLOG(4) << __func__ << "/" << sequence_number << ": " << "Got some messages at " << messages_to_send << " for buffer " << b 
                 << " for locale " << locale << " from core " << current_core;

        // serialize to buffer
        while( (!ready_to_send) && (messages_to_send != nullptr) ) {
          CHECK_LT( aggregated_size, max_size );
          CHECK_GT( remaining_size, 0 );
          DVLOG(4) << __func__ << "/" << sequence_number << ": Serializing message from " << messages_to_send << " with remaining_size=" << remaining_size;

          Grappa::impl::MessageBase * prev_messages_to_send = messages_to_send;

          char * end = aggregate_to_buffer( current_buf, &messages_to_send, remaining_size );
          size_t current_aggregated_size = end - current_buf;

          DVLOG(4) << __func__ << "/" << sequence_number << ": Right after serializing, pointer was " << messages_to_send 
                   << " current_aggregated_size==" << current_aggregated_size
                   << " end-start=" << end - current_buf;

          // we don't use the terminating byte, so get rid of it
          if( current_aggregated_size > 0 ) {
            current_aggregated_size--;
            current_buf = end - 1;
          }
          

          // record how much this core has (minus termination byte)
          int index = current_core - first_core - 1;
          DVLOG(4) << __func__ << "/" << sequence_number << ": Recording " << current_aggregated_size << " bytes"
                   << " for core " << current_core-1
                   << " index " << index
                   << " previous value " << (b->get_counts())[index];
          (b->get_counts())[index] = current_aggregated_size;

          // // do we have space to aggregate more?
          // if( (current_aggregated_size > 0) && (remaining_size > 0) && ((messages_to_send != NULL) || current_core < max_core) {
          //   // ignore terminating byte and prepare for next aggregation
          //   current_aggregated_size--;
          //   current_buf = end - 1;
          //   DVLOG(4) << __func__ << "/" << sequence_number << ": We have more to send; adjusting current_aggregated_size==" << current_aggregated_size
          //            << " end-start=" << end - current_buf;
          // }

          aggregated_size += current_aggregated_size;
          remaining_size -= current_aggregated_size;
          

          // if we couldn't fit the next message, go send
          if( current_aggregated_size == 0 ) ready_to_send = true;

          DVLOG(4) << __func__ << "/" << sequence_number << ": After serializing and fixing up, pointer was " << messages_to_send 
                   << " current_aggregated_size==" << current_aggregated_size
                   << " aggregated_size=" << aggregated_size 
                   << " remaining_size=" << remaining_size
                   << " ready_to_send=" << ready_to_send;


        }

        DVLOG(4) << __func__ << "/" << sequence_number << ": " << "Serialized all we could for buffer " << b 
                 << " for locale " << locale << " from core " << current_core;
      }

      DVLOG(4) << __func__ << "/" << sequence_number << ": " << "Ready to send buffer " << b << " size " << aggregated_size
               << " to locale " << locale << " core " << dest_core;

      if( aggregated_size == 0 ) break;

      DVLOG(4) << __func__ << "/" << sequence_number << ": " << "Sending buffer " << b << " sequence " << sequence_number
               << " size " << aggregated_size
               << " core0 count " << (b->get_counts())[0]
               << " core1 count " << (b->get_counts())[1]
               << " to locale " << locale
               << " core " << dest_core;

      // for debugging
      b->set_next( reinterpret_cast<RDMABuffer*>( aggregated_size ) );
      b->set_ack( reinterpret_cast<RDMABuffer*>( sequence_number ) );
      
      // we have a buffer. send.
      global_communicator.send( dest_core, copy_enqueue_buffer_handle_, b->get_base(), aggregated_size + b->get_base_size() );
      rdma_message_bytes += aggregated_size;
      DVLOG(4) << __func__ << "/" << sequence_number << ": " << "Sent buffer of size " << aggregated_size << " through medium to " << dest_core
               << " with " << messages_to_send << " still to send, then check core " << current_core << " of " << max_core;

      sequence_number += Grappa::cores();
    }

    // return buffer
    free_buffer_list_.push(b);
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
