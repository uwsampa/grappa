////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#include <fstream>
#include <sstream>
#include <algorithm>
#include <tuple>

#include "RDMAAggregator.hpp"
#include "Message.hpp"
#include "Aggregator.hpp"


namespace Grappa {
  namespace impl {
    // defined in Grappa.cpp
    extern void failure_function();
  }
}

// (don't) prefetch two cache lines per message
//#define DOUBLE_LINE_PREFETCH

DEFINE_bool( enable_aggregation, true, "Enable message aggregation." );

DEFINE_int64( target_size, 0, "Deprecated; do not use. Target size for aggregated messages capacity flushes; disabled by default" );

DEFINE_int64( aggregator_target_size, 1 << 10, "Target size for aggregated messages" );

DECLARE_int64( log2_concurrent_receives );
DECLARE_int64( log2_concurrent_sends );

DEFINE_int64( rdma_workers_per_core, 1 << 6, "Number of RDMA deaggregation worker threads" );
DEFINE_int64( rdma_buffers_per_core, 1 << 7, "Number of RDMA aggregated message buffers per core" );

DEFINE_int64( rdma_threshold, 64, "Threshold in bytes below which we send immediately instead of using RDMA" );

DEFINE_string( route_graph_filename, "", "Name of file for routing graph output (no output if empty)" );

DEFINE_bool( rdma_flush_on_idle, true, "Flush RDMA buffers when idle" );

/// stats for application messages
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, app_messages_enqueue, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, app_messages_enqueue_cas, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, app_messages_immediate, 0 );

GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, app_messages_serialized, 0 );
GRAPPA_DEFINE_METRIC( SummarizingMetric<int64_t>, app_bytes_serialized, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, app_messages_deserialized, 0 );

GRAPPA_DEFINE_METRIC( SummarizingMetric<int64_t>, app_nt_message_bytes, 0 );
GRAPPA_DEFINE_METRIC( SummarizingMetric<int64_t>, aggregated_nt_message_bytes, 0 );

GRAPPA_DEFINE_METRIC( SummarizingMetric<int64_t>, app_messages_delivered_locally, 0 );
GRAPPA_DEFINE_METRIC( SummarizingMetric<int64_t>, app_bytes_delivered_locally, 0 );

//GRAPPA_DEFINE_METRIC( SummarizingMetric<int64_t>, app_message_bytes, 0 );

/// stats for aggregated messages
GRAPPA_DEFINE_METRIC( SummarizingMetric<int64_t>, rdma_message_bytes, 0 );

GRAPPA_DEFINE_METRIC( SummarizingMetric<int64_t>, rdma_first_buffer_bytes, 0 );
GRAPPA_DEFINE_METRIC( SummarizingMetric<int64_t>, rdma_buffers_used_for_send, 0 );

/// stats for RDMA Aggregator events

GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_receive_start, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_receive_end, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_send_start, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_send_end, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_capacity_flushes, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_idle_flushes, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_core_idle_flushes, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_requested_flushes, 0 );

GRAPPA_DEFINE_METRIC( SummarizingMetric<int64_t>, rdma_buffers_inuse, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_buffers_blocked, 0 );


GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_poll, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_poll_send, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_poll_receive, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_poll_send_success, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_poll_receive_success, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_poll_yields, 0 );

GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_flush_send, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_flush_receive, 0 );

GRAPPA_DEFINE_METRIC( SummarizingMetric<double>, rdma_local_delivery_time, 0 );

GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, workers_send_blocked, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, workers_idle_blocked, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, workers_receive_blocked, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, workers_block_remote_buffer, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, workers_block_local_buffer, 0 );
GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, workers_active_send, 0 );

GRAPPA_DEFINE_METRIC( SimpleMetric<int64_t>, rdma_enqueue_buffer_am, 0 );


