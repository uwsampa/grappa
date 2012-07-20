
#include <SoftXMT.hpp>
#include "ForkJoin.hpp"

#include <boost/test/unit_test.hpp>

DEFINE_bool( half, false, "Communicate bidirectionally, half cores sending, half receiving" );
DEFINE_bool( simul, false, "Communicate bidirectionally, sending and receiving simultaneously" );
DEFINE_bool( strong, true, "Strong scaling (iters per cores) or weak scaling (iters / core per core)?" );
DEFINE_int64( iterations, 1 << 26, "Iterations" );


double wall_clock_time() {
  const double nano = 1.0e-9;
  timespec t;
  clock_gettime( CLOCK_MONOTONIC, &t );
  return nano * t.tv_nsec + t.tv_sec;
}

BOOST_AUTO_TEST_SUITE( NetworkPerformance_tests );

static int64_t value = 0;

void receive( char* arg, size_t size, void * payload, size_t payload_size ) {
  ++value;
}
// void receive( int64_t* arg, size_t size, void * payload, size_t payload_size ) {
//   ++value;
// }

LOOP_FUNCTION( func_start_profiling, index ) {
  SoftXMT_start_profiling();
}

LOOP_FUNCTION( func_stop_profiling, index ) {
  SoftXMT_stop_profiling();
}


bool send_or_receive[ 1024 ] = { false };
int targets[ 1024 ] = { 0 };

LOOP_FUNCTOR( func_half_ping, index, ((int64_t, count)) ((int64_t, payload_size)) ) {
  char foo[64] = { 0 };
  char * hostname = getenv("SLURM_TOPOLOGY_ADDR");
  Node target = targets[ index ];

  if( send_or_receive[ index ] == true ) {
    // senders
    LOG(INFO) << "Node " << SoftXMT_mynode() << " (" << hostname << ") sending " << count << " messages to " << target;
    for( int i = 0; i < count; ++i ) {
      SoftXMT_call_on( target, &receive, foo, 0 );
    }
    SoftXMT_flush( target );
    value = count;
  } else {
    LOG(INFO) << "Node " << SoftXMT_mynode() << " (" << hostname << ") receiving " << count << " messages from " << target;
    // receivers
    while( value != count ) SoftXMT_yield();
    LOG(INFO) << "Node " << SoftXMT_mynode() << " received " << value << " messages";
  }
  BOOST_CHECK( value == count );
}

LOOP_FUNCTOR( func_ping, index, ((int64_t, count)) ((int64_t, payload_size)) ) {
  if( SoftXMT_mynode() < SoftXMT_nodes() / 2 ) {
    // senders
    Node target = SoftXMT_mynode() + SoftXMT_nodes() / 2;
    LOG(INFO) << "Node " << SoftXMT_mynode() << " sending " << count << " messages to " << target;
    for( int i = 0; i < count; ++i ) {
      SoftXMT_call_on( target, &receive, (char*)0, 0 );
      ++target;
      if( target == SoftXMT_nodes() ) target = SoftXMT_nodes() / 2;
      //if( (i & 0x0) == 0 ) SoftXMT_yield();
      SoftXMT_yield();
    }
    // for( int i = SoftXMT_nodes() / 2; i < SoftXMT_nodes(); ++i ) {
    //   SoftXMT_flush( i );
    // }
    value = count;
    LOG(INFO) << "Node " << SoftXMT_mynode() << " sent " << count << " messages to " << target;
  } else {
    // receivers
    while( value != count ) SoftXMT_yield();
    LOG(INFO) << "Node " << SoftXMT_mynode() << " received " << value << " messages";
  }
  BOOST_CHECK( value == count );
}

LOOP_FUNCTOR( func_bidir_ping, index, ((int64_t, count)) ((int64_t, payload_size)) ) {
  // senders
  //Node target = (SoftXMT_nodes() - 1) - SoftXMT_mynode();
  
  //LOG(INFO) << "Node " << SoftXMT_mynode() << " sending " << count << " messages to " << target;

  Node target = SoftXMT_mynode();
  for( int i = 0; i < count; ++i ) {
    SoftXMT_call_on( target, &receive, (char*)0, 0 );
    target = target + 1;
    if( target >= SoftXMT_nodes() ) target = 0;
    //if( (i & 0xff) == 0 ) SoftXMT_yield();
    SoftXMT_yield();
  }
  //SoftXMT_flush( target );  
  LOG(INFO) << "Sent " << count << " messages";

  // receivers
  while( value != count ) SoftXMT_yield();

  LOG(INFO) << "Node " << SoftXMT_mynode() << " received " << value << " messages";

  BOOST_CHECK( value == count );
}


void user_main( int * args ) {
  value = 0;
  LOG(INFO) << "Value starts at " << value;

  func_start_profiling start_profiling;
  func_stop_profiling stop_profiling;

  
  func_ping ping( FLAGS_strong ? FLAGS_iterations : FLAGS_iterations / SoftXMT_nodes(), 0 );
  func_bidir_ping bidir_ping( FLAGS_strong ? FLAGS_iterations : FLAGS_iterations / SoftXMT_nodes(), 0 );
  func_half_ping half_ping( FLAGS_strong ? FLAGS_iterations : FLAGS_iterations / SoftXMT_nodes(), 0 );

  fork_join_custom( &start_profiling );

  double start = wall_clock_time();
  if( FLAGS_simul ) {
    fork_join_custom( &bidir_ping );
  } else if( FLAGS_half ){
    fork_join_custom( &half_ping );
  } else {
    fork_join_custom( &ping );
  }
  double end = wall_clock_time();

  fork_join_custom( &stop_profiling );

  SoftXMT_dump_stats_all_nodes();

  double runtime = end - start;
  double throughput = (SoftXMT_nodes() / 2) * FLAGS_iterations / runtime;
  if( FLAGS_half ) throughput /= 4.0;

  LOG(INFO) << "Value finishes at " << value 
	    << " in " << runtime 
	    << "s == " << throughput << "it/s per node";
}


BOOST_AUTO_TEST_CASE( test1 ) {
    SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
            &(boost::unit_test::framework::master_test_suite().argv) );
    SoftXMT_activate();

    for( int i = 0; i < SoftXMT_nodes(); ++i ) {
      targets[ i ] = (SoftXMT_nodes() - 1) - SoftXMT_mynode();
      send_or_receive[ i ] = i & 0x1;
    }

    SoftXMT_run_user_main( &user_main, (int*)NULL );

    SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
