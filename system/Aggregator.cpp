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

#include <gflags/gflags.h>

#include "Aggregator.hpp"
#include "Grappa.hpp"
#include <csignal>

#include "PerformanceTools.hpp"

// command line options for Aggregator
DEFINE_int64( aggregator_autoflush_ticks, 50000, "number of ticks to wait before autoflushing aggregated active messages");
DEFINE_int64( aggregator_max_flush, 0, "flush no more than this many buffers per poll (0 for unlimited)");
DEFINE_bool( aggregator_enable, true, "should we aggregate packets or just send them?");
DEFINE_bool( flush_on_idle, true, "flush all aggregated messages there's nothing better to do");


GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_0_to_255_bytes, 0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_256_to_511_bytes, 0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_512_to_767_bytes, 0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_768_to_1023_bytes, 0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_1024_to_1279_bytes,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_1280_to_1535_bytes,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_1536_to_1791_bytes,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_1792_to_2047_bytes,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_2048_to_2303_bytes,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_2304_to_2559_bytes,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_2560_to_2815_bytes,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_2816_to_3071_bytes,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_3072_to_3327_bytes,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_3328_to_3583_bytes,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_3584_to_3839_bytes,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>,aggregator_3840_to_4095_bytes,0);


GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_messages_aggregated_,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_bytes_aggregated_,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_messages_deaggregated_, 0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_bytes_deaggregated_,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_messages_forwarded_,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_bytes_forwarded_,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_newest_wait_ticks_,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_oldest_wait_ticks_,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_polls_,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_flushes_,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_multiflushes_,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_timeouts_,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_idle_flushes,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_capacity_flushes_,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_idle_poll_,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_idle_poll_useful_,0);
GRAPPA_DEFINE_METRIC( SimpleMetric<uint64_t>, aggregator_bundles_received_,0);
GRAPPA_DEFINE_METRIC( SummarizingMetric<uint64_t>, aggregator_bundle_bytes_received_, 0);

/* Not currently useful because Communicator.cpp has one *//*
GRAPPA_DEFINE_METRIC( SimpleMetric<double>, aggregator_start_time, []() {
    // initialization value
    return Grappa::walltime();
    });
GRAPPA_DEFINE_METRIC( CallbackMetric<double>, aggregator_end_time, []() {
    // sampling value
    return Grappa::walltime();
    });
    */

/// global Aggregator instance
Aggregator global_aggregator;


#ifdef STL_DEBUG_ALLOCATOR
DEFINE_int64( aggregator_access_control_signal, SIGUSR2, "signal used to toggle aggregator queue access control");
bool aggregator_access_control_active = false;
static void aggregator_toggle_access_control_sighandler( int signum ) {
  aggregator_access_control_active = ~aggregator_access_control_active;
}

#endif

/// Construct Aggregator
Aggregator::Aggregator( ) 
  : max_nodes_( -1 )
  , buffers_( )
  , previous_timestamp_( 0L )
  , least_recently_sent_( )
  , aggregator_deaggregate_am_handle_( -1 )
  , route_map_( )
#ifdef VTRACE_FULL
  , tag_( -1 )
  , vt_agg_commid_( VT_COMM_DEF( "Aggregator" ) )
#endif
  , stats()
{ 
#ifdef STL_DEBUG_ALLOCATOR
  struct sigaction access_control_toggle_sa;
  sigemptyset( &access_control_toggle_sa.sa_mask );
  access_control_toggle_sa.sa_flags = 0;
  access_control_toggle_sa.sa_handler = &aggregator_toggle_access_control_sighandler;
  CHECK_EQ( 0, sigaction( FLAGS_aggregator_access_control_signal, &access_control_toggle_sa, 0 ) ) 
    << "Aggregator access control signal handler installation failed.";
  if( aggregator_access_control_active ) STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadOnly);
#endif
}

void Aggregator_deaggregate_am( void * buf, size_t size );

