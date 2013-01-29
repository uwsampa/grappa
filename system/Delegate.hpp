
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#ifndef __DELEGATE_HPP__
#define __DELEGATE_HPP__

#include "Grappa.hpp"

/// Stats for delegate operations
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
    int64_t current_time = Grappa_get_timestamp();
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
    int64_t current_time = Grappa_get_timestamp();
    int64_t latency = current_time - start_time;
    ops_network_ticks_total += latency;
    if( latency > ops_network_ticks_max )
      ops_network_ticks_max = latency;
    if( latency < ops_network_ticks_min )
      ops_network_ticks_min = latency;
  }

  void dump( std::ostream& o, const char * terminator );
  void sample();
  void profiling_sample();
  void merge(DelegateStatistics * other);
  static void merge_am(DelegateStatistics * other, size_t sz, void* payload, size_t psz);
};

extern DelegateStatistics delegate_stats;


/// Delegate word write
void Grappa_delegate_write_word( GlobalAddress<int64_t> address, int64_t data );

/// Delegate word read
int64_t Grappa_delegate_read_word( GlobalAddress<int64_t> address );

/// Delegate word fetch and add
int64_t Grappa_delegate_fetch_and_add_word( GlobalAddress<int64_t> address, int64_t data );

/// Delegate word compare and swap
bool Grappa_delegate_compare_and_swap_word(GlobalAddress<int64_t> address, int64_t cmpval, int64_t newval);

template< typename T >
struct memory_desc;

/// Args for delegate generic read reply
template< typename T >
struct read_reply_args {
  GlobalAddress< memory_desc<T> > descriptor;
};

/// Handler for delegate generic read reply
template< typename T >
static void read_reply_am( read_reply_args<T> * args, size_t size, void * payload, size_t payload_size ) {
  DCHECK( payload_size == sizeof(T) );
  *(args->descriptor.pointer()->data) = *(static_cast<T*>(payload));
  args->descriptor.pointer()->done = true;
  if( args->descriptor.pointer()->t != NULL ) {
    Grappa_wake( args->descriptor.pointer()->t );
  }
  if( args->descriptor.pointer()->start_time != 0 ) {
    args->descriptor.pointer()->network_time = Grappa_get_timestamp();
    delegate_stats.record_network_latency( args->descriptor.pointer()->start_time );
  }
}

/// Descriptor for delegate generic read
template< typename T >
struct memory_desc {
  Thread * t;
  GlobalAddress<T> address;
  T* data;
  bool done;
  int64_t start_time;
  int64_t network_time;
};

/// Args for delegate generic read request
template< typename T >
struct read_request_args {
  GlobalAddress< memory_desc<T> > descriptor;
  GlobalAddress<T> address;
};

/// Handler for delegate generic read request
template< typename T >
static void read_request_am( read_request_args<T> * args, size_t size, void * payload, size_t payload_size ) {
  delegate_stats.count_op_am();
  delegate_stats.count_T_read_am();
  T data = *(args->address.pointer());
  read_reply_args<T> reply_args;
  reply_args.descriptor = args->descriptor;
  if( args->descriptor.node() == Grappa_mynode() ) {
    read_reply_am( &reply_args, sizeof( reply_args ), &data, sizeof(data) );
  } else {
    Grappa_call_on( args->descriptor.node(), &read_reply_am<T>, 
		     &reply_args, sizeof(reply_args), 
		     &data, sizeof(data) );
  }
}

/// Delegate generic read
template< typename T >
void Grappa_delegate_read( GlobalAddress<T> address, T * buf) {
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
  if( address.node() == Grappa_mynode() ) {
    read_request_am( &args, sizeof( args ), NULL, 0 );
  } else {
    Grappa_call_on( address.node(), &read_request_am<T>, &args );
  }
  if( !md.done ) {
    md.start_time = Grappa_get_timestamp();
    while (!md.done) {
      md.t = CURRENT_THREAD;
      Grappa_suspend();
      md.t = NULL;
    }
    delegate_stats.record_wakeup_latency( md.start_time, md.network_time );
  } else {
    md.start_time = 0;
  }
}

