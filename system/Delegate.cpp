
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include "Delegate.hpp"
#include "Timestamp.hpp"
#include "common.hpp"

#include <cassert>
#include <numeric>
#include <limits>

#include <gflags/gflags.h>
#include <glog/logging.h>

/// Descriptor for delegate operation.
/// Used to hold state while waiting for reply.
struct memory_descriptor {
  Thread * t;
  GlobalAddress<int64_t> address;
  int64_t data;
  bool done;
  int64_t start_time;
  int64_t network_time;
};

/////////////////////////////////////////////////////////////////////////////

/// Wait for a delegate operation reply
static inline void Delegate_wait( memory_descriptor * md ) {
  if( !md->done ) {
    md->start_time = Grappa_get_timestamp();
    while( !md->done ) {
      md->t = CURRENT_THREAD;
      Grappa_suspend();
      md->t = NULL;
    }
    delegate_stats.record_wakeup_latency( md->start_time, md->network_time );
  } else {
    md->start_time = 0;
  }
}

/// Wake a thread waiting on a delegate operation
static inline void Delegate_wakeup( memory_descriptor * md ) {
  if( md->t != NULL ) {
    Grappa_wake( md->t );
  }
  if( md->start_time != 0 ) {
    md->network_time = Grappa_get_timestamp();
    delegate_stats.record_network_latency( md->start_time );
  }
}

/////////////////////////////////////////////////////////////////////////////

/// Args for delegate word write reply
struct memory_write_reply_args {
  GlobalAddress<memory_descriptor> descriptor;
};

/// Handler for delegate word write reply
static void memory_write_reply_am( memory_write_reply_args * args, size_t size, void * payload, size_t payload_size ) {
  (args->descriptor.pointer())->done = true;
  Delegate_wakeup( args->descriptor.pointer() );
}

/// Args for delegate word write request
struct memory_write_request_args {
  GlobalAddress<memory_descriptor> descriptor;
  GlobalAddress<int64_t> address;
};

/// Handler for delegate word write request
static void memory_write_request_am( memory_write_request_args * args, size_t size, void * payload, size_t payload_size ) {
  delegate_stats.count_op_am();
  delegate_stats.count_word_write_am();

  DCHECK( (int64_t)args->address.pointer() > 0x1000 )<< "read request:"
						     << "\n address="<<args->address
						     << "\n descriptor="<<args->descriptor;
  DCHECK_EQ( payload_size, sizeof(int64_t) );
  int64_t payload_int = *(static_cast<int64_t*>(payload));
  VLOG(3) << "payload("<<(void*)payload<<")="<<payload_int<<"\n    pointer="<<(void*)args->address.pointer();
  *(args->address.pointer()) = payload_int;
  memory_write_reply_args reply_args;
  reply_args.descriptor = args->descriptor;
  if( args->descriptor.node() == Grappa_mynode() ) {
    memory_write_reply_am( &reply_args, sizeof( reply_args ), NULL, 0 );
  } else {
    Grappa_call_on( args->descriptor.node(), &memory_write_reply_am, &reply_args );
  }
}

/// Delegate word write
void Grappa_delegate_write_word( GlobalAddress<int64_t> address, int64_t data ) {
  delegate_stats.count_op();
  delegate_stats.count_word_write();

  memory_descriptor md;
  md.address = address;
  md.data = data;
  md.done = false;
  md.t = NULL;
  md.start_time = 0;
  md.network_time = 0;
  memory_write_request_args args;
  args.descriptor = make_global(&md);
  args.address = address;
  if( address.node() == Grappa_mynode() ) {
    memory_write_request_am( &args, sizeof(args), 
			     &data, sizeof(data) );
  } else {
    Grappa_call_on( address.node(), &memory_write_request_am, 
		     &args, sizeof(args), 
		     &data, sizeof(data) );
  }
  Delegate_wait( &md );
}

/////////////////////////////////////////////////////////////////////////////

/// Args for delegate word read reply
struct memory_read_reply_args {
  GlobalAddress<memory_descriptor> descriptor;
};