/// Initialize aggregator
void Aggregator::init() {
  max_nodes_ = global_communicator.cores;
  least_recently_sent_.resize( global_communicator.cores );

  buffers_.resize( max_nodes_ - buffers_.size() );
  route_map_.resize( max_nodes_ - route_map_.size() );
  // initialize route map
  for( Core i = 0; i < max_nodes_; ++i ) {
    route_map_[i] = i;
  }
#ifdef VTRACE_FULL
  tag_ = global_communicator.mycore;
#endif
}

/// Tear down aggregator
Aggregator::~Aggregator() {
#ifdef STL_DEBUG_ALLOCATOR
  STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadWrite);
#endif
}

/// After deaggregate handler is called to buffer an aggregated
/// message bundle, this method unpacks the bundle and executes the
/// Grappa-level active message handlers.
void Aggregator::deaggregate( ) {
  GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
#ifdef VTRACE_FULL
  VT_TRACER("deaggregate");
#endif
  StateTimer::enterState_deaggregation();
  while( !received_AM_queue_.empty() ) {
    DVLOG(5) << "deaggregating";
    // TODO: too much copying
    ReceivedAM amp = received_AM_queue_.front();

#ifdef STL_DEBUG_ALLOCATOR
    if( aggregator_access_control_active ) STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadWrite);
#endif
    received_AM_queue_.pop();
#ifdef STL_DEBUG_ALLOCATOR
    if( aggregator_access_control_active ) STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadOnly);
#endif

    DVLOG(5) << "deaggregating message of size " << amp.size_;
    uintptr_t msg_base = reinterpret_cast< uintptr_t >( amp.buf_ );
    for( unsigned int i = 0; i < amp.size_; ) {
      AggregatorGenericCallHeader * header = reinterpret_cast< AggregatorGenericCallHeader * >( msg_base );
      AggregatorAMHandler fp = reinterpret_cast< AggregatorAMHandler >( header->function_pointer );
      void * args = reinterpret_cast< void * >( msg_base + 
                                                sizeof( AggregatorGenericCallHeader ) );
      void * payload = reinterpret_cast< void * >( msg_base +
                                                   sizeof( AggregatorGenericCallHeader ) +
                                                   header->args_size );
      if( header->destination == Grappa::mycore() ) { // for us?
          
	stats.record_deaggregation( sizeof( AggregatorGenericCallHeader ) + header->args_size + header->payload_size );
          // trace fine-grain communication
#ifdef GRAPPA_TRACE
          if (FLAGS_record_grappa_events) {
              // TODO: good candidate for TAU_CONTEXT_EVENT
              int fn_p_tag = aggregator_trace_tag( fp );
              TAU_TRACE_RECVMSG(fn_p_tag, header->source, header->args_size + header->payload_size );
          }
#endif

#ifdef VTRACE_FULL
	  {
	    VT_RECV( vt_agg_commid_, header->tag, sizeof( AggregatorGenericCallHeader ) + header->args_size + header->payload_size );
	  }
#endif


        DVLOG(5) << "calling " << *header 
                << " with args " << args
                << " and payload " << payload;
      
        {   
            GRAPPA_PROFILE( deag_func_timer, "deaggregate execution", "", GRAPPA_USERAM_GROUP );
            fp( args, header->args_size, payload, header->payload_size ); // execute
        } 
      } else { // not for us, so forward towards destination
        DVLOG(5) << "forwarding " << *header
                << " with args " << args
                << " and payload " << payload;
	stats.record_forward( sizeof( AggregatorGenericCallHeader ) + header->args_size + header->payload_size );
        aggregate( header->destination, fp, args, header->args_size, payload, header->payload_size );
      }
      i += sizeof( AggregatorGenericCallHeader ) + header->args_size + header->payload_size;
      msg_base += sizeof( AggregatorGenericCallHeader ) + header->args_size + header->payload_size;
    }
  }
}
  
/// clean up aggregator before destruction
void Aggregator::finish() {
#ifdef STL_DEBUG_ALLOCATOR
  LOG(INFO) << "Cleaning up access control....";
  STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadWrite);
