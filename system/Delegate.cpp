

#include "Delegate.hpp"

#include <cassert>
#include <gflags/gflags.h>
#include <glog/logging.h>


struct memory_descriptor {
  Thread * t;
  GlobalAddress<int64_t> address;
  int64_t data;
  bool done;
};

/////////////////////////////////////////////////////////////////////////////

static inline void Delegate_wait( memory_descriptor * md ) {
  while( !md->done ) {
    md->t = CURRENT_THREAD;
    SoftXMT_suspend();
    md->t = NULL;
  }
}

static inline void Delegate_wakeup( memory_descriptor * md ) {
  if( md->t != NULL ) {
    SoftXMT_wake( md->t );
  }
}

/////////////////////////////////////////////////////////////////////////////

struct memory_write_reply_args {
  GlobalAddress<memory_descriptor> descriptor;
};

static void memory_write_reply_am( memory_write_reply_args * args, size_t size, void * payload, size_t payload_size ) {
  (args->descriptor.pointer())->done = true;
  Delegate_wakeup( args->descriptor.pointer() );
}

struct memory_write_request_args {
  GlobalAddress<memory_descriptor> descriptor;
  GlobalAddress<int64_t> address;
};


static void memory_write_request_am( memory_write_request_args * args, size_t size, void * payload, size_t payload_size ) {
  CHECK( (int64_t)args->address.pointer() > 0x1000 )<< "read request:"
                                                    << "\n address="<<args->address
                                                    << "\n descriptor="<<args->descriptor;
  assert( payload_size == sizeof(int64_t) );
  int64_t payload_int = *(static_cast<int64_t*>(payload));
  VLOG(3) << "payload("<<(void*)payload<<")="<<payload_int<<"\n    pointer="<<(void*)args->address.pointer();
  *(args->address.pointer()) = payload_int;
  memory_write_reply_args reply_args;
  reply_args.descriptor = args->descriptor;
  if( args->descriptor.node() == SoftXMT_mynode() ) {
    memory_write_reply_am( &reply_args, sizeof( reply_args ), NULL, 0 );
  } else {
    SoftXMT_call_on( args->descriptor.node(), &memory_write_reply_am, &reply_args );
  }
}

void SoftXMT_delegate_write_word( GlobalAddress<int64_t> address, int64_t data ) {
  delegate_stats.count_op();
  delegate_stats.count_word_write();

  memory_descriptor md;
  md.address = address;
  md.data = data;
  md.done = false;
  md.t = NULL;
  memory_write_request_args args;
  args.descriptor = make_global(&md);
  args.address = address;
  if( address.node() == SoftXMT_mynode() ) {
    memory_write_request_am( &args, sizeof(args), 
			     &data, sizeof(data) );
  } else {
    SoftXMT_call_on( address.node(), &memory_write_request_am, 
		     &args, sizeof(args), 
		     &data, sizeof(data) );
  }
  Delegate_wait( &md );
}

/////////////////////////////////////////////////////////////////////////////

struct memory_read_reply_args {
  GlobalAddress<memory_descriptor> descriptor;
};

static void memory_read_reply_am( memory_read_reply_args * args, size_t size, void * payload, size_t payload_size ) {
  DCHECK( payload_size == sizeof(int64_t ) );
  args->descriptor.pointer()->data = *(static_cast<int64_t*>(payload));
  args->descriptor.pointer()->done = true;
  Delegate_wakeup( args->descriptor.pointer() );
}

struct memory_read_request_args {
  GlobalAddress<memory_descriptor> descriptor;
  GlobalAddress<int64_t> address;
};


static void memory_read_request_am( memory_read_request_args * args, size_t size, void * payload, size_t payload_size ) {
  DCHECK( (int64_t)args->address.pointer() > 0x1000 ) << "read request:"
						      << "\n address="<<args->address
						      << "\n descriptor="<<args->descriptor;
  int64_t data = *(args->address.pointer());
  memory_read_reply_args reply_args;
  reply_args.descriptor = args->descriptor;
  if( args->descriptor.node() == SoftXMT_mynode() ) {
    memory_read_reply_am( &reply_args, sizeof( reply_args ), &data, sizeof(data) );
  } else {
    SoftXMT_call_on( args->descriptor.node(), &memory_read_reply_am, 
		     &reply_args, sizeof(reply_args), 
		     &data, sizeof(data) );
  }
}