/// Handler for delegate word read reply
static void memory_read_reply_am( memory_read_reply_args * args, size_t size, void * payload, size_t payload_size ) {
  DCHECK_EQ( payload_size, sizeof(int64_t ) );
  args->descriptor.pointer()->data = *(static_cast<int64_t*>(payload));
  args->descriptor.pointer()->done = true;
  Delegate_wakeup( args->descriptor.pointer() );
}

/// Args for delegate word read request
struct memory_read_request_args {
  GlobalAddress<memory_descriptor> descriptor;
  GlobalAddress<int64_t> address;
};

/// Handler for delegate word read request
static void memory_read_request_am( memory_read_request_args * args, size_t size, void * payload, size_t payload_size ) {
  delegate_stats.count_op_am();
  delegate_stats.count_word_read_am();

  DCHECK( (int64_t)args->address.pointer() > 0x1000 ) << "read request:"
						      << "\n address="<<args->address
						      << "\n descriptor="<<args->descriptor;
  int64_t data = *(args->address.pointer());
  memory_read_reply_args reply_args;
  reply_args.descriptor = args->descriptor;
  if( args->descriptor.node() == Grappa_mynode() ) {
    memory_read_reply_am( &reply_args, sizeof( reply_args ), &data, sizeof(data) );
  } else {
    Grappa_call_on( args->descriptor.node(), &memory_read_reply_am, 
		     &reply_args, sizeof(reply_args), 
		     &data, sizeof(data) );
  }
}

/// Delegate word read
int64_t Grappa_delegate_read_word( GlobalAddress<int64_t> address ) {
  delegate_stats.count_op();
  delegate_stats.count_word_read();

  memory_descriptor md;
  md.address = address;
  md.data = 0;
  md.done = false;
  md.t = NULL;
  md.start_time = 0;
  md.network_time = 0;
  memory_read_request_args args;
  args.descriptor = make_global(&md);
  args.address = address;
  if( address.node() == Grappa_mynode() ) {
    memory_read_request_am( &args, sizeof(args), 
			    NULL, 0 );
  } else {
    Grappa_call_on( address.node(), &memory_read_request_am, &args );
  }
  Delegate_wait( &md );
  return md.data;
}


/////////////////////////////////////////////////////////////////////////////


/// Args for delegate word fetch and add reply
struct memory_fetch_add_reply_args {
  GlobalAddress<memory_descriptor> descriptor;
};

/// Args for delegate word fetch and add request
struct memory_fetch_add_request_args {
  GlobalAddress<memory_descriptor> descriptor;
  GlobalAddress<int64_t> address;
};

/// Handler for delegate word fetch and add reply
static void memory_fetch_add_reply_am( memory_fetch_add_reply_args * args, size_t size, void * payload, size_t payload_size ) {
  DCHECK_EQ( payload_size, sizeof(int64_t) );
  args->descriptor.pointer()->data = *(static_cast<int64_t*>(payload));
  args->descriptor.pointer()->done = true;
  Delegate_wakeup( args->descriptor.pointer() );
}

/// Handler for delegate word fetch and add request.
/// runs on server side to fetch data 
static void memory_fetch_add_request_am( memory_fetch_add_request_args * args, size_t size, void * payload, size_t payload_size ) {
  delegate_stats.count_op_am();
  delegate_stats.count_word_fetch_add_am();

  DCHECK_EQ( payload_size, sizeof(int64_t) );
  int64_t data = *(args->address.pointer()); // fetch
  *(args->address.pointer()) += *(static_cast<int64_t*>(payload)); // increment
  memory_fetch_add_reply_args reply_args;
  reply_args.descriptor = args->descriptor;

  if( args->descriptor.node() == Grappa_mynode() ) {
    memory_fetch_add_reply_am( &reply_args, sizeof(reply_args), 
			       &data, sizeof(data) );
  } else {
    Grappa_call_on( args->descriptor.node(), &memory_fetch_add_reply_am, 
		     &reply_args, sizeof(reply_args), 
		     &data, sizeof(data) );
  }
}

