
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

/// One implementation of GUPS. This does no load-balancing, and may
/// suffer from some load imbalance.

#include <memory>
#include <algorithm>

#include <Grappa.hpp>
#include "ForkJoin.hpp"
#include "GlobalAllocator.hpp"
#include "GlobalTaskJoiner.hpp"
#include "Array.hpp"
#include "Message.hpp"
#include "CompletionEvent.hpp"
#include "Statistics.hpp"

#include "LocaleSharedMemory.hpp"

#include <boost/test/unit_test.hpp>

DEFINE_int64( repeats, 1, "Repeats" );
DEFINE_int64( iterations, 1 << 30, "Iterations" );
DEFINE_int64( sizeA, 1024, "Size of array that gups increments" );
DEFINE_int64( outstanding, 1 << 10, "Number of outstanding requests" );
DEFINE_bool( validate, true, "Validate result" );
DEFINE_bool( rdma, false, "Use RDMA aggregator" );

DECLARE_string( load_balance );


//const int outstanding = 1 << 4;
//const int outstanding = 1 << 13;

template< typename T >
class ReuseMessage : public Grappa::Message<T> {
public:
  Grappa::impl::ReuseList< ReuseMessage > * list_;
  virtual ReuseMessage * get_next() { 
    // we know this is safe since the list only holds this message type.
    return static_cast<ReuseMessage*>(this->next_); 
  }
  virtual void set_next( ReuseMessage * next ) { this->next_ = next; }
protected:
  virtual void mark_sent() {
    DVLOG(5) << __func__ << ": " << this << " Marking sent with is_enqueued_=" << this->is_enqueued_ 
             << " is_delivered_=" << this->is_delivered_ 
             << " is_sent_=" << this->is_sent_;

    if( Grappa::mycore() == this->source_ ) {
      CHECK_EQ( Grappa::mycore(), this->source_ );
      list_->push(this);
    }

    Grappa::Message<T>::mark_sent();
  }
};


// completion message handler
struct C {
  GlobalAddress< Grappa::CompletionEvent > ce;
  void operator()() {
    DVLOG(5) << "Received completion at node " << Grappa::mycore()
             << " with target " << ce.node()
             << " address " << ce.pointer();
    ce.pointer()->complete();
  }
};

// list of free completion messages
Grappa::impl::ReuseList< ReuseMessage<C> > completion_list;
void completion_list_push( ReuseMessage<C> * c ) {
  completion_list.push(c);
}

// GUPS request message handler
struct M {
  // address to increment
  GlobalAddress< int64_t > addr;
  // address of completion counter
  GlobalAddress< Grappa::CompletionEvent > ce;

  // once we have a message to use, do this
  static void complete( ReuseMessage<C> * message, GlobalAddress< Grappa::CompletionEvent > event ) {
    message->reset();
    (*message)->ce = event;
    message->enqueue( event.node() );
  }

  void operator()() {
    DVLOG(5) << "Received GUP at node " << Grappa::mycore()
             << " with target " << addr.node()
             << " address " << addr.pointer()
             << " reply to " << ce.node()
             << " with address " << ce.pointer();

    // increment address
    int64_t * ptr = addr.pointer();
    *ptr++;

    // now send completion
    // is the completion local?
    if (ce.node() == Grappa::mycore()) {
      // yes, so just do it.
      ce.pointer()->complete();
    } else {
      // we need to send a message
      // try to grab a message
      ReuseMessage<C> * c = NULL;
      if( (c = completion_list.try_pop()) != NULL ) {
        // got one; send completion
        M::complete( c, ce );
      } else {
        // we need to block, so spawn a task
        auto my_ce = ce;
        Grappa::privateTask( [this, my_ce] { // this actually unused
            ReuseMessage<C> * c = completion_list.block_until_pop();
            M::complete( c, my_ce );
          });
      }
    }
  }
};

// list of free request messages
Grappa::impl::ReuseList< ReuseMessage<M> > message_list;
void message_list_push( ReuseMessage<M> * m ) {
  message_list.push(m);
}

// keep track of this core's completions
Grappa::CompletionEvent ce;



double wall_clock_time() {
  const double nano = 1.0e-9;
  timespec t;
  clock_gettime( CLOCK_MONOTONIC, &t );
  return nano * t.tv_nsec + t.tv_sec;
}

BOOST_AUTO_TEST_SUITE( Gups_tests );

LOOP_FUNCTION( func_start_profiling, index ) {
  Grappa_start_profiling();
}

LOOP_FUNCTION( func_stop_profiling, index ) {
  Grappa_stop_profiling();
  //Grappa::Statistics::print();
}

/// Functor to execute one GUP.
static GlobalAddress<int64_t> Array;
static int64_t * base;
LOOP_FUNCTOR( func_gups, index, ((GlobalAddress<int64_t>, _Array)) ) {
  Array = _Array;
  base = Array.localize();
}

void func_gups_x(int64_t * p) {
  const uint64_t LARGE_PRIME = 18446744073709551557UL;
  /* across all nodes and all calls, each instance of index in [0.. iterations)
  ** must be encounted exactly once */
  //uint64_t index = random() + Grappa_mynode();//(p - base) * Grappa_nodes() + Grappa_mynode();
  //uint64_t b = (index*LARGE_PRIME) % FLAGS_sizeA;

  static uint64_t index = 1;
  //uint64_t b = ((Grappa_mynode() + index++) *LARGE_PRIME) & 1023;
  uint64_t b = ((Grappa_mynode() + index++) *LARGE_PRIME) % FLAGS_sizeA;

  //fprintf(stderr, "%d ", b);
  ff_delegate_add( Array + b, (const int64_t &) 1 );
}
void validate(GlobalAddress<uint64_t> A, size_t n) {
  int total = 0, max = 0, min = INT_MAX;
  double sum_sqr = 0.0;
  for (int i = 0; i < n; i++) {
    //int tmp = Grappa_delegate_read_word(A+i);
    int tmp = Grappa::delegate::read(A+i);
    total += tmp;
    sum_sqr += tmp*tmp;
    max = tmp > max ? tmp : max;
    min = tmp < min ? tmp : min;
  }
  // fprintf(stderr, "Validation:  total updates %d; min %d; max %d; avg value %g; std dev %g\n",
  //         total, min, max, total/((double)n), sqrt(sum_sqr/n - ((total/n)*total/n)));
}

