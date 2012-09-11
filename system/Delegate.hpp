
#ifndef __DELEGATE_HPP__
#define __DELEGATE_HPP__

#include "SoftXMT.hpp"


class DelegateStatistics {
private:
  uint64_t ops;
  uint64_t word_writes;
  uint64_t word_reads;
  uint64_t T_reads;
  uint64_t word_fetch_adds;
  uint64_t word_compare_swaps;
  uint64_t generic_ops;
  uint64_t op_ams;
  uint64_t word_write_ams;
  uint64_t word_read_ams;
  uint64_t T_read_ams;
  uint64_t word_fetch_add_ams;
  uint64_t word_compare_swap_ams;
  uint64_t generic_op_ams;
  uint64_t ops_blocked;
  uint64_t ops_blocked_ticks_total;
  uint64_t ops_network_ticks_total;
  uint64_t ops_wakeup_ticks_total;
  uint64_t ops_blocked_ticks_max;
  uint64_t ops_blocked_ticks_min;
  uint64_t ops_network_ticks_max;
  uint64_t ops_network_ticks_min;
  uint64_t ops_wakeup_ticks_max;
  uint64_t ops_wakeup_ticks_min;
#ifdef VTRACE_SAMPLED
  unsigned delegate_grp_vt;
  unsigned ops_ev_vt;
  unsigned word_writes_ev_vt;
  unsigned word_reads_ev_vt;
  unsigned T_reads_ev_vt;
  unsigned word_fetch_adds_ev_vt;
  unsigned word_compare_swaps_ev_vt;
  unsigned generic_ops_ev_vt;
  unsigned op_ams_ev_vt;
  unsigned word_write_ams_ev_vt;
  unsigned word_read_ams_ev_vt;
  unsigned T_read_ams_ev_vt;
  unsigned word_fetch_add_ams_ev_vt;
  unsigned word_compare_swap_ams_ev_vt;
  unsigned generic_op_ams_ev_vt;
  unsigned ops_blocked_ev_vt;
  unsigned ops_blocked_ticks_total_ev_vt;
  unsigned ops_network_ticks_total_ev_vt;
  unsigned ops_wakeup_ticks_total_ev_vt;
  unsigned ops_blocked_ticks_max_ev_vt;
  unsigned ops_blocked_ticks_min_ev_vt;
  unsigned ops_wakeup_ticks_max_ev_vt;
  unsigned ops_wakeup_ticks_min_ev_vt;
  unsigned ops_network_ticks_max_ev_vt;
  unsigned ops_network_ticks_min_ev_vt;
  unsigned average_latency_ev_vt;
  unsigned average_network_latency_ev_vt;
  unsigned average_wakeup_latency_ev_vt;

#endif

public:
  DelegateStatistics();
  void reset();

  inline void count_op() { ops++; }
  inline void count_word_write() { word_writes++; }
  inline void count_word_read() { word_reads++; }
  inline void count_T_read() { T_reads++; }
  inline void count_word_fetch_add() { word_fetch_adds++; }
  inline void count_word_compare_swap() { word_compare_swaps++; }
  inline void count_generic_op() { generic_ops++; }

  inline void count_op_am() { op_ams++; }
  inline void count_word_write_am() { word_write_ams++; }
  inline void count_word_read_am() { word_read_ams++; }
  inline void count_T_read_am() { T_read_ams++; }
  inline void count_word_fetch_add_am() { word_fetch_add_ams++; }
  inline void count_word_compare_swap_am() { word_compare_swap_ams++; }
  inline void count_generic_op_am() { generic_op_ams++; }

  inline void record_wakeup_latency( int64_t start_time, int64_t network_time ) { 
    ops_blocked++; 
    int64_t current_time = SoftXMT_get_timestamp();
    int64_t blocked_latency = current_time - start_time;
    int64_t wakeup_latency = current_time - network_time;
    ops_blocked_ticks_total += blocked_latency;
    ops_wakeup_ticks_total += wakeup_latency;
    if( blocked_latency > ops_blocked_ticks_max ) 
      ops_blocked_ticks_max = blocked_latency;
    if( blocked_latency < ops_blocked_ticks_min ) 
      ops_blocked_ticks_min = blocked_latency;
    if( wakeup_latency > ops_wakeup_ticks_max )
      ops_wakeup_ticks_max = wakeup_latency;
    if( wakeup_latency < ops_wakeup_ticks_min )
      ops_wakeup_ticks_min = wakeup_latency;
  }

  inline void record_network_latency( int64_t start_time ) { 
    int64_t current_time = SoftXMT_get_timestamp();
    int64_t latency = current_time - start_time;
    ops_network_ticks_total += latency;
    if( latency > ops_network_ticks_max )
      ops_network_ticks_max = latency;
    if( latency < ops_network_ticks_min )
      ops_network_ticks_min = latency;
  }