/// Generic delegate functor descriptor
struct DelegateCallbackArgs {
  Thread * sleeper;
  void* forig; // pointer to original functor
  bool done;
};

/// Handler for generic delegate functor reply
static void am_delegate_wake(GlobalAddress<DelegateCallbackArgs> * callback, size_t csz, void * p, size_t psz) {
  // copy possibly-modified functor back 
  // (allows user to modify func to effectively pass a return value back)
  DelegateCallbackArgs * args = callback->pointer();

  memcpy(args->forig, p, psz);
 
  if ( args->sleeper != NULL ) { 
    Grappa_wake(args->sleeper);
    args->sleeper = NULL;
  }
  
  args->done = true;
}

/// Handler for generic delegate functor request
template<typename Func>
static void am_delegate(GlobalAddress<DelegateCallbackArgs> * callback, size_t csz, void* p, size_t fsz) {
  delegate_stats.count_op_am();
  delegate_stats.count_generic_op_am();
  Func * f = static_cast<Func*>(p);
  (*f)();
  Grappa_call_on(callback->node(), &am_delegate_wake, callback, sizeof(*callback), p, fsz);
}

/// Supports more generic delegate operations in the form of functors. The given 
/// functor is copied over to remote node, executed atomically, and copied back. 
/// This allows a value to be 'returned' by the delegate in the form of a modified 
/// Functor field. Note: this functor will be run in an active message, so 
/// operations disallowed in active messages are disallowed here (i.e. no yielding 
/// or suspending)
/// TODO: it would be better to not do the extra copying associated with sending the "return" value back and forth
template<typename Func>
void Grappa_delegate_func(Func * f, Node target) {
  delegate_stats.count_op();
  delegate_stats.count_generic_op();
  if (target == Grappa_mynode()) {
    (*f)();
  } else {
    DelegateCallbackArgs callbackArgs = { NULL, (void*)f, false };
    GlobalAddress<DelegateCallbackArgs> args_address = make_global( &callbackArgs );
    Grappa_call_on(target, &am_delegate<Func>, &args_address, sizeof(args_address), (void*)f, sizeof(*f));
    
    if ( !callbackArgs.done ) {
        callbackArgs.sleeper = CURRENT_THREAD;
        Grappa_suspend();
    }
  }
}



/// 
/// Generic delegate operations, with templated argument, return type, function pointer
///

/// TODO: alternative is to take a GlobalAddress, which we can get the
/// target from, but then F will take the pointer part as an argument

/// wrapper for the user's delegated function arguments
/// to include a pointer to the memory descriptor
template < typename ArgType, typename T > 
struct generic_delegate_request_args {
  ArgType argument;
  GlobalAddress< memory_desc<T> > descriptor;
};

/// Args for generic delegate reply
template < typename T > 
struct generic_delegate_reply_args {
  T retVal;
  GlobalAddress< memory_desc<T> > descriptor;
};

/// Generic delegate reply active message
/// Fill the return value, wake the sleeping thread, mark operation as done
template < typename ArgType, typename ReturnType, ReturnType (*F)(ArgType) >
void generic_delegate_reply_am( generic_delegate_reply_args<ReturnType> * args, size_t arg_size, void * payload, size_t payload_size ) {
  memory_desc<ReturnType> * md = args->descriptor.pointer();
  
  *(md->data) = args->retVal;

  if ( md->t != NULL ) {
    Grappa_wake( md->t );
    md->t = NULL;
  }

  md->done = true;
  
  if( md->start_time != 0 ) {
    md->network_time = Grappa_get_timestamp();
    delegate_stats.record_network_latency( md->start_time );
  }
}

