
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "GlobalTaskJoiner.hpp"
#include "ForkJoin.hpp"
#include "AsyncParallelFor.hpp"
#include "Delegate.hpp"
#include "GlobalAllocator.hpp"

#include "PerformanceTools.hpp"


BOOST_AUTO_TEST_SUITE( AsyncParallelFor_Global_tests );

#define MAX_NODES 8
#define SIZE 1000000
int64_t done[SIZE*MAX_NODES] = {0};
int64_t count2 = 0;
int64_t global_count = 0;
int64_t local_count = 0;

#define N (1L<<10)

void loop_body(int64_t start, int64_t num ) {
  DVLOG(5) << "iters " << start << ", " << num;
  /*BOOST_MESSAGE( "execute [" << start << ","
            << start+num << ") with thread " << CURRENT_THREAD->id );*/
  BOOST_CHECK( num <= FLAGS_async_par_for_threshold );
  for (int i=start; i<start+num; i++) {
    int64_t marked = Grappa_delegate_fetch_and_add_word( make_global( &done[i], 0 ), 1 );
    BOOST_CHECK( marked == 0 );

    Grappa_delegate_fetch_and_add_word( make_global( &global_count, 0 ), 1 ); 
    local_count++;
  }
}

struct parallel_func : public ForkJoinIteration {
  void operator()(int64_t nid) const {
    int64_t start = nid * SIZE;
    int64_t iters = SIZE;

    global_joiner.reset();
    async_parallel_for<loop_body, joinerSpawn<loop_body, ASYNC_PAR_FOR_DEFAULT>, ASYNC_PAR_FOR_DEFAULT > ( start, iters ); 
    global_joiner.wait();
  }
};


// don't care what type goes into the GlobalAddress
typedef int64_t dummy_t;

void loop_body2(int64_t start, int64_t num, GlobalAddress<dummy_t> shared_arg_packed) {
  /*BOOST_MESSAGE( "execute [" << start << ","
            << start+num << ") with thread " << CURRENT_THREAD->id );*/
  BOOST_CHECK( num <= FLAGS_async_par_for_threshold );
  int64_t origin = reinterpret_cast<int64_t>(shared_arg_packed.pointer());
  for (int i=start; i<start+num; i++) {
    Grappa_delegate_fetch_and_add_word( make_global( &count2, origin ), 1 );

    Grappa_delegate_fetch_and_add_word( make_global( &global_count, 0 ), 1 ); 
    local_count++;
  }
}

LOOP_FUNCTION(check_count2, nid) {
  BOOST_CHECK( count2 == SIZE );
}

struct parallel_func2 : public ForkJoinIteration {
  // called on all nodes in parallel by "fork_join_custom"
  void operator()(int64_t nid) const {
    local_count = 0;

    int64_t start = nid * SIZE;
    int64_t iters = SIZE;

    // pack an argument into the global address (hack).
    // This is useful if we need just the node() from the global address and want to use the other
    // bits for something else.
    int64_t shared_arg = Grappa_mynode(); 
    GlobalAddress<dummy_t> shared_arg_packed = make_global( reinterpret_cast<dummy_t*>(shared_arg) );

    // get ready for parallel phase
    global_joiner.reset();

    // do loop in parallel with recursive decomposition
    // creates publicly stealable tasks
    async_parallel_for<dummy_t,loop_body2,joinerSpawn_hack<dummy_t,loop_body2,ASYNC_PAR_FOR_DEFAULT>,ASYNC_PAR_FOR_DEFAULT >(start,iters,shared_arg_packed);

    // wait for all of the loop iterations to complete on all nodes
    global_joiner.wait();
  }
};


  
LOOP_FUNCTION(func_enable_google_profiler, nid) {
  Grappa_start_profiling();
}
void profile_start() {
#ifdef GOOGLE_PROFILER
  func_enable_google_profiler g;
  fork_join_custom(&g);
#endif
}
LOOP_FUNCTION(func_disable_google_profiler, nid) {
  Grappa_stop_profiling();
}
void profile_stop() {
#ifdef GOOGLE_PROFILER
  func_disable_google_profiler g;
  fork_join_custom(&g);
#endif
}