  void dump();
  void sample();
  void profiling_sample();
  void merge(const DelegateStatistics * other);
};

extern DelegateStatistics delegate_stats;


void SoftXMT_delegate_write_word( GlobalAddress<int64_t> address, int64_t data );

int64_t SoftXMT_delegate_read_word( GlobalAddress<int64_t> address );

int64_t SoftXMT_delegate_fetch_and_add_word( GlobalAddress<int64_t> address, int64_t data );

bool SoftXMT_delegate_compare_and_swap_word(GlobalAddress<int64_t> address, int64_t cmpval, int64_t newval);

template< typename T >
struct memory_desc;

template< typename T >
struct read_reply_args {
  GlobalAddress< memory_desc<T> > descriptor;
};

template< typename T >
static void read_reply_am( read_reply_args<T> * args, size_t size, void * payload, size_t payload_size ) {
  DCHECK( payload_size == sizeof(T) );
  *(args->descriptor.pointer()->data) = *(static_cast<T*>(payload));
  args->descriptor.pointer()->done = true;
  if( args->descriptor.pointer()->t != NULL ) {
    SoftXMT_wake( args->descriptor.pointer()->t );
  }
  if( args->descriptor.pointer()->start_time != 0 ) {
    args->descriptor.pointer()->network_time = SoftXMT_get_timestamp();
    delegate_stats.record_network_latency( args->descriptor.pointer()->start_time );
  }
}

template< typename T >
struct memory_desc {
  Thread * t;
  GlobalAddress<T> address;
  T* data;
  bool done;
  int64_t start_time;
  int64_t network_time;
};

template< typename T >
struct read_request_args {
  GlobalAddress< memory_desc<T> > descriptor;
  GlobalAddress<T> address;
};

template< typename T >
static void read_request_am( read_request_args<T> * args, size_t size, void * payload, size_t payload_size ) {
  delegate_stats.count_op_am();
  delegate_stats.count_T_read_am();
  T data = *(args->address.pointer());
  read_reply_args<T> reply_args;
  reply_args.descriptor = args->descriptor;
  if( args->descriptor.node() == SoftXMT_mynode() ) {
    read_reply_am( &reply_args, sizeof( reply_args ), &data, sizeof(data) );
  } else {
    SoftXMT_call_on( args->descriptor.node(), &read_reply_am<T>, 
		     &reply_args, sizeof(reply_args), 
		     &data, sizeof(data) );
  }
}

template< typename T >
void SoftXMT_delegate_read( GlobalAddress<T> address, T * buf) {
  delegate_stats.count_op();
  delegate_stats.count_T_read();
  memory_desc<T> md;
  md.address = address;
  md.done = false;
  md.data = buf;
  md.t = NULL;
  md.start_time = 0;
  md.network_time = 0;
  read_request_args<T> args;
  args.descriptor = make_global(&md);
  args.address = address;
  if( address.node() == SoftXMT_mynode() ) {
    read_request_am( &args, sizeof( args ), NULL, 0 );
  } else {
    SoftXMT_call_on( address.node(), &read_request_am<T>, &args );
  }
  if( !md.done ) {
    md.start_time = SoftXMT_get_timestamp();
    while (!md.done) {
      md.t = CURRENT_THREAD;
      SoftXMT_suspend();
      md.t = NULL;
    }
    delegate_stats.record_wakeup_latency( md.start_time, md.network_time );
  } else {
    md.start_time = 0;
  }
}

struct DelegateCallbackArgs {
  Thread * sleeper;
  void* forig; // pointer to original functor
  bool done;
};

static void am_delegate_wake(GlobalAddress<DelegateCallbackArgs> * callback, size_t csz, void * p, size_t psz) {
  // copy possibly-modified functor back 
  // (allows user to modify func to effectively pass a return value back)
  DelegateCallbackArgs * args = callback->pointer();

  memcpy(args->forig, p, psz);
 
  if ( args->sleeper != NULL ) { 
    SoftXMT_wake(args->sleeper);
    args->sleeper = NULL;
  }
  
  args->done = true;
}

template<typename Func>
static void am_delegate(GlobalAddress<DelegateCallbackArgs> * callback, size_t csz, void* p, size_t fsz) {
  delegate_stats.count_op_am();
  delegate_stats.count_generic_op_am();
  Func * f = static_cast<Func*>(p);
  (*f)();
  SoftXMT_call_on(callback->node(), &am_delegate_wake, callback, sizeof(*callback), p, fsz);
}