int64_t SoftXMT_delegate_read_word( GlobalAddress<int64_t> address ) {
  delegate_stats.count_op();
  delegate_stats.count_word_read();

  memory_descriptor md;
  md.address = address;
  md.data = 0;
  md.done = false;
  md.t = NULL;
  memory_read_request_args args;
  args.descriptor = make_global(&md);
  args.address = address;
  if( address.node() == SoftXMT_mynode() ) {
    memory_read_request_am( &args, sizeof(args), 
			    NULL, 0 );
  } else {
    SoftXMT_call_on( address.node(), &memory_read_request_am, &args );
  }
  Delegate_wait( &md );
  return md.data;
}


/////////////////////////////////////////////////////////////////////////////


struct memory_fetch_add_reply_args {
  GlobalAddress<memory_descriptor> descriptor;
};

struct memory_fetch_add_request_args {
  GlobalAddress<memory_descriptor> descriptor;
  GlobalAddress<int64_t> address;
};

static void memory_fetch_add_reply_am( memory_fetch_add_reply_args * args, size_t size, void * payload, size_t payload_size ) {
  assert( payload_size == sizeof(int64_t) );
  args->descriptor.pointer()->data = *(static_cast<int64_t*>(payload));
  args->descriptor.pointer()->done = true;
  Delegate_wakeup( args->descriptor.pointer() );
}

/// runs on server side to fetch data 
static void memory_fetch_add_request_am( memory_fetch_add_request_args * args, size_t size, void * payload, size_t payload_size ) {
  assert( payload_size == sizeof(int64_t) );
  int64_t data = *(args->address.pointer()); // fetch
  *(args->address.pointer()) += *(static_cast<int64_t*>(payload)); // increment
  memory_fetch_add_reply_args reply_args;
  reply_args.descriptor = args->descriptor;

  if( args->descriptor.node() == SoftXMT_mynode() ) {
    memory_fetch_add_reply_am( &reply_args, sizeof(reply_args), 
			       &data, sizeof(data) );
  } else {
    SoftXMT_call_on( args->descriptor.node(), &memory_fetch_add_reply_am, 
		     &reply_args, sizeof(reply_args), 
		     &data, sizeof(data) );
  }
}

int64_t SoftXMT_delegate_fetch_and_add_word( GlobalAddress<int64_t> address, int64_t data ) {
  delegate_stats.count_op();
  delegate_stats.count_word_fetch_add();

  // set up descriptor
  memory_descriptor md;
  md.address = address;
  md.data = data;
  md.done = false;
  md.t = NULL;

  // set up args for request
  memory_fetch_add_request_args args;
  args.descriptor = make_global(&md);
  args.address = address;

  // make request
  if( address.node() == SoftXMT_mynode() ) {
    memory_fetch_add_request_am( &args, sizeof(args), 
				 &data, sizeof(data) );
  } else {
    SoftXMT_call_on( address.node(), &memory_fetch_add_request_am, 
		     &args, sizeof(args), 
		     &data, sizeof(data) );
  }

  // wait for response
  Delegate_wait( &md );
  
  return md.data;
}

struct cmp_swap_request_args {
  GlobalAddress<memory_descriptor> descriptor;
  GlobalAddress<int64_t> address;
  int64_t newval;
  int64_t cmpval;
};
struct cmp_swap_reply_args {
  GlobalAddress<memory_descriptor> descriptor;
};

static void cmp_swap_reply_am(cmp_swap_reply_args * args, size_t size, void * payload, size_t payload_size) {
  assert( payload_size == sizeof(int64_t) );
  args->descriptor.pointer()->data = *(static_cast<int64_t*>(payload));
  args->descriptor.pointer()->done = true;
  Delegate_wakeup( args->descriptor.pointer() );
}

