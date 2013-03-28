
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <fstream>
#include <sstream>
#include <algorithm>

#include "RDMAAggregator.hpp"
#include "Message.hpp"



namespace Grappa {
namespace impl {

void poll();
void failure_function();

}
}


DEFINE_int64( target_size, 1 << 12, "Target size for aggregated messages" );

DEFINE_int64( rdma_workers_per_core, 1 << 4, "Number of RDMA deaggregation worker threads" );
DEFINE_int64( rdma_buffers_per_core, 16, "Number of RDMA aggregated message buffers per core" );

DEFINE_int64( rdma_threshold, 64, "Threshold in bytes below which we send immediately instead of using RDMA" );

DEFINE_string( route_graph_filename, "routing.dot", "Name of file for routing graph" );

DEFINE_bool( rdma_flush_on_idle, true, "Flush RDMA buffers when idle" );

/// stats for application messages
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, app_messages_enqueue, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, app_messages_enqueue_cas, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, app_messages_serialized, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, app_messages_deserialized, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, app_messages_immediate, 0 );

GRAPPA_DEFINE_STAT( SummarizingStatistic<int64_t>, app_messages_delivered_locally, 0 );
GRAPPA_DEFINE_STAT( SummarizingStatistic<int64_t>, app_bytes_delivered_locally, 0 );

//GRAPPA_DEFINE_STAT( SummarizingStatistic<int64_t>, app_message_bytes, 0 );

/// stats for aggregated messages
GRAPPA_DEFINE_STAT( SummarizingStatistic<int64_t>, rdma_message_bytes, 0 );

GRAPPA_DEFINE_STAT( SummarizingStatistic<int64_t>, rdma_first_buffer_bytes, 0 );
GRAPPA_DEFINE_STAT( SummarizingStatistic<int64_t>, rdma_buffers_used_for_send, 0 );

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

