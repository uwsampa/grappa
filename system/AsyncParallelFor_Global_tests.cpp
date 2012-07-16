#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
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
  /*BOOST_MESSAGE( "execute [" << start << ","
            << start+num << ") with thread " << CURRENT_THREAD->id );*/
  BOOST_CHECK( num <= FLAGS_async_par_for_threshold );
  for (int i=start; i<start+num; i++) {
    int64_t marked = SoftXMT_delegate_fetch_and_add_word( make_global( &done[i], 0 ), 1 );
    BOOST_CHECK( marked == 0 );

    SoftXMT_delegate_fetch_and_add_word( make_global( &global_count, 0 ), 1 ); 
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
    SoftXMT_delegate_fetch_and_add_word( make_global( &count2, origin ), 1 );

    SoftXMT_delegate_fetch_and_add_word( make_global( &global_count, 0 ), 1 ); 
    local_count++;
  }
}

LOOP_FUNCTION(check_count2, nid) {
  BOOST_CHECK( count2 == SIZE );
}

struct parallel_func2 : public ForkJoinIteration {
  void operator()(int64_t nid) const {
    local_count = 0;

    int64_t start = nid * SIZE;
    int64_t iters = SIZE;

    // pack an argument into the global address (hack).
    // This is useful if we need just the node() from the global address and want to use the other
    // bits for something else.
    int64_t shared_arg = SoftXMT_mynode(); 
    GlobalAddress<dummy_t> shared_arg_packed = make_global( reinterpret_cast<dummy_t*>(shared_arg) );

    global_joiner.reset();
    async_parallel_for<dummy_t,loop_body2,joinerSpawn_hack<dummy_t,loop_body2,ASYNC_PAR_FOR_DEFAULT>,ASYNC_PAR_FOR_DEFAULT >(start,iters,shared_arg_packed);
    global_joiner.wait();
  }
};


  
LOOP_FUNCTION(func_enable_google_profiler, nid) {
  SoftXMT_start_profiling();
}
void profile_start() {
#ifdef GOOGLE_PROFILER
  func_enable_google_profiler g;
  fork_join_custom(&g);
#endif
}
LOOP_FUNCTION(func_disable_google_profiler, nid) {
  SoftXMT_stop_profiling();
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

LOOP_FUNCTION( one_work_func, nid ) {
    global_joiner.reset();
    global_joiner.registerTask();
    int64_t ignore;
    SoftXMT_privateTask( &signaling_task, &ignore );
    global_joiner.wait();
}

LOOP_FUNCTION( one_self_work_func, nid ) {
    global_joiner.reset();

    // register and signal
    global_joiner.registerTask();
    global_joiner.signal();

    global_joiner.wait();
}

LOOP_FUNCTOR( ff_test_func, nid, ((GlobalAddress<double>,array)) ) {
  global_joiner.reset();

  range_t r = blockDist(0, N, nid, SoftXMT_nodes());

  for (int64_t i=r.start; i<r.end; i++) {
    ff_delegate_write(array+i, (double)i);
  }

  global_joiner.wait();
}

void user_main( void * args ) {
  BOOST_CHECK( SoftXMT_nodes() <= MAX_NODES );
  
  int64_t total_iters = SoftXMT_nodes() * SIZE;
  BOOST_MESSAGE( total_iters << " iterations over " <<
                 SoftXMT_nodes() << " nodes" );

  BOOST_MESSAGE( "Test default" );
  {

  // add sampling of global_count
  SoftXMT_add_profiling_counter( (uint64_t*)&global_count, "global count", "globcnt", false, 0 );
  
  parallel_func f;
  
  SoftXMT_reset_stats_all_nodes();
  profile_start();
  fork_join_custom(&f);
  profile_stop();

  for (int i=0; i<total_iters; i++) {
    BOOST_CHECK( done[i] == 1 );
  }
  BOOST_CHECK( global_count == total_iters );

  // print out individual participations 
  for (Node n=0; n<SoftXMT_nodes(); n++) {
    int64_t n_count = SoftXMT_delegate_read_word( make_global( &local_count, n ) );
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
  for (Node n=0; n<SoftXMT_nodes(); n++) {
    int64_t n_count = SoftXMT_delegate_read_word( make_global( &local_count, n ) );
    BOOST_MESSAGE( n << " did " << n_count << " iterations" );
  }
  }

  BOOST_MESSAGE("testing feed-forward delegates");
  {
    GlobalAddress<double> array = SoftXMT_typed_malloc<double>(N);
    { ff_test_func f(array); fork_join_custom(&f); }
    
    for (int64_t i=0; i<N; i++) {
      double d;
      SoftXMT_delegate_read(array+i, &d);
      BOOST_CHECK( d == (double)i );
    }
  }

  BOOST_MESSAGE( "user main is exiting" );
  SoftXMT_dump_stats_all_nodes();
}



BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();

  DVLOG(1) << "Spawning user main Thread....";
  SoftXMT_run_user_main( &user_main, (void*)NULL );
  VLOG(5) << "run_user_main returned";
  CHECK( SoftXMT_done() );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