#endif
}

/// Deaggration active message handler. This receives an
/// aggregated message bundle and buffers it for later deaggregation.
void Aggregator_deaggregate_am( void * buf, size_t size ) {
  GRAPPA_FUNCTION_PROFILE( GRAPPA_COMM_GROUP );
#ifdef VTRACE_FULL
  VT_TRACER("deaggregate AM");
#endif

  DVLOG(5) << "received message with size " << size;
  // TODO: too much copying
  Aggregator::ReceivedAM am( size, buf );
  global_aggregator.stats.record_receive_bundle( size );
#ifdef STL_DEBUG_ALLOCATOR
  if( aggregator_access_control_active ) STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadWrite);
#endif
  global_aggregator.received_AM_queue_.push( am );
#ifdef STL_DEBUG_ALLOCATOR
  if( aggregator_access_control_active ) STLMemDebug::BaseAllocator::getMemMgr().setAccessMode(STLMemDebug::memReadOnly);
#endif

}

/// proxy call to make it easier to integrate with scheduler
bool idle_flush_aggregator() {
  return global_aggregator.idle_flush_poll();
}

/// proxy call to make it easier to integrate with scheduler
size_t Grappa_sizeof_header() {
  return sizeof( AggregatorGenericCallHeader );
}


/* metrics */
AggregatorMetrics::AggregatorMetrics()
    : histogram_()
  {
    histogram_[0] = &aggregator_0_to_255_bytes;
    histogram_[1] = &aggregator_256_to_511_bytes;
    histogram_[2] = &aggregator_512_to_767_bytes;
    histogram_[3] = &aggregator_768_to_1023_bytes;
    histogram_[4] = &aggregator_1024_to_1279_bytes;
    histogram_[5] = &aggregator_1280_to_1535_bytes;
    histogram_[6] = &aggregator_1536_to_1791_bytes;
    histogram_[7] = &aggregator_1792_to_2047_bytes;
    histogram_[8] = &aggregator_2048_to_2303_bytes;
    histogram_[9] = &aggregator_2304_to_2559_bytes;
    histogram_[10] = &aggregator_2560_to_2815_bytes;
    histogram_[11] = &aggregator_2816_to_3071_bytes;
    histogram_[12] = &aggregator_3072_to_3327_bytes;
    histogram_[13] = &aggregator_3328_to_3583_bytes;
    histogram_[14] = &aggregator_3584_to_3839_bytes;
    histogram_[15] = &aggregator_3840_to_4095_bytes;
  }
  


void AggregatorMetrics::record_poll() {
  aggregator_polls_++;
}

void AggregatorMetrics::record_flush( Grappa::Timestamp oldest_ts, Grappa::Timestamp newest_ts ) {
  Grappa::Timestamp ts = Grappa::timestamp();
  aggregator_oldest_wait_ticks_ += ts - oldest_ts;
  aggregator_newest_wait_ticks_ += ts - newest_ts;
  aggregator_flushes_++;
}

void AggregatorMetrics::record_idle_flush() {
  aggregator_idle_flushes++;
}

void AggregatorMetrics::record_multiflush() {
  aggregator_multiflushes_++;
}

void AggregatorMetrics::record_timeout() {
  aggregator_timeouts_++;
}

void AggregatorMetrics::record_idle_poll( bool useful ) {
  if ( useful ) aggregator_idle_poll_useful_++;
  else aggregator_idle_poll_++;
}

void AggregatorMetrics::record_capacity_flush() {
  aggregator_capacity_flushes_++;
}
  
void AggregatorMetrics::record_forward( size_t bytes ) {
  ++aggregator_messages_forwarded_;
  aggregator_bytes_forwarded_ += bytes;
}

void AggregatorMetrics::record_receive_bundle( size_t bytes ) {
  ++aggregator_bundles_received_;
  aggregator_bundle_bytes_received_+= bytes ;
} 