/// Generic delegate request active message
/// Call the delegated function, reply with the return value
template < typename ArgType, typename ReturnType, ReturnType (*F)(ArgType) >
void generic_delegate_request_am( generic_delegate_request_args<ArgType,ReturnType> * args, size_t arg_size, void * payload, size_t payload_size ) {
  delegate_stats.count_op_am();
 
  ReturnType r = F( args->argument );

  generic_delegate_reply_args< ReturnType > reply_args;
  reply_args.retVal = r;
  reply_args.descriptor = args->descriptor;
  
  Grappa_call_on( args->descriptor.node(), &generic_delegate_reply_am<ArgType, ReturnType, F>, &reply_args );
}

///
/// Call F(arg) as a delegated function on the target node, and return the return value.
/// Blocking operation.
///
template < typename ArgType, typename ReturnType, ReturnType (*F)(ArgType) >
ReturnType Grappa_delegate_func( ArgType arg, Node target ) {
  delegate_stats.count_op();
  delegate_stats.count_generic_op();
  
  if (target == Grappa_mynode() ) {
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

    Grappa_call_on( target, &generic_delegate_request_am<ArgType,ReturnType,F>, &del_args );

    if ( !md.done ) {
      md.start_time = Grappa_get_timestamp();
      md.t = CURRENT_THREAD;
      Grappa_suspend();
      CHECK( md.done );
      delegate_stats.record_wakeup_latency( md.start_time, md.network_time );
    }

    return result;
  }
}

#include "Grappa.hpp"
#include "Message.hpp"
#include "FullEmpty.hpp"
#include "Message.hpp"
#include "ConditionVariable.hpp"
    
namespace Grappa {
  namespace delegate {
    /// @addtogroup Delegates
    /// @{
    
    /// Implements essentially a blocking remote procedure call. Callable object (lambda,
    /// function pointer, or functor object) is called from the `dest` core and the return
    /// value is sent back to the calling task.
    template <typename F>
    inline auto call(Core dest, F func) -> decltype(func()) {
      using R = decltype(func());
      Node origin = Grappa_mynode();
      
      if (dest == origin) {
        // short-circuit if local
        return func();
      } else {
        FullEmpty<R> result;
        
        send_message(dest, [&result, origin, func] {
          R val = func();
          
          // TODO: replace with handler-safe send_message
          send_heap_message(origin, [&result, val] {
            result.writeXF(val); // can't block in message, assumption is that result is already empty
          });
        }); // send message
        // ... and wait for the result
        R r = result.readFE();
        return r;
      }
    }
    
    /// Read the value (potentially remote) at the given GlobalAddress, blocks the calling task until
    /// round-trip communication is complete.
    /// @warning { Target object must lie on a single node (not span blocks in global address space). }
    template< typename T >
    T read(GlobalAddress<T> target) {
      return call(target.node(), [target]() -> T {
        return *target.pointer();
      });
    }
    
    /// Blocking remote write.
    /// @warning { Target object must lie on a single node (not span blocks in global address space). }
    template< typename T, typename U >
    bool write(GlobalAddress<T> target, U value) {
      // TODO: don't return any val, requires changes to `delegate::call()`.
      return call(target.node(), [target, value]() -> bool {
        *target.pointer() = (T)value;
        return true;
      });
    }
    
    /// Fetch the value at `target`, increment the value stored there with `inc` and return the
    /// original value to blocking thread.
    /// @warning { Target object must lie on a single node (not span blocks in global address space). }
    template< typename T, typename U >
    T fetch_and_add(GlobalAddress<T> target, U inc) {
      T * p = target.pointer();
      return call(target.node(), [p, inc]() -> T {
        T r = *p;
        *p += inc;
        return r;
      });
    }
    
    /// If value at `target` equals `cmp_val`, set the value to `new_val` and return `true`,
    /// otherwise do nothing and return `false`.
    /// @warning { Target object must lie on a single node (not span blocks in global address space). }
    template< typename T, typename U, typename V >
    bool compare_and_swap(GlobalAddress<T> target, U cmp_val, V new_val) {
      T * p = target.pointer();
      return call(target.node(), [p, cmp_val, new_val]() -> bool {
        if (cmp_val == *p) {
          *p = new_val;
          return true;
        } else {
          return false;
        }
      });
    }
    
    /// @}
  } // namespace delegate
} // namespace Grappa

#endif