GRAPPA_DEFINE_STAT( SummarizingStatistic<double>, rdma_local_delivery_time, 0 );


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

        Grappa::impl::poll();

        // receive_poll();
        
        if( !disable_everything_ ) {

          // see if we have anything at all to send
          for( int i = 0; i < core_partner_locale_count_; ++i ) {
            Locale locale = core_partner_locales_[i];
            // if( check_for_any_work_on( locale ) ) {
              Grappa::signal( &cores_[ locale * Grappa::locale_cores() ].send_cv_ );
            // }
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
    // (make we have at least one locale per core)
    // (round up)
    Locale locales_per_core = 1 + ((Grappa::locales() - 1) / Grappa::locale_cores());


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
      DVLOG(2) << "From locale " << Grappa::mylocale() << " to locale " << i 
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

    void RDMAAggregator::fill_free_pool( size_t num_buffers ) {
        void * p = Grappa::impl::locale_shared_memory.allocate_aligned( sizeof(RDMABuffer) * num_buffers, 8 );
        CHECK_NOTNULL( p );
        DVLOG(2) << "Allocated buffers: " << num_buffers;
        rdma_buffers_ = reinterpret_cast< RDMABuffer * >( p );
        for( int i = 0; i < num_buffers; ++i ) {
          free_buffer_list_.push( &rdma_buffers_[i] );
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

      enqueue_counts_ = new uint64_t[ global_communicator.cores() ];
      aggregate_counts_ = new uint64_t[ global_communicator.cores() ];
      deaggregate_counts_ = new uint64_t[ global_communicator.cores() ];
      for( int i = 0; i < global_communicator.cores(); ++i ) {
        enqueue_counts_[i] = 0;
        aggregate_counts_[i] = 0;
        deaggregate_counts_[i] = 0;
      }

      // register active messages
      deserialize_buffer_handle_ = global_communicator.register_active_message_handler( &deserialize_buffer_am );
      deserialize_first_handle_ = global_communicator.register_active_message_handler( &deserialize_first_am );
      enqueue_buffer_handle_ = global_communicator.register_active_message_handler( &enqueue_buffer_am );
      enqueue_buffer_async_handle_ = global_communicator.register_active_message_handler( &enqueue_buffer_async_am );
      copy_enqueue_buffer_handle_ = global_communicator.register_active_message_handler( &copy_enqueue_buffer_am );
#endif
    }

    void RDMAAggregator::activate() {
#ifdef ENABLE_RDMA_AGGREGATOR
      // one core on each locale initializes shared data
      if( global_communicator.locale_mycore() == 0 ) {
        try {
          // allocate core message list structs
          // one for each core on this locale + one for communication within the locale.
          cores_ = Grappa::impl::locale_shared_memory.segment.construct<CoreData>("Cores")[global_communicator.cores() * 
                                                                                           (global_communicator.locale_cores() + 1 )]();
          
          // allocate routing info
          source_core_for_locale_ = Grappa::impl::locale_shared_memory.segment.construct<Core>("SourceCores")[global_communicator.locales()]();
          dest_core_for_locale_ = Grappa::impl::locale_shared_memory.segment.construct<Core>("DestCores")[global_communicator.locales()]();
        }
        catch(...){
          failure_function();
          throw;
        }
        compute_route_map();
      }

      // make sure everything is allocated before other cores try to attach
      global_communicator.barrier();

      // other cores attach to shared data
      if( global_communicator.locale_mycore() != 0 ) {
        try{
          // attach to core message list structs
          std::pair< CoreData *, boost::interprocess::managed_shared_memory::size_type > p;
          p = Grappa::impl::locale_shared_memory.segment.find<CoreData>("Cores");
          CHECK_EQ( p.second, global_communicator.cores() * (global_communicator.locale_cores() + 1) );
          cores_ = p.first;
          
          // attach to routing info
          std::pair< Core *, boost::interprocess::managed_shared_memory::size_type > q;
          q = Grappa::impl::locale_shared_memory.segment.find<Core>("SourceCores");
          CHECK_EQ( q.second, global_communicator.locales() );
          source_core_for_locale_ = q.first;
          q = Grappa::impl::locale_shared_memory.segment.find<Core>("DestCores");
          CHECK_EQ( q.second, global_communicator.locales() );
          dest_core_for_locale_ = q.first;
        }
        catch(...){
          failure_function();
          throw;
        }
      }

      //
      // what locales is my core responsible for?
      //

      // draw route map if enabled
      draw_routing_graph();

      // spread responsibility for locales between our cores, ensuring
      // that all locales get assigned
      // (round up)
      Locale locales_per_core = 1 + ((Grappa::locales() - 1) / Grappa::locale_cores());

      // generate list of locales this core is responsible for
      core_partner_locales_ = new Locale[ locales_per_core ];
      for( int i = 0; i < Grappa::locales(); ++i ) {
        if( source_core_for_locale_[i] == Grappa::mycore() ) {
          CHECK_LT( core_partner_locale_count_, locales_per_core ) << "this core is responsible for more locales than expected";
          core_partner_locales_[ core_partner_locale_count_++ ] = i;
          DVLOG(2) << "Core " << Grappa::mycore() << " responsible for locale " << i;
        }
      }
      DVLOG(2) << "Partner locale count is " << core_partner_locale_count_ << ", locales per core is " << locales_per_core;

      // fill pool of buffers
      if( core_partner_locale_count_ > 0 ) {
        const int num_buffers = core_partner_locale_count_ * FLAGS_rdma_buffers_per_core;
        DVLOG(2) << "Number of buffers: " << num_buffers;
        fill_free_pool( num_buffers );
      }

      // spawn send workers
      for( int i = 0; i < core_partner_locale_count_; ++i ) {
        Grappa::spawn_worker( [this, i] { 
            DVLOG(5) << "Spawning send worker " << i << " for locale " << core_partner_locales_[i];
            send_worker( core_partner_locales_[i] );
          });
      }

      // spawn receive workers
      for( int i = 0; i < FLAGS_rdma_workers_per_core; ++i ) {
        Grappa::spawn_worker( [this] { 
            receive_worker();
          });
      }

      // spawn flusher
      Grappa::spawn_worker( [this] {
          idle_flusher();
        });

      //
      // precache buffers
      //

      // for now, give each core 1 buffer/token
      size_t expected_buffers = core_partner_locale_count_; // * remote_buffer_pool_size;

      for( int i = 0; i < core_partner_locale_count_; ++i ) {
        Locale locale = core_partner_locales_[i];
        Core dest = dest_core_for_locale_[ locale ];
        Core mylocale_core = Grappa::mylocale() * Grappa::locale_cores();
        Core mycore = Grappa::mycore();

        DVLOG(3) << __PRETTY_FUNCTION__ << ": sending " << expected_buffers << " buffers to core " << dest << " on locale " << locale;

        for( int j = 0; j < remote_buffer_pool_size; ++j ) {
          RDMABuffer * b = global_rdma_aggregator.free_buffer_list_.try_pop();
          CHECK_NOTNULL( b );
          
          // (make heap message since we're not on a shared stack)
          auto request = Grappa::message( dest, [mycore, b] {
              auto p = global_rdma_aggregator.localeCoreData(mycore);
              DVLOG(3) << __PRETTY_FUNCTION__ << " initializing by pushing buffer " << b << " into " << p << " for " << mycore;
              p->remote_buffers_.push( b );
              rdma_buffers_inuse += remote_buffer_pool_size - p->remote_buffers_.count();
            });
          request.send_immediate();
        }
      }

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
      Grappa::impl::locale_shared_memory.deallocate( rdma_buffers_ );
#endif
    }

    void RDMAAggregator::deserialize_buffer_am( gasnet_token_t token, void * buf, size_t size ) {
#ifdef DEBUG
      gasnet_node_t src = -1;
      gasnet_AMGetMsgSource(token,&src);
      DVLOG(5) << __func__ << ": Receiving buffer of size " << size << " through gasnet from " << src;
#endif
      Grappa::impl::global_scheduler.set_no_switch_region( true );
      deaggregate_buffer( static_cast< char * >( buf ), size );
      Grappa::impl::global_scheduler.set_no_switch_region( false );
    }

    void RDMAAggregator::deserialize_first_am( gasnet_token_t token, void * buf, size_t size ) {
#ifdef DEBUG
      gasnet_node_t src = -1;
      gasnet_AMGetMsgSource(token,&src);
      DVLOG(5) << __func__ << ": Receiving buffer of size " << size << " from " << src << " through gasnet; deserializing first entry";
#endif
      app_messages_deserialized++;
      Grappa::impl::global_scheduler.set_no_switch_region( true );
      Grappa::impl::MessageBase::deserialize_and_call( static_cast< char * >( buf ) );
      Grappa::impl::global_scheduler.set_no_switch_region( false );
    }

    void RDMAAggregator::enqueue_buffer_am( gasnet_token_t token, void * buf, size_t size ) {
#ifdef DEBUG
      gasnet_node_t src = -1;
      gasnet_AMGetMsgSource(token,&src);
#endif
      RDMABuffer * b = reinterpret_cast< RDMABuffer * >( buf );
      Core source = b->get_source();
      Core dest = b->get_dest();
      DVLOG(5) << __func__ << ": Receiving buffer of size " << size 
               << " from " << b->get_source() << " to " << b->get_dest()
               << " at " << buf << " with ack " << b->get_ack()
               << " core0 count " << (b->get_counts())[0]
               << " core1 count " << (b->get_counts())[1]
               << "; enqueuing for deserialization";


      global_rdma_aggregator.received_buffer_list_.push( b );
    }

    void RDMAAggregator::enqueue_buffer_async_am( gasnet_token_t token, void * buf, size_t size ) {
#ifdef DEBUG
      gasnet_node_t src = -1;
      gasnet_AMGetMsgSource(token,&src);
      DVLOG(5) << __func__ << ": Receiving buffer of size " << size << " from " << src << " through gasnet; enqueuing for deserialization";
#endif
      RDMABuffer * b = reinterpret_cast< RDMABuffer * >( buf );
      RDMABuffer * ack = b->get_ack();
      global_rdma_aggregator.received_buffer_list_.push( b );

      // send reply as required by spec
      auto reply = Grappa::message( b->get_source(), [ack] {
          global_rdma_aggregator.free_buffer_list_.push( ack );
        } );
    }

    void RDMAAggregator::copy_enqueue_buffer_am( gasnet_token_t token, void * buf, size_t size ) {
#ifdef DEBUG
      gasnet_node_t src = -1;
      gasnet_AMGetMsgSource(token,&src);
      DVLOG(5) << __func__ << ": Receiving buffer of size " << size << " from " << src << " through gasnet; enqueuing for deserialization";
#endif

      Grappa::impl::global_scheduler.set_no_switch_region( true );

      // grab a buffer
      RDMABuffer * b = global_rdma_aggregator.free_buffer_list_.try_pop();
      CHECK_NOTNULL( b ); // for now, die if we don't get one. later, block

      // copy payload into buffer
      memcpy( b->get_base(), buf, size );
      //CHECK_NULL( b->get_next() );
      b->set_next( NULL );

      uint64_t sequence_number = reinterpret_cast< uint64_t >( b->get_ack() );
      uint64_t my_size = reinterpret_cast< uint64_t >( b->get_next() );
      
      DVLOG(4) << __func__ << "/" << sequence_number << ": Received buffer " << b << " sequence " << sequence_number
               << " size " << my_size
               << " core0 count " << (b->get_counts())[0]
               << " core1 count " << (b->get_counts())[1]
               << " from core " << b->get_source();

      // check that the buffer makes sense
      size_t claimed_size = b->get_base_size();
      for( int i = 0; i < Grappa::locale_cores(); ++i ) {
        claimed_size += (b->get_counts())[i];
      }
      CHECK_EQ( claimed_size, size ) << "Buffer claims it is a different size than what was received";

      // enqueue to be dequeued
      global_rdma_aggregator.received_buffer_list_.push( b );

      Grappa::impl::global_scheduler.set_no_switch_region( false );
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

      while( message ) {
        DVLOG(5) << __func__ << ": Serializing message " << message << " to " << message->destination_ << ": " << message->typestr();

        // issue prefetch for next message
        __builtin_prefetch( message->prefetch_, 1, prefetch_type );

        if( expected_dest == -1 ) {
          expected_dest = message->destination_;
        } else {
          CHECK_EQ( message->destination_, expected_dest );
        }

        // add message to buffer
        char * new_buffer = message->serialize_to( buffer, max - size);
        CHECK_GE( new_buffer, buffer );

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

      return buffer;
    }

    char * RDMAAggregator::deaggregate_buffer( char * buffer, size_t size ) {
      DVLOG(5) << __func__ << ": Deaggregating buffer at " << (void*) buffer << " of max size " << size;
      char * end = buffer + size;
      while( buffer < end ) {
        app_messages_deserialized++;
        DVLOG(5) << __func__ << ": Deserializing and calling at " << (void*) buffer << " with " << end - buffer << " remaining";
        char * next = Grappa::impl::MessageBase::deserialize_and_call( buffer );
        DVLOG(5) << __func__ << ": Deserializing and called at " << (void*) buffer << " with next " << (void*) next;
        buffer = next;
      }
      DVLOG(5) << __func__ << ": Done deaggregating buffer";
      return buffer; /// how far did we get before we had to stop?
    }



  void RDMAAggregator::dump_counts() {
    {
      std::stringstream ss;
      ss << "Enqueued:";
      for( int i = 0; i < Grappa::cores(); ++i ) {
        ss << " " << i << ":" << enqueue_counts_[i];
      }
      LOG(INFO) << ss.str();
    }
    {
      std::stringstream ss;
      ss << "Aggregated:";
      for( int i = 0; i < Grappa::cores(); ++i ) {
        ss << " " << i << ":" << aggregate_counts_[i];
      }
      LOG(INFO) << ss.str();
    }
    {
      std::stringstream ss;
      ss << "Received:";
      for( int i = 0; i < Grappa::cores(); ++i ) {
        ss << " " << i << ":" << deaggregate_counts_[i];
      }
      LOG(INFO) << ss.str();

      LOG(INFO) << "Free buffers: " << free_buffer_list_.count();
      LOG(INFO) << "Received buffers: " << received_buffer_list_.count();
      LOG(INFO) << "Active send workers: " << active_send_workers_;
      LOG(INFO) << "Active receive workers: " << active_receive_workers_;
    }
  }


    ///
    /// new approach to send/recv
    ///

  // block until we're ready to send to a locale and do so
  void RDMAAggregator::send_worker( Locale locale ) {
    CoreData * locale_core = localeCoreData( locale * Grappa::locale_cores() );

    while( !Grappa_done_flag ) {

      // block until it's time to send to this locale
      Grappa::wait( &(locale_core->send_cv_) );

      active_send_workers_++;
      
      //CHECK_EQ( disable_everything_, false ) << "Whoops! Why are we sending when disabled?";
      if( disable_everything_ ) LOG(WARNING) << "Sending while disabled...";

      // send to locale
      send_locale( locale );

      // record when we last sent
      // TODO: should this go earlier? probably not.
      // TODO: should we tick here?
      Grappa_tick();
      locale_core->last_sent_ = Grappa_get_timestamp();

      // send done! loop!
      active_send_workers_--;
    }
  }

  // block until there's something to receive and do so
  void RDMAAggregator::receive_worker() {
    RDMABuffer * buf = NULL;
    while( !Grappa_done_flag ) {
      
      // block until we have a buffer to deaggregate
      buf = NULL;
      while( buf == NULL ) {
        buf = received_buffer_list_.block_until_pop();
        CHECK_NOTNULL( buf );
      }
      
      active_receive_workers_++;

      DVLOG(3) << __PRETTY_FUNCTION__ << "/" << Grappa::impl::global_scheduler.get_current_thread() 
               << " processing buffer " << buf 
               << " serial " << reinterpret_cast<int64_t>( buf->get_ack() );

      // what core is buffer from?
      Core c = buf->get_source();
      
      // process buffer
      receive_buffer( buf );

      // once we're done, send ack to give permission to send again,
      // unless buffer is from the local core (for debugging)
      if( c != Grappa::mycore() ) {
        Core mylocale_core = Grappa::mylocale() * Grappa::locale_cores();
        Core mycore = Grappa::mycore();
        DVLOG(3) << __PRETTY_FUNCTION__ << "/" << Grappa::impl::global_scheduler.get_current_thread() << " finished with " << buf
                 << "; acking with buffer " << buf << " for core " << mycore << " on core " << c;
        auto request = Grappa::message( c, [mycore, buf] {
            auto p = global_rdma_aggregator.localeCoreData(mycore);
            DVLOG(3) << __PRETTY_FUNCTION__ << " acking by pushing buffer " << buf << " into " << p << " for " << mycore;
            p->remote_buffers_.push( buf );
            rdma_buffers_inuse += remote_buffer_pool_size - p->remote_buffers_.count();
          });
        request.send_immediate();
      } else {
        DVLOG(3) << __PRETTY_FUNCTION__ << "/" << Grappa::impl::global_scheduler.get_current_thread() << " finished with " << buf
                 << "; pushing on free list";
        free_buffer_list_.push( buf );
      }
            
      active_receive_workers_--;
    }
  }


  // block until there's something to receive and do so
  void RDMAAggregator::medium_receive_worker() {
    RDMABuffer * buf = NULL;
    while( !Grappa_done_flag ) {
      
      // block until we have a buffer to deaggregate
      buf = NULL;
      while( buf == NULL ) {
        buf = received_buffer_list_.block_until_pop();
        CHECK_NOTNULL( buf );
      }
      
      active_receive_workers_++;

      DVLOG(3) << __PRETTY_FUNCTION__ << "/" << Grappa::impl::global_scheduler.get_current_thread() 
               << " processing buffer " << buf 
               << " serial " << reinterpret_cast<int64_t>( buf->get_ack() );

      // what core is buffer from?
      Core c = buf->get_source();
      
      // process buffer
      receive_buffer( buf );

      // done with this buffer. put it back on free list
      free_buffer_list_.push( buf );

      // once we're done, send ack to give permission to send again,
      // unless buffer is from the local core (for debugging)
      if( c != Grappa::mycore() ) {
        // TODO: for now, just send token
        RDMABuffer * buf = NULL;
        Core mylocale_core = Grappa::mylocale() * Grappa::locale_cores();
        Core mycore = Grappa::mycore();
        DVLOG(3) << __PRETTY_FUNCTION__ << "/" << Grappa::impl::global_scheduler.get_current_thread() << " finished with " << buf
                 << "; acking with buffer " << buf << " for core " << mycore << " on core " << c;
        auto request = Grappa::message( c, [mycore, buf] {
            auto p = global_rdma_aggregator.localeCoreData(mycore);
            DVLOG(3) << __PRETTY_FUNCTION__ << " acking by pushing buffer " << buf << " into " << p << " for " << mycore;
            p->remote_buffers_.push( buf );
            rdma_buffers_inuse += remote_buffer_pool_size - p->remote_buffers_.count();
          });
        request.send_immediate();
      }
            
      active_receive_workers_--;
    }
  }
















  void RDMAAggregator::receive_buffer( RDMABuffer * buf ) {

    uint64_t sequence_number = reinterpret_cast< uint64_t >( buf->get_ack() );
        
    DVLOG(4) << __func__ << "/" << sequence_number << ": Now deaggregating buffer " << buf << " sequence " << sequence_number
             << " core0 count " << (buf->get_counts())[0]
             << " core1 count " << (buf->get_counts())[1]
             << " from core " << buf->get_source();

    {
      // now send messages to deaggregate everybody's chunks

      // message to send
      struct ReceiveBuffer {
        char * buf;
        uint16_t size;
        uint64_t sequence_number;
        void operator()() {
          DVLOG(5) << __PRETTY_FUNCTION__ << "/" << sequence_number
                   << ": received " << size << "-byte buffer slice at " << (void*) buf << " to deaggregate";

          global_rdma_aggregator.deaggregate_buffer( buf, size );

          DVLOG(5) << __PRETTY_FUNCTION__ << "/" << sequence_number 
                   << ": done deaggregating " << size << "-byte buffer slice at " << (void*) buf;
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
        DVLOG(5) << __func__ << "/" << sequence_number 
                 << ": found " << counts[ locale_core ] << " bytes for core " << locale_core << " at " << (void*) current_buf;

        if( counts[locale_core] > 0 ) {

          msgs[locale_core]->buf = current_buf;
          msgs[locale_core]->size = counts[locale_core];
          msgs[locale_core]->sequence_number = sequence_number;

          if( locale_core != Grappa::locale_mycore() ) {
            msgs[locale_core].locale_enqueue( locale_core + Grappa::mylocale() * Grappa::locale_cores() );
            outstanding++;
          } else {
            my_buf = current_buf;
          }

          current_buf += counts[locale_core];
        }
      }

      // deaggregate my messages
      if( counts[ Grappa::locale_mycore() ] > 0 ) {
        DVLOG(5) << __func__ << "/" << sequence_number 
                 << ": deaggregating my own " << counts[ Grappa::locale_mycore() ] << "-byte buffer slice at " << (void*) buf;

        Grappa::impl::global_scheduler.set_no_switch_region( true );
        deaggregate_buffer( my_buf, counts[ Grappa::locale_mycore() ] );
        Grappa::impl::global_scheduler.set_no_switch_region( false );

        DVLOG(5) << __func__ << "/" << sequence_number 
                 << ": done deaggregating my own " << counts[ Grappa::locale_mycore() ] << "-byte buffer slice at " << (void*) buf;
      }

      // // if we're lucky, responses are now waiting
      // receive_poll();

      // block here until messages are sent (i.e., delivered and deaggregated)
      DVLOG(5) << __func__ << "/" << Grappa::impl::global_scheduler.get_current_thread() << "/" << sequence_number 
               << ": maybe blocking until my " << outstanding << " outstanding messages are delivered";
    }

    DVLOG(5) << __func__ << "/" << sequence_number 
             << ": continuting; my outstanding messages are all delivered";
  }









      size_t RDMAAggregator::deliver_locally( Core core,
                                            Grappa::impl::MessageList ml,
                                            CoreData * dest ) {
      MessageBase * messages_to_send = get_pointer( &ml );
      size_t delivered_count = 0;
      size_t delivered_bytes = 0;

      DVLOG(4) << "Delivering locally with " << ml.count_;

      //
      // serialize
      //
      
      // issue initial prefetches
      // (these may have been overwritten by another sender, so hope for the best.)
      for( int i = 0; i < prefetch_dist; ++i ) {
        Grappa::impl::MessageBase * pre = get_pointer( &dest->prefetch_queue_[i] );
        __builtin_prefetch( pre, 1, prefetch_type ); // prefetch for read
      }

      Grappa::impl::global_scheduler.set_no_switch_region( true );
      while( messages_to_send != NULL ) {
        DVLOG(4) << __func__ << "/" << Grappa::impl::global_scheduler.get_current_thread() << ": Delivered message " << messages_to_send 
                 << " with is_delivered_=" << messages_to_send->is_delivered_ 
                 << ": " << messages_to_send->typestr();
        MessageBase * next = messages_to_send->next_;
        __builtin_prefetch( messages_to_send->prefetch_, 1, prefetch_type );

        DCHECK_EQ( messages_to_send->destination_, Grappa::mycore() );


        delivered_bytes += messages_to_send->size();
        messages_to_send->deliver_locally();
        delivered_count++;

        messages_to_send = next;
      }
      Grappa::impl::global_scheduler.set_no_switch_region( false );

      app_messages_delivered_locally += delivered_count;
      app_bytes_delivered_locally += delivered_bytes;
       
      return delivered_count;
    }








  ///
  /// first try at multicore aggregation
  ///


  
  /// See if there are messages to send
  inline bool RDMAAggregator::maybe_has_messages( Core c, Core locale_source ) {
    return coreData( c, locale_source )->messages_.raw_ != 0;
  }
  
  /// issue prefetches if available
  inline void RDMAAggregator::issue_initial_prefetches( Core core, Core locale_source ) {
    for( int i = 0; i < prefetch_dist; ++i ) {
      Grappa::impl::MessageBase * pre = get_pointer( &(coreData(core, locale_source)->prefetch_queue_[i])  );
      __builtin_prefetch( pre, 0, prefetch_type ); // prefetch for read
      coreData( core, locale_source )->prefetch_queue_[i].raw_ = 0;    // clear out for next time around
    }
  }

  inline void RDMAAggregator::issue_initial_prefetches( CoreData * cd ) {
    for( int i = 0; i < prefetch_dist; ++i ) {
      Grappa::impl::MessageBase * pre = get_pointer( &(cd->prefetch_queue_[i])  );
      __builtin_prefetch( pre, 0, prefetch_type ); // prefetch for read
      cd->prefetch_queue_[i].raw_ = 0;    // clear out for next time around
    }
  }












  class MessageListChooser {
  private:
    // indices of source core message lists
    Core dest_cores_start_;
    Core dest_cores_end_;
    
    // cores in this locale to pull from
    Core source_cores_start_;
    Core source_cores_end_;
    
    // where are we right now?
    Core current_dest_core_;
    Core current_source_core_;
    
    bool done;
    
  public:
    MessageListChooser() = delete;
    MessageListChooser( const MessageListChooser& ) = delete;
    MessageListChooser( MessageListChooser&& ) = delete;

    MessageListChooser( Core dest_start, Core dest_end, Core source_start, Core source_end ) 
      : dest_cores_start_( dest_start )
      , dest_cores_end_( dest_end )
      , source_cores_start_( source_start )
      , source_cores_end_( source_end )
      , current_dest_core_( dest_cores_start_ )
      , current_source_core_( source_cores_start_ )
      , done( false )
    { 
      DVLOG(4) << __func__ << "Initialized with dest range " << dest_cores_start_ << "/" << dest_cores_end_
               << " source range " << source_cores_start_ << "/" << source_cores_end_;
    }

    CoreData * get_next_list( Core * c ) {
      CoreData * cd = NULL;

      if( !done ) {
        do {
          // load next core data pointer. ideally this will have messages most of the time.
          cd = global_rdma_aggregator.coreData( current_dest_core_, current_source_core_ );
          *c = current_dest_core_;

          DVLOG(4) << __func__ << ": Fetched CoreData ptr " << cd << " dest core " << current_dest_core_
                   << " with cd " << cd << " messageList " << cd->messages_.raw_;

          // got nothing, so go to next source core in this locale
          current_source_core_++;
          
          // if we've checked all the local cores for this destination
          if( current_source_core_ >= source_cores_end_ ) {
            // reset source
            current_source_core_ = source_cores_start_;
            // and increment dest
            current_dest_core_++;
            // if we've also checked all the destinations
            if( current_dest_core_ >= dest_cores_end_ ) {
              done = true; // we're done.
            }
          }

          // if we still have something to fetch, prefetch for next
          // call (assuming we found something earlier and don't have
          // to keep searching....)
          if( !done ) {
            CoreData * next = global_rdma_aggregator.coreData( current_dest_core_, current_source_core_ );
            __builtin_prefetch( next, 1, prefetch_type );
          }
        } while( 0 == cd->messages_.raw_ && !done );
      }
      
      return cd;
    }
  };


  void RDMAAggregator::send_locale( Locale locale ) {
    rdma_send_start++;
    active_send_workers_++;
    static uint64_t sequence_number = Grappa::mycore();

    int buffers_used_for_send = 0;

    // this is the core we are sending to
    Core dest_core = dest_core_for_locale_[ locale ];

    bool all_message_lists_sent = false;

    Core first_core = locale * Grappa::locale_cores();
    Core max_core = first_core + Grappa::locale_cores();
    Core current_dest_core = -1;

    MessageListChooser mlc( first_core, max_core, 0, Grappa::locale_cores() );
    DVLOG(3) << __PRETTY_FUNCTION__ << "/" << Grappa::impl::global_scheduler.get_current_thread() 
             << " MessageListChooser constructed at " << &mlc;


    // list of messages we're working on
    Grappa::impl::MessageBase * messages_to_send = NULL;

    // send all available message lists for this locale, maybe in multiple 
    while( (!all_message_lists_sent) || (messages_to_send != NULL) ) {

      // first, grab a token/buffer to send
      // by doing this first, we limit the rate at which we consume buffers from the local free list
      // as well as allowing the remote node to limit the rate we send
      RDMABuffer * dest_buf = localeCoreData( dest_core )->remote_buffers_.block_until_pop();
      CHECK_NOTNULL( dest_buf );

      rdma_buffers_inuse += remote_buffer_pool_size - localeCoreData( dest_core )->remote_buffers_.count();

      // now, grab a temporary buffer for local serialization
      RDMABuffer * b = free_buffer_list_.block_until_pop();

      // but only use enough bytes that we can fit in a medium AM.
      const size_t max_size = b->get_max_size();
      DVLOG(5) << "Max buffer size is " << max_size;
      
      // clear out buffer's count array
      for( Core i = first_core; i < max_core; ++i ) {
        (b->get_counts())[i - first_core] = 0;
      }

      // mark as being from this core
      b->set_source( Grappa::mycore() );

      char * current_buf = b->get_payload();
      int64_t remaining_size = std::min( max_size, b->get_max_size() );
      size_t aggregated_size = 0;
      bool ready_to_send = false;

      int64_t count = 0;

      DVLOG(4) << __func__ << "/" << sequence_number << ": " << "Preparing an active message in buffer " << b << " for locale " << locale;

      // fill the buffer
      while( (!ready_to_send) &&                           // buffer is not full
             ( (!all_message_lists_sent) ||
               (messages_to_send != NULL) ) ) {

        // if we have no more to send now, get some
        if( messages_to_send == NULL ) {
          DVLOG(4) << __func__ << "/" << sequence_number << ": " 
                   << "Trying to grab a new message list for buffer " << b 
                   << " for locale " << locale << " from core " << current_dest_core;


          // get the next core struct with something to send
          CoreData * core_data = mlc.get_next_list( &current_dest_core );

          // if we got something,
          if( core_data != NULL ) {
            // prefetch and grab head of message list
            Grappa::impl::MessageList ml = grab_messages( core_data );
            issue_initial_prefetches( core_data );
            messages_to_send = get_pointer( &ml );
          } else {
            // break out of loop; we've sent all available messages
            all_message_lists_sent = true;
            break;
          }

        }

        // serialize to buffer
        while( (!ready_to_send) && (messages_to_send != nullptr) ) {
          CHECK_LT( aggregated_size, max_size );
          CHECK_GT( remaining_size, 0 );
          DVLOG(4) << __func__ << "/" << sequence_number 
                   << ": Serializing messages from " << messages_to_send 
                   << " to " << current_dest_core
                   << " with remaining_size=" << remaining_size;

          Grappa::impl::MessageBase * prev_messages_to_send = messages_to_send;
          CHECK_EQ( messages_to_send->destination_, current_dest_core ) << "hmm. this doesn't seem right";

          char * end = aggregate_to_buffer( current_buf, &messages_to_send, remaining_size, &aggregate_counts_[current_dest_core] );
          size_t current_aggregated_size = end - current_buf;
          CHECK_LE( aggregated_size + current_aggregated_size, max_size );
          CHECK_GE( remaining_size, 0 );

          DVLOG(4) << __func__ << "/" << sequence_number 
                   << ": Right after serializing, pointer was " << messages_to_send 
                   << " current_aggregated_size==" << current_aggregated_size
                   << " end-start=" << end - current_buf;

          // record how much this core has
          int index = current_dest_core - first_core;
          DVLOG(4) << __func__ << "/" << sequence_number 
                   << ": Recording " << current_aggregated_size << " bytes"
                   << " for core " << current_dest_core
                   << " index " << index
                   << " previous value " << (b->get_counts())[index];
          (b->get_counts())[index] += current_aggregated_size;
          
          // update pointer
          current_buf = end;


          aggregated_size += current_aggregated_size;
          remaining_size -= current_aggregated_size;
          if( remaining_size == 0 ) ready_to_send = true;

          // if we couldn't fit the next message, go send
          if( current_aggregated_size == 0 ) ready_to_send = true;

          DVLOG(4) << __func__ << "/" << sequence_number << ": After serializing and fixing up, pointer was " << messages_to_send 
                   << " current_aggregated_size==" << current_aggregated_size
                   << " aggregated_size=" << aggregated_size 
                   << " remaining_size=" << remaining_size
                   << " ready_to_send=" << ready_to_send;
        }

        DVLOG(4) << __func__ << "/" << sequence_number 
                 << ": " << "Serialized all we could for buffer " << b 
                 << " for locale " << locale << " core " << current_dest_core;
      }

      if( aggregated_size >= 0 ) {
        DVLOG(4) << __func__ << "/" << sequence_number << ": " 
                 << "Ready to send buffer " << b << " size " << aggregated_size
                 << " to locale " << locale << " core " << dest_core;
      } else {
        DVLOG(4) << __func__ << "/" << sequence_number << ": " 
               << "Nothing to send in buffer " << b << " size " << aggregated_size
               << " to locale " << locale << " core " << dest_core;
        break;
      }

      DVLOG(3) << __func__ << "/" << sequence_number << ": " << "Sending buffer " << b << " sequence " << sequence_number
               << " size " << aggregated_size
               << " core0 count " << (b->get_counts())[0]
               << " core1 count " << (b->get_counts())[1]
               << " to locale " << locale
               << " core " << dest_core
               << " buf " << dest_buf;

      // for debugging
      b->set_next( reinterpret_cast<RDMABuffer*>( aggregated_size ) );

      b->set_source( Grappa::mycore() );
      b->set_ack( b );
      
      if( aggregated_size > 0 ) {
        // we have a buffer. send.
        global_communicator.send( dest_core, enqueue_buffer_handle_, b->get_base(), aggregated_size + b->get_base_size(), dest_buf );
        rdma_message_bytes += aggregated_size + b->get_base_size();
        if( buffers_used_for_send == 0 ) rdma_first_buffer_bytes += aggregated_size + b->get_base_size();
        buffers_used_for_send++;
        DVLOG(4) << __func__ << "/" << sequence_number 
                 << ": Sent buffer of size " << aggregated_size 
                 << " through RDMA to " << dest_core
                 << " with " << messages_to_send 
                 << " still to send, then check core " << current_dest_core << " of " << max_core;
      } else {
        DVLOG(4) << __func__ << "/" << sequence_number 
                 << ": No message to send; returning dest buffer " << dest_buf << " to pool";
        // return buffer to pool
        localeCoreData( dest_core )->remote_buffers_.push( dest_buf );
        rdma_buffers_inuse += remote_buffer_pool_size - localeCoreData( dest_core )->remote_buffers_.count();
      }

      sequence_number += Grappa::cores();

      // return buffer (only safe for non-async rdma sends)
      free_buffer_list_.push(b);
    }

    if( buffers_used_for_send > 0 ) rdma_buffers_used_for_send += buffers_used_for_send;

    CHECK_NULL( messages_to_send );

    active_send_workers_--;
    rdma_send_end++;
  }


  }
}