static void cmp_swap_request_am(cmp_swap_request_args * args, size_t sz, void * p, size_t psz) {
  int64_t data = *(args->address.pointer());
  int64_t swapped = false;
  if (data == args->cmpval) { // compare
    *(args->address.pointer()) = args->newval; // swap
    swapped = true;
  }
  
  cmp_swap_reply_args reply_args;
  reply_args.descriptor = args->descriptor;
  
  if( args->descriptor.node() == SoftXMT_mynode() ) {
    cmp_swap_reply_am( &reply_args, sizeof(reply_args), 
		       &swapped, sizeof(swapped) );
  } else {
    SoftXMT_call_on(args->descriptor.node(), &cmp_swap_reply_am, &reply_args, sizeof(reply_args), &swapped, sizeof(swapped));
  }
}

bool SoftXMT_delegate_compare_and_swap_word(GlobalAddress<int64_t> address, int64_t cmpval, int64_t newval) {
  delegate_stats.count_op();
  delegate_stats.count_word_compare_swap();

  // set up descriptor
  memory_descriptor md;
  md.address = address;
  md.done = false;
  md.t = NULL;
  
  // set up args for request
  cmp_swap_request_args args;
  args.descriptor = make_global(&md);
  args.address = address;
  args.cmpval = cmpval;
  args.newval = newval;
  
  // make request
  if( address.node() == SoftXMT_mynode() ) {
    cmp_swap_request_am( &args, sizeof(args), 
			 NULL, 0 );
  } else {
    SoftXMT_call_on( address.node(), &cmp_swap_request_am, 
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
  , word_fetch_adds_ev_vt( VT_COUNT_DEF( "Delegate word fetch and adds", "fads", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , word_compare_swaps_ev_vt( VT_COUNT_DEF( "Delegate word compare and swaps", "casses", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
  , generic_ops_ev_vt( VT_COUNT_DEF( "Delegate generic operations", "operations", VT_COUNT_TYPE_UNSIGNED, delegate_grp_vt ) )
#endif
{
  reset();
}

void DelegateStatistics::reset() {
  ops = 0;
  word_writes = 0;
  word_reads = 0;
  word_fetch_adds = 0;
  word_compare_swaps = 0;
  generic_ops = 0;
}

void DelegateStatistics::dump() {
  std::cout << "DelegateStats { "
	    << "ops: " << ops << ", "
	    << "word_writes: " << word_writes  << ", "
	    << "word_reads: " << word_reads  << ", "
	    << "word_fetch_adds: " << word_fetch_adds  << ", "
	    << "word_compare_swaps: " << word_compare_swaps  << ", "
	    << "generic_ops: " << generic_ops  << ", "
	    << " }" << std::endl;
}

void DelegateStatistics::sample() {
  ;
}

void DelegateStatistics::profiling_sample() {
#ifdef VTRACE_SAMPLED
  VT_COUNT_UNSIGNED_VAL( ops_ev_vt, ops );
  VT_COUNT_UNSIGNED_VAL( word_writes_ev_vt, word_writes );
  VT_COUNT_UNSIGNED_VAL( word_reads_ev_vt, word_reads );
  VT_COUNT_UNSIGNED_VAL( word_fetch_adds_ev_vt, word_fetch_adds );
  VT_COUNT_UNSIGNED_VAL( word_compare_swaps_ev_vt, word_compare_swaps );
  VT_COUNT_UNSIGNED_VAL( generic_ops_ev_vt, generic_ops );
#endif
}

void DelegateStatistics::merge(DelegateStatistics * other) {
  ops += other->ops;
  word_writes += other->word_writes;
  word_reads += other->word_reads;
  word_fetch_adds += other->word_fetch_adds;
  word_compare_swaps += other->word_compare_swaps;
  generic_ops += other->generic_ops;
}

void DelegateStatistics::merge_am(DelegateStatistics * other, size_t sz, void* payload, size_t psz) {
  delegate_stats.merge( other );
}