/// Delegate word fetch and add
int64_t Grappa_delegate_fetch_and_add_word( GlobalAddress<int64_t> address, int64_t data ) {
  delegate_stats.count_op();
  delegate_stats.count_word_fetch_add();

  // set up descriptor
  memory_descriptor md;
  md.address = address;
  md.data = data;
  md.done = false;
  md.t = NULL;
  md.start_time = 0;
  md.network_time = 0;

  // set up args for request
  memory_fetch_add_request_args args;
  args.descriptor = make_global(&md);
  args.address = address;

  // make request
  if( address.node() == Grappa_mynode() ) {
    memory_fetch_add_request_am( &args, sizeof(args), 
				 &data, sizeof(data) );
  } else {
    Grappa_call_on( address.node(), &memory_fetch_add_request_am, 
		     &args, sizeof(args), 
		     &data, sizeof(data) );
  }

  // wait for response
  Delegate_wait( &md );
  
  return md.data;
}

/// Args for delegate word compare and swap request
struct cmp_swap_request_args {
  GlobalAddress<memory_descriptor> descriptor;
  GlobalAddress<int64_t> address;
  int64_t newval;
  int64_t cmpval;
};

/// Args for delegate word compare and swap reply
struct cmp_swap_reply_args {
  GlobalAddress<memory_descriptor> descriptor;
};

/// Handler for delegate word compare and swap reply
static void cmp_swap_reply_am(cmp_swap_reply_args * args, size_t size, void * payload, size_t payload_size) {
  DCHECK_EQ( payload_size, sizeof(int64_t) );
  args->descriptor.pointer()->data = *(static_cast<int64_t*>(payload));
  args->descriptor.pointer()->done = true;
  Delegate_wakeup( args->descriptor.pointer() );
}

/// Handler for delegate word compare and swap request
static void cmp_swap_request_am(cmp_swap_request_args * args, size_t sz, void * p, size_t psz) {
  delegate_stats.count_op_am();
  delegate_stats.count_word_compare_swap_am();

  int64_t data = *(args->address.pointer());
  int64_t swapped = false;
  if (data == args->cmpval) { // compare
    *(args->address.pointer()) = args->newval; // swap
    swapped = true;
  }
  
  cmp_swap_reply_args reply_args;
  reply_args.descriptor = args->descriptor;
  
  if( args->descriptor.node() == Grappa_mynode() ) {
    cmp_swap_reply_am( &reply_args, sizeof(reply_args), 
		       &swapped, sizeof(swapped) );
  } else {
    Grappa_call_on(args->descriptor.node(), &cmp_swap_reply_am, &reply_args, sizeof(reply_args), &swapped, sizeof(swapped));
  }
}

/// Delegate word compare and swap
bool Grappa_delegate_compare_and_swap_word(GlobalAddress<int64_t> address, int64_t cmpval, int64_t newval) {
  delegate_stats.count_op();
  delegate_stats.count_word_compare_swap();

  // set up descriptor
  memory_descriptor md;
  md.address = address;
  md.done = false;
  md.t = NULL;
  md.start_time = 0;
  md.network_time = 0;

  // set up args for request
  cmp_swap_request_args args;
  args.descriptor = make_global(&md);
  args.address = address;
  args.cmpval = cmpval;
  args.newval = newval;
  
  // make request
  if( address.node() == Grappa_mynode() ) {
    cmp_swap_request_am( &args, sizeof(args), 
			 NULL, 0 );
  } else {
    Grappa_call_on( address.node(), &cmp_swap_request_am, 
		     &args, sizeof(args), 
		     NULL, 0);
  }

  // wait for response
  Delegate_wait( &md );
  
  return (md.data != 0);
}



DelegateStatistics delegate_stats;

