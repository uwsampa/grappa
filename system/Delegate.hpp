
#ifndef __DELEGATE_HPP__
#define __DELEGATE_HPP__

#include "SoftXMT.hpp"


class DelegateStatistics {
private:
  uint64_t ops;
  uint64_t word_writes;
  uint64_t word_reads;
  uint64_t word_fetch_adds;
  uint64_t word_compare_swaps;
  uint64_t generic_ops;
#ifdef VTRACE_SAMPLED
  unsigned delegate_grp_vt;
  unsigned ops_ev_vt;
  unsigned word_writes_ev_vt;
  unsigned word_reads_ev_vt;
  unsigned word_fetch_adds_ev_vt;
  unsigned word_compare_swaps_ev_vt;
  unsigned generic_ops_ev_vt;
#endif

public:
  DelegateStatistics();
  void reset();

  inline void count_op() { ops++; }
  inline void count_word_write() { word_writes++; }
  inline void count_word_read() { word_reads++; }
  inline void count_word_fetch_add() { word_fetch_adds++; }
  inline void count_word_compare_swap() { word_compare_swaps++; }
  inline void count_generic_op() { generic_ops++; }

  void dump();
  void sample();
  void profiling_sample();
  void merge(DelegateStatistics * other);
  static void merge_am(DelegateStatistics * other, size_t sz, void* payload, size_t psz);
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
}

template< typename T >
struct memory_desc {
  Thread * t;
  GlobalAddress<T> address;
  T* data;
  bool done;
};

template< typename T >
struct read_request_args {
  GlobalAddress< memory_desc<T> > descriptor;
  GlobalAddress<T> address;
};

template< typename T >
static void read_request_am( read_request_args<T> * args, size_t size, void * payload, size_t payload_size ) {
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
  memory_desc<T> md;
  md.address = address;
  md.done = false;
  md.data = buf;
  md.t = NULL;
  read_request_args<T> args;
  args.descriptor = make_global(&md);
  args.address = address;
  if( address.node() == SoftXMT_mynode() ) {
    read_request_am( &args, sizeof( args ), NULL, 0 );
  } else {
    SoftXMT_call_on( address.node(), &read_request_am<T>, &args );
  }
  while (!md.done) {
    md.t = CURRENT_THREAD;
    SoftXMT_suspend();
    md.t = NULL;
  }
}

struct DelegateCallbackArgs {
  GlobalAddress<Thread> sleeper;
  void* forig; // pointer to original functor
};

static void am_delegate_wake(DelegateCallbackArgs * callback, size_t csz, void * p, size_t psz) {
  // copy possibly-modified functor back 
  // (allows user to modify func to effectively pass a return value back)
  memcpy(callback->forig, p, psz);
  SoftXMT_wake(callback->sleeper.pointer());
}

template<typename Func>
static void am_delegate(DelegateCallbackArgs * callback, size_t csz, void* p, size_t fsz) {
  Func * f = static_cast<Func*>(p);
  (*f)();
  SoftXMT_call_on(callback->sleeper.node(), &am_delegate_wake, callback, sizeof(*callback), p, fsz);
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
    DelegateCallbackArgs callbackArgs = { make_global(CURRENT_THREAD), (void*)f };
    SoftXMT_call_on(target, &am_delegate<Func>, &callbackArgs, sizeof(callbackArgs), (void*)f, sizeof(*f));
    SoftXMT_suspend();
  }
}

#endif