LOOP_FUNCTION( no_work_func, nid ) {
    global_joiner.reset();
    /* no work */
    global_joiner.wait();
}

void signaling_task( int64_t * arg ) {
  global_joiner.signal();
}

/// Test one task doing work
LOOP_FUNCTION( one_work_func, nid ) {
    global_joiner.reset();
    global_joiner.registerTask();
    int64_t ignore;
    Grappa_privateTask( &signaling_task, &ignore );
    global_joiner.wait();
}

/// Verify global joiner doesn't wait if all the work is done when it's called
LOOP_FUNCTION( one_self_work_func, nid ) {
    global_joiner.reset();

    // register and signal
    global_joiner.registerTask();
    global_joiner.signal();

    global_joiner.wait();
}

LOOP_FUNCTOR( ff_test_func, nid, ((GlobalAddress<double>,array)) ) {
  global_joiner.reset();

  range_t r = blockDist(0, N, nid, Grappa_nodes());

  for (int64_t i=r.start; i<r.end; i++) {
    ff_delegate_write(array+i, (double)i);
  }

  global_joiner.wait();
}

void user_main( void * args ) {
  
  BOOST_CHECK( Grappa_nodes() <= MAX_NODES );
  
  int64_t total_iters = Grappa_nodes() * SIZE;
  BOOST_MESSAGE( total_iters << " iterations over " <<
                 Grappa_nodes() << " nodes" );

  BOOST_MESSAGE( "Test default" );
  {

  // add sampling of global_count
  Grappa_add_profiling_counter( (uint64_t*)&global_count, "global count", "globcnt", false, 0 );
  
  parallel_func f;
  
  Grappa_reset_stats_all_nodes();
  profile_start();
  fork_join_custom(&f);
  profile_stop();

  for (int i=0; i<total_iters; i++) {
    BOOST_CHECK( done[i] == 1 );
  }
  BOOST_CHECK( global_count == total_iters );

  // print out individual participations 
  for (Node n=0; n<Grappa_nodes(); n++) {
    int64_t n_count = Grappa_delegate_read_word( make_global( &local_count, n ) );
    BOOST_MESSAGE( n << " did " << n_count << " iterations" );
  }
  }

  BOOST_MESSAGE( "No work test" );
  {
    no_work_func f;
    fork_join_custom(&f);
  }

  BOOST_MESSAGE( "One work test" );
  {
    one_work_func f;
    fork_join_custom(&f);
  }
  
  BOOST_MESSAGE( "One self work test" );
  {
    one_self_work_func f;
    fork_join_custom(&f);
  }


  BOOST_MESSAGE( "Test with shared arg" );
  {
  // with shared arg
  global_count = 0;
  parallel_func2 f; 
  profile_start();
  fork_join_custom(&f);
  profile_stop();

  // check all iterations happened
  BOOST_CHECK( global_count == total_iters );

  // check all shared args used properly
  check_count2 ff;
  fork_join_custom(&ff);

  // print out individual participations 
  for (Node n=0; n<Grappa_nodes(); n++) {
    int64_t n_count = Grappa_delegate_read_word( make_global( &local_count, n ) );
    BOOST_MESSAGE( n << " did " << n_count << " iterations" );
  }
  }

  BOOST_MESSAGE("testing feed-forward delegates");
  {
    GlobalAddress<double> array = Grappa_typed_malloc<double>(N);
    { ff_test_func f(array); fork_join_custom(&f); }
    
    for (int64_t i=0; i<N; i++) {
      double d;
      Grappa_delegate_read(array+i, &d);
      BOOST_CHECK( d == (double)i );
    }
  }

  BOOST_MESSAGE( "user main is exiting" );
  Grappa_dump_stats_all_nodes();
}



BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  Grappa_activate();

  DVLOG(1) << "Spawning user main Thread....";
  Grappa_run_user_main( &user_main, (void*)NULL );
  VLOG(5) << "run_user_main returned";
  CHECK( Grappa_done() );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