LOOP_FUNCTION( func_gups_rdma, index ) {
  //void func_gups_x(int64_t * p) {
  const uint64_t LARGE_PRIME = 18446744073709551557UL;
  const uint64_t each_iters = FLAGS_iterations / Grappa::cores();
  const uint64_t my_start = each_iters * Grappa::mycore();

  DVLOG(1) << "Initializing RDMA GUPS....";

  // record how many messages we plan to send
  ce.enroll( each_iters );
    
  DVLOG(1) << "Starting RDMA GUPS";
  for( uint64_t index = my_start; index < my_start + each_iters; ++index ) {
    DCHECK_LT( index, FLAGS_iterations ) << "index exploded!";
    
    // compute address to increment
    uint64_t b = (index * LARGE_PRIME) % FLAGS_sizeA;
    auto a = Array + b;

    // get a message to use to send
    ReuseMessage<M> * m = message_list.block_until_pop();

    // send
    m->reset();
    (*m)->addr = a;
    (*m)->ce = make_global( &ce );
    m->enqueue( a.node() );
    DVLOG(5) << "Sent GUP from node " << Grappa::mycore()
             << " to " << a.node()
             << " address " << a.pointer()
             << " reply to address " << &ce;

  }

  DVLOG(1) << "Done sending; now waiting for replies";
  ce.wait();

  DVLOG(1) << "Got all replies";
}


void user_main( int * args ) {

  //fprintf(stderr, "Entering user_main\n");
  func_start_profiling start_profiling;
  func_stop_profiling stop_profiling;

  //CHECK_EQ( FLAGS_sizeA, 1024 ) << "sizeA must be 1024 unless you switch back to mods";
  CHECK_EQ( FLAGS_load_balance, "none" ) << "load balancing must be disabled unless you change the iteration approach";

  GlobalAddress<int64_t> A = Grappa_typed_malloc<int64_t>(FLAGS_sizeA);

  //fprintf(stderr, "user_main: Allocated global array of %d integers\n", FLAGS_sizeA);
  //fprintf(stderr, "user_main: Will run %d iterations\n", FLAGS_iterations);

  func_gups gups( A );

  func_gups_rdma gups_rdma;

    double runtime = 0.0;
    double throughput = 0.0;
    int nnodes = atoi(getenv("SLURM_NNODES"));
    double throughput_per_node = 0.0;

    Grappa_add_profiling_value( &runtime, "runtime", "s", false, 0.0 );
    Grappa_add_profiling_integer( &FLAGS_iterations, "iterations", "it", false, 0 );
    Grappa_add_profiling_integer( &FLAGS_sizeA, "sizeA", "entries", false, 0 );
    Grappa_add_profiling_value( &throughput, "updates_per_s", "up/s", false, 0.0 );
    Grappa_add_profiling_value( &throughput_per_node, "updates_per_s_per_node", "up/s", false, 0.0 );

  do {

    LOG(INFO) << "Starting";
    Grappa_memset_local(A, 0, FLAGS_sizeA);
    LOG(INFO) << "Start profiling";
    fork_join_custom( &start_profiling );

    LOG(INFO) << "Do something";
    double start = wall_clock_time();
    fork_join_custom( &gups );
    //printf ("Yeahoow!\n");
    if( FLAGS_rdma ) {
      LOG(INFO) << "Starting RDMA";
      fork_join_custom( &gups_rdma );
    } else {
      forall_local <int64_t, func_gups_x> (A, FLAGS_iterations);
    }
    double end = wall_clock_time();

    fork_join_custom( &stop_profiling );

    runtime = end - start;
    throughput = FLAGS_iterations / runtime;

    throughput_per_node = throughput/nnodes;
    Grappa::Statistics::merge_and_print();

    if( FLAGS_validate ) {
      LOG(INFO) << "Validating....";
      validate(A, FLAGS_sizeA);
    }

    // LOG(INFO) << "GUPS: "
    //         << FLAGS_iterations << " updates at "
    //         << throughput << "updates/s ("
    //         << throughput/nnodes << " updates/s/node).";
   } while (FLAGS_repeats-- > 1);
  LOG(INFO) << "Done. ";
}


BOOST_AUTO_TEST_CASE( test1 ) {
    Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
		  &(boost::unit_test::framework::master_test_suite().argv) );
    Grappa_activate();

    // prepare pools of messages
    auto msgs = Grappa::impl::locale_shared_memory.segment.construct< ReuseMessage<M> >(boost::interprocess::anonymous_instance)[ FLAGS_outstanding ]();
    auto completions = Grappa::impl::locale_shared_memory.segment.construct< ReuseMessage<C> >(boost::interprocess::anonymous_instance)[ FLAGS_outstanding ]();
    for( int i; i < FLAGS_outstanding; ++i ) {
      msgs[i].list_ = &message_list;
      message_list.push( &msgs[i] );
      completions[i].list_ = &completion_list;
      completion_list.push( &completions[i] );
    }

    Grappa_run_user_main( &user_main, (int*)NULL );

    Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