/// Supports more generic delegate operations in the form of functors. The given 
/// functor is copied over to remote node, executed atomically, and copied back. 
/// This allows a value to be 'returned' by the delegate in the form of a modified 
/// Functor field. Note: this functor will be run in an active message, so 
/// operations disallowed in active messages are disallowed here (i.e. no yielding 
/// or suspending)
/// TODO: it would be better to not do the extra copying associated with sending the "return" value back and forth
template<typename Func>
void SoftXMT_delegate_func(Func * f, Node target) {
  delegate_stats.count_op();
  delegate_stats.count_generic_op();
  if (target == SoftXMT_mynode()) {
    (*f)();
  } else {
    DelegateCallbackArgs callbackArgs = { NULL, (void*)f, false };
    GlobalAddress<DelegateCallbackArgs> args_address = make_global( &callbackArgs );
    SoftXMT_call_on(target, &am_delegate<Func>, &args_address, sizeof(args_address), (void*)f, sizeof(*f));
    
    if ( !callbackArgs.done ) {
        callbackArgs.sleeper = CURRENT_THREAD;
        SoftXMT_suspend();
    }
  }
}



/// 
/// Generic delegate operations, with templated argument, return type, function pointer
///

/* TODO alternative is to take a GlobalAddress, which we can get
 * the target from, but then F will take the pointer part as an argument 
 */

// wrapper for the user's delegated function arguments
// to include a pointer to the memory descriptor
template < typename ArgType, typename T > 
struct generic_delegate_request_args {
  ArgType argument;
  GlobalAddress< memory_desc<T> > descriptor;
};

template < typename T > 
struct generic_delegate_reply_args {
  T retVal;
  GlobalAddress< memory_desc<T> > descriptor;
};

// generic delegate reply active message
// Fill the return value, wake the sleeping thread, mark operation as done
template < typename ArgType, typename ReturnType, ReturnType (*F)(ArgType) >
void generic_delegate_reply_am( generic_delegate_reply_args<ReturnType> * args, size_t arg_size, void * payload, size_t payload_size ) {
  memory_desc<ReturnType> * md = args->descriptor.pointer();
  
  *(md->data) = args->retVal;

  if ( md->t != NULL ) {
    SoftXMT_wake( md->t );
    md->t = NULL;
  }

  md->done = true;
  
  if( md->start_time != 0 ) {
    md->network_time = SoftXMT_get_timestamp();
    delegate_stats.record_network_latency( md->start_time );
  }
}

// generic delegate request active message
// Call the delegated function, reply with the return value
template < typename ArgType, typename ReturnType, ReturnType (*F)(ArgType) >
void generic_delegate_request_am( generic_delegate_request_args<ArgType,ReturnType> * args, size_t arg_size, void * payload, size_t payload_size ) {
  delegate_stats.count_op_am();
 
  ReturnType r = F( args->argument );

  generic_delegate_reply_args< ReturnType > reply_args;
  reply_args.retVal = r;
  reply_args.descriptor = args->descriptor;
  
  SoftXMT_call_on( args->descriptor.node(), &generic_delegate_reply_am<ArgType, ReturnType, F>, &reply_args );
}

///
/// Call F(arg) as a delegated function on the target node, and return the return value.
/// Blocking operation.
///
template < typename ArgType, typename ReturnType, ReturnType (*F)(ArgType) >
ReturnType SoftXMT_delegate_func( ArgType arg, Node target ) {
  delegate_stats.count_op();
  delegate_stats.count_generic_op();
  
  if (target == SoftXMT_mynode() ) {
    return F(arg);
  } else {
    memory_desc<ReturnType> md;
    md.done = false;

    ReturnType result;
    md.data = &result; 
    md.t = NULL;
    md.start_time = 0;
    md.network_time = 0;

    generic_delegate_request_args< ArgType, ReturnType > del_args; 
    del_args.argument = arg;
    del_args.descriptor = make_global(&md);

    SoftXMT_call_on( target, &generic_delegate_request_am<ArgType,ReturnType,F>, &del_args );

    if ( !md.done ) {
      md.start_time = SoftXMT_get_timestamp();
      md.t = CURRENT_THREAD;
      SoftXMT_suspend();
      CHECK( md.done );
      delegate_stats.record_wakeup_latency( md.start_time, md.network_time );
    }

    return result;
  }
}

/// Return the size of the request message for this delegate function.
template < typename ArgType, typename ReturnType >
size_t SoftXMT_sizeof_delegate_func_request( ) {
  generic_delegate_request_args< ArgType, ReturnType > del_args; 
  // must be called the same way as call_on in SoftXMT_delegate_func()
  return SoftXMT_sizeof_message( &del_args );
}

/// Return the size of the reply message for this delegate function.
template < typename ArgType, typename ReturnType >
size_t SoftXMT_sizeof_delegate_func_reply( ) {
  generic_delegate_reply_args< ReturnType > reply_args;
  // must be called the same way as call_on in generic_delegate_request_am()
  return SoftXMT_sizeof_message( &reply_args );
}


#endif