DelegateStatistics::DelegateStatistics()
#ifdef VTRACE_SAMPLED
  : delegate_grp_vt( VT_COUNT_GROUP_DEF( "Delegate" ) )
  , ops_ev_vt( VT_COUNT_DEF( "Delegate operations", "operations", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , word_writes_ev_vt( VT_COUNT_DEF( "Delegate word writes", "writes", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , word_reads_ev_vt( VT_COUNT_DEF( "Delegate word reads", "reads", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , T_reads_ev_vt( VT_COUNT_DEF( "Delegate template reads", "reads", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , word_fetch_adds_ev_vt( VT_COUNT_DEF( "Delegate word fetch and adds", "fads", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , word_compare_swaps_ev_vt( VT_COUNT_DEF( "Delegate word compare and swaps", "casses", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , generic_ops_ev_vt( VT_COUNT_DEF( "Delegate generic operations", "operations", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , op_ams_ev_vt( VT_COUNT_DEF( "Delegate operation active messages", "operations", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , word_write_ams_ev_vt( VT_COUNT_DEF( "Delegate word write active messages", "writes", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , word_read_ams_ev_vt( VT_COUNT_DEF( "Delegate word read active messages", "reads", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , T_read_ams_ev_vt( VT_COUNT_DEF( "Delegate template read active messages", "reads", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , word_fetch_add_ams_ev_vt( VT_COUNT_DEF( "Delegate word fetch and add active messages", "fads", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , word_compare_swap_ams_ev_vt( VT_COUNT_DEF( "Delegate word compare and swap active messages", "casses", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , generic_op_ams_ev_vt( VT_COUNT_DEF( "Delegate generic operation active messages", "operations", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , ops_blocked_ev_vt( VT_COUNT_DEF( "Delegate blocked op count", "operations", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , ops_blocked_ticks_total_ev_vt( VT_COUNT_DEF( "Delegate total blocked ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , ops_network_ticks_total_ev_vt( VT_COUNT_DEF( "Delegate total network ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , ops_wakeup_ticks_total_ev_vt( VT_COUNT_DEF( "Delegate total wakeup ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , ops_blocked_ticks_max_ev_vt( VT_COUNT_DEF( "Delegate max blocked ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , ops_blocked_ticks_min_ev_vt( VT_COUNT_DEF( "Delegate min blocked ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , ops_network_ticks_max_ev_vt( VT_COUNT_DEF( "Delegate max network ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , ops_network_ticks_min_ev_vt( VT_COUNT_DEF( "Delegate min network ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , ops_wakeup_ticks_max_ev_vt( VT_COUNT_DEF( "Delegate max wakeup ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , ops_wakeup_ticks_min_ev_vt( VT_COUNT_DEF( "Delegate min wakeup ticks", "ticks", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , average_latency_ev_vt( VT_COUNT_DEF( "Delegate average latency", "ticks/s", VT_COUNT_TYPE_DOUBLE, delegate_grp_vt ) )
  , average_network_latency_ev_vt( VT_COUNT_DEF( "Delegate average network latency", "ticks/s", VT_COUNT_TYPE_DOUBLE, delegate_grp_vt ) )
  , average_wakeup_latency_ev_vt( VT_COUNT_DEF( "Delegate average wakeup latency", "ticks/s", VT_COUNT_TYPE_DOUBLE, delegate_grp_vt ) )
#endif
{
  reset();
}

void DelegateStatistics::reset() {
  ops = 0;
  word_writes = 0;
  word_reads = 0;
  T_reads = 0;
  word_fetch_adds = 0;
  word_compare_swaps = 0;
  generic_ops = 0;
  op_ams = 0;
  word_write_ams = 0;
  word_read_ams = 0;
  T_read_ams = 0;
  word_fetch_add_ams = 0;
  word_compare_swap_ams = 0;
  generic_op_ams = 0;
  ops_blocked = 0;
  ops_blocked_ticks_total = 0;
  ops_network_ticks_total = 0;
  ops_wakeup_ticks_total = 0;
  ops_blocked_ticks_max = std::numeric_limits<uint64_t>::min();
  ops_blocked_ticks_min = std::numeric_limits<uint64_t>::max();
  ops_network_ticks_max = std::numeric_limits<uint64_t>::min();
  ops_network_ticks_min = std::numeric_limits<uint64_t>::max();
  ops_wakeup_ticks_max = std::numeric_limits<uint64_t>::min();
  ops_wakeup_ticks_min = std::numeric_limits<uint64_t>::max();
}

void DelegateStatistics::dump( std::ostream& o = std::cout, const char * terminator = "" ) {
  o << "   \"DelegateStats\": { "
    << "\"ops\": " << ops << ", "
    << "\"word_writes\": " << word_writes  << ", "
    << "\"word_reads\": " << word_reads  << ", "
    << "\"T_reads\": " << T_reads  << ", "
    << "\"word_fetch_adds\": " << word_fetch_adds  << ", "
    << "\"word_compare_swaps\": " << word_compare_swaps  << ", "
    << "\"generic_ops\": " << generic_ops  << ", "
    << "\"op_ams\": " << op_ams << ", "
    << "\"word_write_ams\": " << word_write_ams  << ", "
    << "\"word_read_ams\": " << word_read_ams  << ", "
    << "\"T_read_ams\": " << T_read_ams  << ", "
    << "\"word_fetch_add_ams\": " << word_fetch_add_ams  << ", "
    << "\"word_compare_swap_ams\": " << word_compare_swap_ams  << ", "
    << "\"generic_op_ams\": " << generic_op_ams  << ", "
    << "\"ops_blocked\": " << ops_blocked  << ", "
    << "\"ops_blocked_ticks_total\": " << ops_blocked_ticks_total  << ", "
    << "\"ops_blocked_ticks_min\": " << ops_blocked_ticks_min  << ", "
    << "\"ops_blocked_ticks_max\": " << ops_blocked_ticks_max  << ", "
    << "\"average_latency\": " << nanless_double_ratio( ops_blocked_ticks_total, ops_blocked ) << ", "
    << "\"ops_network_ticks_total\": " << ops_network_ticks_total  << ", "
    << "\"ops_network_ticks_min\": " << ops_network_ticks_min  << ", "
    << "\"ops_network_ticks_max\": " << ops_network_ticks_max  << ", "
    << "\"average_network_latency\": " << nanless_double_ratio( ops_network_ticks_total, ops_blocked ) << ", "
    << "\"ops_wakeup_ticks_total\": " << ops_wakeup_ticks_total  << ", "
    << "\"ops_wakeup_ticks_min\": " << ops_wakeup_ticks_min  << ", "
    << "\"ops_wakeup_ticks_max\": " << ops_wakeup_ticks_max  << ", "
    << "\"average_wakeup_latency\": " << nanless_double_ratio( ops_wakeup_ticks_total, ops_blocked )
    << " }" << terminator << std::endl;
}

void DelegateStatistics::sample() {
  ;
}

void DelegateStatistics::profiling_sample() {
#ifdef VTRACE_SAMPLED
  VT_COUNT_UNSIGNED_VAL( ops_ev_vt, ops );
  VT_COUNT_UNSIGNED_VAL( word_writes_ev_vt, word_writes );
  VT_COUNT_UNSIGNED_VAL( word_reads_ev_vt, word_reads );
  VT_COUNT_UNSIGNED_VAL( T_reads_ev_vt, T_reads );
  VT_COUNT_UNSIGNED_VAL( word_fetch_adds_ev_vt, word_fetch_adds );
  VT_COUNT_UNSIGNED_VAL( word_compare_swaps_ev_vt, word_compare_swaps );
  VT_COUNT_UNSIGNED_VAL( generic_ops_ev_vt, generic_ops );
  VT_COUNT_UNSIGNED_VAL( op_ams_ev_vt, op_ams );
  VT_COUNT_UNSIGNED_VAL( word_write_ams_ev_vt, word_write_ams );
  VT_COUNT_UNSIGNED_VAL( word_read_ams_ev_vt, word_read_ams );
  VT_COUNT_UNSIGNED_VAL( T_read_ams_ev_vt, T_read_ams );
  VT_COUNT_UNSIGNED_VAL( word_fetch_add_ams_ev_vt, word_fetch_add_ams );
  VT_COUNT_UNSIGNED_VAL( word_compare_swap_ams_ev_vt, word_compare_swap_ams );
  VT_COUNT_UNSIGNED_VAL( generic_op_ams_ev_vt, generic_op_ams );
  VT_COUNT_UNSIGNED_VAL( ops_blocked_ev_vt, ops_blocked );
  VT_COUNT_UNSIGNED_VAL( ops_blocked_ticks_total_ev_vt, ops_blocked_ticks_total );
  VT_COUNT_UNSIGNED_VAL( ops_network_ticks_total_ev_vt, ops_network_ticks_total );
  VT_COUNT_UNSIGNED_VAL( ops_wakeup_ticks_total_ev_vt, ops_wakeup_ticks_total );
  VT_COUNT_UNSIGNED_VAL( ops_blocked_ticks_max_ev_vt, ops_blocked_ticks_max );
  VT_COUNT_UNSIGNED_VAL( ops_blocked_ticks_min_ev_vt, ops_blocked_ticks_min );
  VT_COUNT_UNSIGNED_VAL( ops_network_ticks_max_ev_vt, ops_network_ticks_max );
  VT_COUNT_UNSIGNED_VAL( ops_network_ticks_min_ev_vt, ops_network_ticks_min );
  VT_COUNT_UNSIGNED_VAL( ops_wakeup_ticks_max_ev_vt, ops_wakeup_ticks_max );
  VT_COUNT_UNSIGNED_VAL( ops_wakeup_ticks_min_ev_vt, ops_wakeup_ticks_min );
  VT_COUNT_DOUBLE_VAL( average_latency_ev_vt, nanless_double_ratio( ops_blocked_ticks_total, ops_blocked ) );
  VT_COUNT_DOUBLE_VAL( average_network_latency_ev_vt, nanless_double_ratio( ops_network_ticks_total, ops_blocked ) );
  VT_COUNT_DOUBLE_VAL( average_wakeup_latency_ev_vt, nanless_double_ratio( ops_wakeup_ticks_total, ops_blocked ) );
#endif
}

void DelegateStatistics::merge(const DelegateStatistics * other) {
  ops += other->ops;
  word_writes += other->word_writes;
  word_reads += other->word_reads;
  T_reads += other->T_reads;
  word_fetch_adds += other->word_fetch_adds;
  word_compare_swaps += other->word_compare_swaps;
  generic_ops += other->generic_ops;
  op_ams += other->op_ams;
  word_write_ams += other->word_write_ams;
  word_read_ams += other->word_read_ams;
  T_read_ams += other->T_read_ams;
  word_fetch_add_ams += other->word_fetch_add_ams;
  word_compare_swap_ams += other->word_compare_swap_ams;
  generic_op_ams += other->generic_op_ams;
  ops_blocked += other->ops_blocked;
  ops_blocked_ticks_total += other->ops_blocked_ticks_total;
  ops_network_ticks_total += other->ops_network_ticks_total;
  ops_wakeup_ticks_total += other->ops_wakeup_ticks_total;
  if( other->ops_blocked_ticks_max > ops_blocked_ticks_max )
    ops_blocked_ticks_max = other->ops_blocked_ticks_max;
  if( other->ops_blocked_ticks_min < ops_blocked_ticks_min )
    ops_blocked_ticks_min = other->ops_blocked_ticks_min;
  if( other->ops_network_ticks_max > ops_network_ticks_max )
    ops_network_ticks_max = other->ops_network_ticks_max;
  if( other->ops_network_ticks_min < ops_network_ticks_min )
    ops_network_ticks_min = other->ops_network_ticks_min;
  if( other->ops_wakeup_ticks_max > ops_wakeup_ticks_max )
    ops_wakeup_ticks_max = other->ops_wakeup_ticks_max;
  if( other->ops_wakeup_ticks_min < ops_wakeup_ticks_min )
    ops_wakeup_ticks_min = other->ops_wakeup_ticks_min;
}
