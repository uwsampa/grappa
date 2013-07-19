
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
#include "Collective.hpp"

#include "LocaleSharedMemory.hpp"

#include <boost/test/unit_test.hpp>

DEFINE_int64( repeats, 1, "Repeats" );
DEFINE_int64( iterations, 1 << 30, "Iterations" );
DEFINE_int64( sizeA, 1024, "Size of array that gups increments" );
DEFINE_int64( outstanding, 1 << 10, "Number of outstanding requests" );
DEFINE_bool( validate, true, "Validate result" );
DEFINE_bool( rdma, true, "Use RDMA aggregator" );

DECLARE_string( load_balance );

GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, gups_requests_sent, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, gups_completions_sent, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, gups_requests_received, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, gups_completions_received, 0 );
GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, gups_completions_blocked, 0 );

GRAPPA_DECLARE_STAT( SummarizingStatistic<int64_t>, rdma_message_bytes );

GRAPPA_DEFINE_STAT( SimpleStatistic<int64_t>, gups_total_bytes, 0 );

uint64_t * requests_sent = NULL;
uint64_t * completions_sent = NULL;

int64_t * completions_to_send = NULL;
bool done_sending = false;

double bytes_sent = 0.0;

//const int outstanding = 1 << 4;
//const int outstanding = 1 << 13;

template< typename T >
class GupsReuseMessage : public Grappa::Message<T> {
public:
  Grappa::impl::ReuseList< GupsReuseMessage > * list_;
  virtual GupsReuseMessage * get_next() { 
    // we know this is safe since the list only holds this message type.
    return static_cast<GupsReuseMessage*>(this->next_); 
  }
  virtual void set_next( GupsReuseMessage * next ) { this->next_ = next; }
protected:
  virtual void mark_sent() {
    DVLOG(5) << __func__ << ": " << this << " Marking sent with is_enqueued_=" << this->is_enqueued_ 
             << " is_delivered_=" << this->is_delivered_ 
             << " is_sent_=" << this->is_sent_;

    Grappa::Message<T>::mark_sent();

    if( Grappa::mycore() == this->source_ ) {
      CHECK_EQ( Grappa::mycore(), this->source_ );
      list_->push(this);
    }
  }
} __attribute__((aligned(64)));


// completion message handler
struct C {
  GlobalAddress< Grappa::CompletionEvent > rce;
  int64_t completions_for_here;
  void operator()() {
    DVLOG(5) << "Received completion at node " << Grappa::mycore()
             << " with target " << rce.node()
             << " address " << rce.pointer();
    rce.pointer()->complete( completions_for_here );
    gups_completions_received++;
  }
};

// list of free completion messages
Grappa::impl::ReuseList< GupsReuseMessage<C> > completion_list;
void completion_list_push( GupsReuseMessage<C> * c ) {
  completion_list.push(c);
}

// GUPS request message handler
struct M {
  // address to increment
  GlobalAddress< int64_t > addr;

  // address of completion counter
  GlobalAddress< Grappa::CompletionEvent > rce;
  
  // number of completions for this node
  int64_t completions_for_here;

  // once we have a message to use, do this
  static void complete( GupsReuseMessage<C> * message, GlobalAddress< Grappa::CompletionEvent > event ) {
    gups_completions_sent++;
    message->reset();
    (*message)->rce = event;
    (*message)->completions_for_here = 1;
    message->enqueue( event.node() );
    completions_sent[ event.node() ]++;
  }

  void operator()() {
    DVLOG(5) << "Received GUP at node " << Grappa::mycore()
             << " with target " << addr.node()
             << " address " << addr.pointer()
             << " reply to " << rce.node()
             << " with address " << rce.pointer();
    gups_requests_received++;

    // increment address
    int64_t * ptr = addr.pointer();
    *ptr++;

    // deliver any batched completions
    rce.pointer()->complete( completions_for_here );

    if( !done_sending ) {
      // remember to send completion
      completions_to_send[ rce.node() ]++;
    } else {
      // is the completion local?
      if (rce.node() == Grappa::mycore()) {
        // yes, so just do it.
        rce.pointer()->complete();
        gups_completions_sent++;
        completions_sent[ rce.node() ]++;
      } else {
        // we need to send a message
        // try to grab a message
        GupsReuseMessage<C> * c = NULL;
        if( (c = completion_list.try_pop()) != NULL ) {
          // got one; send completion
          M::complete( c, rce );
        } else {
          gups_completions_blocked++;
          // we need to block, so spawn a task
          auto my_ce = rce;
          Grappa::privateTask( [this, my_ce] { // this actually unused
              GupsReuseMessage<C> * c = completion_list.block_until_pop();
              M::complete( c, my_ce );
            });
        }
      }
    }
  }
};

// list of free request messages
Grappa::impl::ReuseList< GupsReuseMessage<M> > message_list;
void message_list_push( GupsReuseMessage<M> * m ) {
  message_list.push(m);
}

// keep track of this core's completions
Grappa::CompletionEvent ce;

void gups_dump_counts() {
  LOG(INFO) << "Completion count " << ce.get_count()
            << " message_list size " << message_list.count()
            << " completion_list size " << completion_list.count();
}


double wall_clock_time() {
  const double nano = 1.0e-9;
  timespec t;
  clock_gettime( CLOCK_MONOTONIC, &t );
  return nano * t.tv_nsec + t.tv_sec;
}

BOOST_AUTO_TEST_SUITE( Gups_tests );