GRAPPA_DEFINE_METRIC(HistogramMetric, app_bytes_sent_histogram, 0);
GRAPPA_DEFINE_METRIC(HistogramMetric, rdma_bytes_sent_histogram, 0);

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
        ++workers_idle_blocked;
        Grappa::wait( &flush_cv_ );
        --workers_idle_blocked;
        rdma_idle_flushes++;

        Core c = Grappa::mycore();
        
        /////////////////////////////////////////////////////
        // came from old Grappa::impl::poll() in Grappa.cpp
        // (not sure why it was polling the other aggregator...)
        global_communicator.poll();
        global_aggregator.poll();
        /////////////////////////////////////////////////////

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

        // flush NT buffers
        size_t mru;
        while( boost::dynamic_bitset<>::npos != (mru = nt_mru_.find_first() ) ) {
          DVLOG(5) << "Flushing NT buffer for " << mru << ": " << &ntbuffers_[mru];
          nt_flush( &ntbuffers_[mru] );
          send_nt_buffer( mru, &ntbuffers_[mru] );
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
      //cores_.resize( global_communicator.cores );
      mycore_ = global_communicator.mycore;
      mynode_ = -1; // gasnet supernode
      total_cores_ = global_communicator.cores;

      nt_mru_.resize( global_communicator.cores );
      
      enqueue_counts_ = new uint64_t[ global_communicator.cores ];
      aggregate_counts_ = new uint64_t[ global_communicator.cores ];
      deaggregate_counts_ = new uint64_t[ global_communicator.cores ];
      for( int i = 0; i < global_communicator.cores; ++i ) {
        enqueue_counts_[i] = 0;
        aggregate_counts_[i] = 0;
        deaggregate_counts_[i] = 0;
      }
#endif

      if( global_communicator.mycore == 0 ) {
        if( !FLAGS_enable_aggregation ) {
          if( FLAGS_log2_concurrent_receives - FLAGS_log2_concurrent_sends < 4 ) { // arbitrary
            LOG(WARNING) << "Your buffer settings may lead to starvation without aggregation; we suggest sends=3 and receives=7";
          }
        }
      }

      // where can NTBuffer start storing data?
      NTBuffer::set_initial_offset( 4 ); // TODO: for now we say 32 bytes.
      ntbuffers_ = new NTBuffer[ global_communicator.cores ];
    }
    
    size_t RDMAAggregator::estimate_footprint() const {
      return (core_partner_locale_count_ + FLAGS_rdma_workers_per_core + 1) * FLAGS_stack_size
        + (sizeof(Core)*2 + sizeof(CoreData)) * global_communicator.locales;
    }
    
    size_t RDMAAggregator::adjust_footprint(size_t target) {
      if (estimate_footprint() > target) {
        MASTER_ONLY LOG(WARNING) << "Adjusting to fit in target footprint: " << target << " bytes";
        while (estimate_footprint() > target) {
          if (FLAGS_rdma_workers_per_core > 1) FLAGS_rdma_workers_per_core--;
        }
      }
      return estimate_footprint();
    }

    
    void RDMAAggregator::activate() {
#ifdef ENABLE_RDMA_AGGREGATOR
      // one core on each locale initializes shared data
      if( global_communicator.locale_mycore == 0 ) {
        try {
          // allocate core message list structs
          // one for each core on this locale + one for communication within the locale.
          cores_ = Grappa::impl::locale_shared_memory.segment.construct<CoreData>("Cores")[global_communicator.cores * 
                                                                                           (global_communicator.locale_cores + 1 )]();
          
          // allocate routing info
          source_core_for_locale_ = Grappa::impl::locale_shared_memory.segment.construct<Core>("SourceCores")[global_communicator.locales]();
          dest_core_for_locale_ = Grappa::impl::locale_shared_memory.segment.construct<Core>("DestCores")[global_communicator.locales]();
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
      if( global_communicator.locale_mycore != 0 ) {
        try{
          // attach to core message list structs
          std::pair< CoreData *, boost::interprocess::managed_shared_memory::size_type > p;
          p = Grappa::impl::locale_shared_memory.segment.find<CoreData>("Cores");
          CHECK_EQ( p.second, global_communicator.cores * (global_communicator.locale_cores + 1) );
          cores_ = p.first;
          
          // attach to routing info
          std::pair< Core *, boost::interprocess::managed_shared_memory::size_type > q;
          q = Grappa::impl::locale_shared_memory.segment.find<Core>("SourceCores");
          CHECK_EQ( q.second, global_communicator.locales );
          source_core_for_locale_ = q.first;
          q = Grappa::impl::locale_shared_memory.segment.find<Core>("DestCores");
          CHECK_EQ( q.second, global_communicator.locales );
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
#ifndef LEGACY_SEND
        for( int i = 0; i < core_partner_locale_count_; ++i ) {
          Grappa::spawn_worker( [this, i] { 
                                  DVLOG(5) << "Spawning send worker " << i << " for locale " << core_partner_locales_[i];
                                  send_worker( core_partner_locales_[i] );
                                });
        }
#endif

      // spawn receive workers
#ifndef LEGACY_SEND
        for( int i = 0; i < FLAGS_rdma_workers_per_core; ++i ) {
          Grappa::spawn_worker( [this] { 
                                  receive_worker();
                                });
        }
#endif

      // spawn flusher
#ifndef LEGACY_SEND
        Grappa::spawn_worker( [this] {
                                idle_flusher();
                              });
#endif

      // //
      // // precache buffers
      // //

      // // for now, give each core 1 buffer/token
      // size_t expected_buffers = core_partner_locale_count_; // * remote_buffer_pool_size;

      // for( int i = 0; i < core_partner_locale_count_; ++i ) {
      //   Locale locale = core_partner_locales_[i];
      //   Core dest = dest_core_for_locale_[ locale ];
      //   Core mylocale_core = Grappa::mylocale() * Grappa::locale_cores();
      //   Core mycore = Grappa::mycore();

      //   DVLOG(3) << __PRETTY_FUNCTION__ << ": sending " << expected_buffers << " buffers to core " << dest << " on locale " << locale;

      //   for( int j = 0; j < remote_buffer_pool_size; ++j ) {
      //     RDMABuffer * b = global_rdma_aggregator.free_buffer_list_.try_pop();
      //     CHECK_NOTNULL( b );
          
      //     // (make heap message since we're not on a shared stack)
      //     auto request = Grappa::message( dest, [mycore, b] {
      //         auto p = global_rdma_aggregator.localeCoreData(mycore);
      //         DVLOG(3) << __PRETTY_FUNCTION__ << " initializing by pushing buffer " << b << " into " << p << " for " << mycore;
      //         p->remote_buffers_.push( b );
      //         rdma_buffers_inuse += remote_buffer_pool_size - p->remote_buffers_.count();
      //       });
      //     request.send_immediate();
      //   }
      // }

#endif
    }


    void RDMAAggregator::finish() {
#ifdef ENABLE_RDMA_AGGREGATOR
      global_communicator.barrier();
      if( global_communicator.locale_mycore == 0 ) {
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

  void RDMAAggregator::deserialize_buffer_am( void * buf, int size, CommunicatorContext * c ) {
      Grappa::impl::global_scheduler.set_no_switch_region( true );
      deaggregate_buffer( static_cast< char * >( buf ), size );
      Grappa::impl::global_scheduler.set_no_switch_region( false );
      c->reference_count = 0;
  }

  void RDMAAggregator::deserialize_first_am( void * buf, int size, CommunicatorContext * c ) {
      app_messages_deserialized++;
      Grappa::impl::global_scheduler.set_no_switch_region( true );
      Grappa::impl::MessageBase::deserialize_and_call( static_cast< char * >( buf ) );
      Grappa::impl::global_scheduler.set_no_switch_region( false );
      // done with buffer
      c->reference_count = 0;
    }

  void RDMAAggregator::enqueue_buffer_am( void * buf, int size, CommunicatorContext * c ) {
    ++rdma_enqueue_buffer_am;

    DVLOG(2) << "Receiving buffer with deserializer " << (void*) (*((int64_t*)c->buf));
    // correct for deserializer bytes
    RDMABuffer * b = reinterpret_cast< RDMABuffer * >( c->buf );

    __builtin_prefetch( b,    1, 0 ); // prefetch for read
    //__builtin_prefetch( b+64, 1, 0 ); // prefetch for read

    DVLOG(2) << __func__ << ": Receiving buffer of size " << size
             << " from " << b->get_source() << " to " << b->get_dest()
             << " at " << buf << " with ack " << b->get_ack()
             << " deserializer " << b->deserializer
             << " core0 count " << (b->get_counts())[0]
             << " core1 count " << (b->get_counts())[1]
             << "; enqueuing for deserialization";
    
    // save context so we can notify communicator when we're done with the buffer
    b->deserializer = (void*) c;

    // we don't decrement reference count here; receive_worker() will do that.
    global_rdma_aggregator.received_buffer_list_.push( b );
  }


void RDMAAggregator::draw_routing_graph() {
  if( FLAGS_route_graph_filename != "" ) {
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
}






    char * RDMAAggregator::aggregate_to_buffer( char * buffer, Grappa::impl::MessageBase ** message_ptr, size_t max, uint64_t * count_p ) {
      size_t size = 0;
      size_t count = 0;

      Grappa::impl::MessageBase * message = *message_ptr;
      DVLOG(5) << "Serializing messages from " << message;

      while( message ) {
        //DVLOG(5) << __func__ << ": Serializing message " << message << " to " << message->destination_ << ": " << message->typestr();

        // issue prefetch for next message
        char * pf = reinterpret_cast< char* >( message->prefetch_ );
        __builtin_prefetch( pf +  0, 1, prefetch_type );
        //__builtin_prefetch( buffer, 1, prefetch_type );
#ifdef DOUBLE_LINE_PREFETCH
        __builtin_prefetch( pf + 64, 1, prefetch_type );
#endif
        // add message to buffer
        char * new_buffer = message->serialize_to( buffer, max - size);

        if( new_buffer == buffer ) { // if it was too big
          DVLOG(5) << __func__ << ": Message too big: aborting serialization";
          break;                     // quit
        } else {
          // DVLOG(3) << __func__ << ": Serialized message " << message
          //          << " next " << message->next_
          //          << " prefetch " << message->prefetch_ 
          //          << " size " << new_buffer - buffer;

          // track total size
          count++;
          size += new_buffer - buffer;
          //DCHECK_EQ( new_buffer - buffer, message->serialized_size() );

          app_bytes_serialized += new_buffer - buffer;
#ifdef DEBUG
          app_bytes_sent_histogram = new_buffer - buffer;
#endif

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

      app_messages_serialized += count;
      //app_bytes_serialized += size;
      if( count_p != NULL ) (*count_p) += count;

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
      Grappa::impl::global_scheduler.assign_time_to_networking();
      ++workers_send_blocked;
      Grappa::wait( &(locale_core->send_cv_) );
      --workers_send_blocked;

      active_send_workers_++;
      
      //CHECK_EQ( disable_everything_, false ) << "Whoops! Why are we sending when disabled?";
      if( disable_everything_ ) LOG(WARNING) << "Sending while disabled...";

      // send to locale
      send_locale( locale );

      // record when we last sent
      // TODO: should this go earlier? probably not.
      // TODO: should we tick here?
      Grappa::tick();
      locale_core->last_sent_ = Grappa::timestamp();

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
        Grappa::impl::global_scheduler.assign_time_to_networking();
        ++workers_receive_blocked;
        buf = received_buffer_list_.block_until_pop();
        --workers_receive_blocked;
        CHECK_NOTNULL( buf );
      }
      
      rdma_receive_start++;
      active_receive_workers_++;

      __builtin_prefetch( buf,    1, 0 ); // prefetch for read
      __builtin_prefetch( buf+64, 1, 0 ); // prefetch for read
      
      DVLOG(2) << __PRETTY_FUNCTION__ << "/" << Grappa::impl::global_scheduler.get_current_thread() 
               << " processing buffer " << buf << " context " << buf->deserializer
               << " serial " << reinterpret_cast<int64_t>( buf->get_ack() );

      // what core is buffer from?
      Core c = buf->get_source();

      if( buf->get_ack() == reinterpret_cast<RDMABuffer*>(-1) ) {
        // process buffer through non-temporal path
        receive_nt_buffer( buf );
      } else {
        // process buffer through normal path
        receive_buffer( buf );
      }

      // // once we're done, send ack to give permission to send again,
      // // unless buffer is from the local core (for debugging)
      // if( c != Grappa::mycore() ) {
      //   Core mylocale_core = Grappa::mylocale() * Grappa::locale_cores();
      //   Core mycore = Grappa::mycore();
      //   DVLOG(3) << __PRETTY_FUNCTION__ << "/" << Grappa::impl::global_scheduler.get_current_thread() << " finished with " << buf
      //            << "; acking with buffer " << buf << " for core " << mycore << " on core " << c;
      //   auto request = Grappa::message( c, [mycore, buf] {
      //       auto p = global_rdma_aggregator.localeCoreData(mycore);
      //       DVLOG(3) << __PRETTY_FUNCTION__ << " acking by pushing buffer " << buf << " into " << p << " for " << mycore;
      //       p->remote_buffers_.push( buf );
      //       rdma_buffers_inuse += remote_buffer_pool_size - p->remote_buffers_.count();
      //     });
      //   request.send_immediate();
      // } else {
      //   DVLOG(3) << __PRETTY_FUNCTION__ << "/" << Grappa::impl::global_scheduler.get_current_thread() << " finished with " << buf
      //            << "; pushing on free list";
      //   free_buffer_list_.push( buf );
      // }

      DVLOG(2) << __PRETTY_FUNCTION__ << "/" << Grappa::impl::global_scheduler.get_current_thread() 
               << " done processing buffer " << buf << " context " << buf->deserializer
               << " serial " << reinterpret_cast<int64_t>( buf->get_ack() );


      // return buffer to communicator
      CommunicatorContext * context = (CommunicatorContext*) buf->deserializer;
      context->reference_count = 0;
      global_communicator.repost_receive_buffers();
      
      active_receive_workers_--;
      rdma_receive_end++;
    }
  }















  void RDMAAggregator::receive_nt_buffer( RDMABuffer * buf ) {
    int64_t * size = reinterpret_cast<int64_t*>( buf->get_counts() );
    char * b = reinterpret_cast<char*>( size + 1 );
    Grappa::impl::global_scheduler.set_no_switch_region( true );
    deaggregate_nt_buffer( b, *size );
    Grappa::impl::global_scheduler.set_no_switch_region( false );
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
        uint32_t size;
        //uint64_t sequence_number;
        //Grappa::impl::RDMABuffer * buf_base;
        void operator()() {
          DVLOG(5) << __PRETTY_FUNCTION__ // << "/" << sequence_number
                   << ": received " << size << "-byte buffer slice at " << (void*) buf << " to deaggregate";

          global_rdma_aggregator.deaggregate_buffer( buf, size );

          DVLOG(5) << __PRETTY_FUNCTION__ // << "/" << sequence_number 
                   << ": done deaggregating " << size << "-byte buffer slice at " << (void*) buf;
        }
      };

      // deaggregate messages to send
      Grappa::Message< ReceiveBuffer > msgs[ MAX_CORES_PER_LOCALE ];
      
      // index array from buffer
      uint32_t * counts = buf->get_counts();
      char * current_buf = buf->get_payload();
      char * my_buf = NULL;
      int outstanding = 0;


      // fill and send deaggregate messages
      for( Core locale_core = 0; locale_core < Grappa::locale_cores(); ++locale_core ) {
        DVLOG(5) << __func__ << "/" << (void*) sequence_number << "/" << (void*) buf
                 << ": found " << counts[ locale_core ] << " bytes for core " << locale_core << " at " << (void*) current_buf;

        if( counts[locale_core] > 0 ) {

          msgs[locale_core]->buf = current_buf;
          msgs[locale_core]->size = counts[locale_core];
          //msgs[locale_core]->sequence_number = sequence_number;
          //msgs[locale_core]->buf_base = buf;
          
          if( locale_core != Grappa::locale_mycore() ) {
            DVLOG(5) << __func__ << "/" << (void*) sequence_number << "/" << (void*) buf 
                     << " enqueuing " << counts[locale_core] << " bytes to locale_core " << locale_core << " core " << locale_core + Grappa::mylocale() * Grappa::locale_cores() << " my locale_core " << Grappa::locale_mycore() << " my core " << Grappa::mycore();
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

    DVLOG(5) << __func__ << "/" << (void*) sequence_number << "/" << (void*) buf
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
        Grappa::impl::MessageBase * pre_mb = get_pointer( &dest->prefetch_queue_[i] );
        char * pf = reinterpret_cast< char* >( pre_mb );
        DVLOG(5) << __PRETTY_FUNCTION__ << ": Prefetching " << pre_mb;
        __builtin_prefetch( pf +  0, 1, prefetch_type ); // prefetch for read
#ifdef DOUBLE_LINE_PREFETCH
        __builtin_prefetch( pf + 64, 1, prefetch_type ); // prefetch for read
#endif
      }

      Grappa::impl::global_scheduler.set_no_switch_region( true );
      while( messages_to_send != NULL ) {
        // DVLOG(4) << __func__ << "/" << Grappa::impl::global_scheduler.get_current_thread() << ": Delivered message " << messages_to_send 
        //          << " with is_delivered_=" << messages_to_send->is_delivered_ 
        //          << ": " << messages_to_send->typestr();
        MessageBase * next = messages_to_send->next_;
        DVLOG(5) << __PRETTY_FUNCTION__ << ": Processing " << messages_to_send << ", prefetching " << messages_to_send->prefetch_;
        char * pf = reinterpret_cast< char* >( messages_to_send->prefetch_ );
        __builtin_prefetch( pf +  0, 1, prefetch_type );
#ifdef DOUBLE_LINE_PREFETCH
        __builtin_prefetch( pf + 64, 1, prefetch_type );
#endif
        // DCHECK_EQ( messages_to_send->destination_, Grappa::mycore() );


        delivered_bytes += messages_to_send->size();
        messages_to_send->deliver_locally();
        delivered_count++;

        messages_to_send = next;
      }
      Grappa::impl::global_scheduler.set_no_switch_region( false );

      DVLOG(5) << __PRETTY_FUNCTION__ << ": Done.";

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
      char * pf = reinterpret_cast< char* >( pre );
      __builtin_prefetch( pf +  0, 1, prefetch_type );
#ifdef DOUBLE_LINE_PREFETCH
      __builtin_prefetch( pf + 64, 1, prefetch_type );
#endif
      coreData( core, locale_source )->prefetch_queue_[i].raw_ = 0;    // clear out for next time around
    }
  }

  inline void RDMAAggregator::issue_initial_prefetches( CoreData * cd ) {
    for( int i = 0; i < prefetch_dist; ++i ) {
      Grappa::impl::MessageBase * pre = get_pointer( &(cd->prefetch_queue_[i])  );
      char * pf = reinterpret_cast< char* >( pre );
      __builtin_prefetch( pf +  0, 1, prefetch_type );
#ifdef DOUBLE_LINE_PREFETCH
      __builtin_prefetch( pf + 64, 1, prefetch_type );
#endif
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
            char * pf = reinterpret_cast< char* >( next );
            __builtin_prefetch( pf +  0, 1, prefetch_type );
#ifdef DOUBLE_LINE_PREFETCH
            __builtin_prefetch( pf + 64, 1, prefetch_type );
#endif
          }
        } while( 0 == cd->messages_.raw_ && !done );
      }
      
      return cd;
    }
  };


  void RDMAAggregator::send_locale( Locale locale ) {
    rdma_send_start++;
    active_send_workers_++;
    ++workers_active_send;
    static uint64_t sequence_number = Grappa::mycore();

    int buffers_used_for_send = 0;
    int64_t bytes_sent = 0;

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
      Grappa::impl::global_scheduler.assign_time_to_networking();
      // ++workers_block_remote_buffer;
      // RDMABuffer * dest_buf = localeCoreData( dest_core )->remote_buffers_.block_until_pop();
      // --workers_block_remote_buffer;
      // CHECK_NOTNULL( dest_buf );

      rdma_buffers_inuse += remote_buffer_pool_size - localeCoreData( dest_core )->remote_buffers_.count();

      // now, grab a temporary buffer for local serialization
      Grappa::impl::global_scheduler.assign_time_to_networking();
      ++workers_block_local_buffer;
      RDMABuffer * b = free_buffer_list_.block_until_pop();
      --workers_block_local_buffer;

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
          static_assert(sizeof(size_t) == sizeof(uint64_t), "must be 64-bit");
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

      // if( aggregated_size >= 0 ) {
      DVLOG(4) << __func__ << "/" << sequence_number << ": " 
               << "Ready to send buffer " << b << " size " << aggregated_size
               << " to locale " << locale << " core " << dest_core;
      // } else {
      //   DVLOG(4) << __func__ << "/" << sequence_number << ": " 
      //          << "Nothing to send in buffer " << b << " size " << aggregated_size
      //          << " to locale " << locale << " core " << dest_core;
      //   break;
      // }

      DVLOG(3) << __func__ << "/" << sequence_number << ": " << "Maybe sending buffer " << b << " sequence " << sequence_number
               << " size " << aggregated_size
               << " core0 count " << (b->get_counts())[0]
               << " core1 count " << (b->get_counts())[1]
               << " to locale " << locale
               << " core " << dest_core;

      // for debugging
      b->set_next( reinterpret_cast<RDMABuffer*>( aggregated_size ) );

      b->set_source( Grappa::mycore() );
      b->set_ack( b );
      
      if( aggregated_size > 0 ) {
        // we have a buffer. send.
        //global_communicator.send( dest_core, enqueue_buffer_handle_, b->get_base(), aggregated_size + b->get_base_size(), dest_buf );
        b->deserializer = (void*) &enqueue_buffer_am;
        b->context.callback = [] ( CommunicatorContext * c, int source, int tag, int received_size ) {
          DVLOG(4) << "Got callback for " << c;
          global_rdma_aggregator.free_buffer_list_.push( (RDMABuffer*) c->buf );
        };
        b->context.buf = (void*) b;
        b->context.size = b->get_max_size();
        b->context.reference_count = 1;
        DVLOG(3) << "Sending " << &b->context << " with deserializer " << (void*) &enqueue_buffer_am;
        global_communicator.post_external_send( &b->context, dest_core,
                                                aggregated_size + b->get_base_size() );

        rdma_message_bytes += aggregated_size + b->get_base_size();
        bytes_sent += aggregated_size + b->get_base_size();

        if( buffers_used_for_send == 0 ) rdma_first_buffer_bytes += aggregated_size + b->get_base_size();
        buffers_used_for_send++;

        DVLOG(4) << __func__ << "/" << sequence_number 
                 << ": Sent buffer of size " << aggregated_size 
                 << " through RDMA to " << dest_core
                 << " with " << messages_to_send 
                 << " still to send, then check core " << current_dest_core << " of " << max_core;
      } else {
        DVLOG(3) << __func__ << "/" << sequence_number 
                 << ": No message to send; returning buffer " << b;
        free_buffer_list_.push( b );
      }

      sequence_number += Grappa::cores();
    }

    if( buffers_used_for_send > 0 ) rdma_buffers_used_for_send += buffers_used_for_send;

    CHECK_NULL( messages_to_send );

    rdma_bytes_sent_histogram = bytes_sent;

    active_send_workers_--;
    rdma_send_end++;
    --workers_active_send;
  }


  void RDMAAggregator::send_nt_buffer( Core dest, NTBuffer * buf ) {
    nt_mru_.reset(dest);
    auto buftuple = buf->take_buffer();
    auto b = reinterpret_cast<RDMABuffer*>( std::get<0>(buftuple) );
    auto size = std::get<1>(buftuple);
    DVLOG(3) << "Sending NT buffer " << buf << " with data buffer " << b << " of size " << size;

    // TODO: hack to carry size for now
    int64_t * size_p = reinterpret_cast<int64_t*>( b->get_counts() );
    *size_p = size - 4 * sizeof(uint64_t); // TODO: make header smarter
    
    b->set_dest( dest );
    b->set_next( reinterpret_cast<RDMABuffer*>( size ) );
    b->set_source( Grappa::mycore() );
    b->set_ack( reinterpret_cast<RDMABuffer*>(-1) ); // TODO: magic number for now
    b->deserializer = (void*) &enqueue_buffer_am;
    b->context.callback = [] ( CommunicatorContext * c, int source, int tag, int received_size ) {
      DVLOG(4) << "Got callback for " << c;
      free( c->buf );
    };
    b->context.buf = (void*) b;
    b->context.size = b->get_max_size();
    b->context.reference_count = 1;
    DVLOG(3) << "Sending " << &b->context << " with deserializer " << (void*) &enqueue_buffer_am;
    global_communicator.post_external_send( &b->context, dest, size );
    aggregated_nt_message_bytes += size;
    
    // give ourselves a chance to receive something
    if( !global_scheduler.in_no_switch_region() ) {
      Grappa::impl::global_scheduler.thread_yield();
    }
  }

}
}