LOOP_FUNCTION( func_start_profiling, index ) {
  Grappa::Statistics::reset();
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
void validate(GlobalAddress<int64_t> A, size_t n) {
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

  //exit(123);

  DVLOG(1) << "Initializing RDMA GUPS....";

  // record how many messages we plan to send
  ce.enroll( each_iters );
  done_sending = false;
    
  DVLOG(1) << "Starting RDMA GUPS";
  for( uint64_t index = my_start; index < my_start + each_iters; ++index ) {
    DCHECK_LT( index, FLAGS_iterations ) << "index exploded!";
    
    // compute address to increment
    uint64_t b = (index * LARGE_PRIME) % FLAGS_sizeA;
    auto a = Array + b;

    // get a message to use to send
    GupsReuseMessage<M> * m = message_list.block_until_pop();

    // send
    m->reset();

    // address to increment
    (*m)->addr = a;
    (*m)->rce = make_global( &ce );

    // send buffered completions
    (*m)->completions_for_here = completions_to_send[ a.node() ];
    completions_to_send[ a.node() ] = 0;
    
    m->enqueue( a.node() );
    gups_requests_sent++;
    requests_sent[a.node()]++;
    DVLOG(5) << "Sent GUP from node " << Grappa::mycore()
             << " to " << a.node()
             << " address " << a.pointer()
             << " reply to address " << &ce;

  }

  done_sending = true;

  // now flush remaining completions
  // (any new messages that arrive now will generate their own messages)
  for( Core core = 0; core < Grappa::cores(); ++core ) {
    if( completions_to_send[ core ] > 0 ) {
      GupsReuseMessage<C> * c = completion_list.block_until_pop();
      
      if( completions_to_send[ core ] == 0 ) {
        completion_list.push( c );
        break;
      }

      c->reset();
      
      (*c)->rce = make_global( &ce, core ); // generate address of completion event
      (*c)->completions_for_here = completions_to_send[ core ];
      completions_to_send[ core ] = 0;
      
      c->enqueue( core );
      
      gups_completions_sent++;
      completions_sent[ core ]++;
      
      Grappa::impl::global_rdma_aggregator.flush( core );
    }
  };

  DVLOG(1) << "Done sending; now waiting for replies";
  ce.wait();

  DVLOG(1) << "Got all replies";
  Grappa_barrier_suspending();
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
    double bandwidth_per_node = 0.0;

    Grappa_add_profiling_value( &runtime, "runtime", "s", false, 0.0 );
    Grappa_add_profiling_integer( &FLAGS_iterations, "iterations", "it", false, 0 );
    Grappa_add_profiling_integer( &FLAGS_sizeA, "sizeA", "entries", false, 0 );
    Grappa_add_profiling_value( &throughput, "updates_per_s", "up/s", false, 0.0 );
    Grappa_add_profiling_value( &throughput_per_node, "updates_per_s_per_node", "up/s", false, 0.0 );
    Grappa_add_profiling_value( &bandwidth_per_node, "bytes_per_s_per_node", "B/s", false, 0.0 );

  do {

    LOG(INFO) << "Starting";
    Grappa_memset_local(A, 0, FLAGS_sizeA);
    LOG(INFO) << "Do something";
    fork_join_custom( &gups );
    //printf ("Yeahoow!\n");

    LOG(INFO) << "Start profiling";
    //fork_join_custom( &start_profiling );
    Grappa::Statistics::reset_all_cores();

    double start = wall_clock_time();
    Grappa::on_all_cores( [] {
        bytes_sent = -rdma_message_bytes.value();
      } );
    
    if( FLAGS_rdma ) {
      LOG(INFO) << "Starting RDMA";
      fork_join_custom( &gups_rdma );
    } else {
      forall_local <int64_t, func_gups_x> (A, FLAGS_iterations);
    }
    int64_t final_bytes = rdma_message_bytes.value();
    double end = wall_clock_time();

    fork_join_custom( &stop_profiling );

    runtime = end - start;
    throughput = FLAGS_iterations / runtime;

    throughput_per_node = throughput/nnodes;
    Grappa::on_all_cores( [] {
        bytes_sent += rdma_message_bytes.value();
        double total_bytes_sent = Grappa::allreduce< size_t, collective_add >(bytes_sent);
        bytes_sent = total_bytes_sent;
      } );
    
    bandwidth_per_node = bytes_sent / runtime / nnodes;

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

    requests_sent = new uint64_t[ Grappa::cores() ];
    completions_sent = new uint64_t[ Grappa::cores() ];
    completions_to_send = new int64_t[ Grappa::cores() ];
    for( int i = 0; i < Grappa::cores(); ++i ) {
      requests_sent[i] = 0;
      completions_sent[i] = 0;
      completions_to_send[i] = 0;
    }

    // prepare pools of messages
    auto msgs = Grappa::impl::locale_shared_memory.segment.construct< GupsReuseMessage<M> >(boost::interprocess::anonymous_instance)[ FLAGS_outstanding ]();
    auto completions = Grappa::impl::locale_shared_memory.segment.construct< GupsReuseMessage<C> >(boost::interprocess::anonymous_instance)[ FLAGS_outstanding ]();

    for( int i = 0; i < FLAGS_outstanding; ++i ) {
      msgs[i].list_ = &message_list;
      message_list.push( &msgs[i] );
      completions[i].list_ = &completion_list;
      completion_list.push( &completions[i] );
    }

    Grappa_run_user_main( &user_main, (int*)NULL );

    Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
