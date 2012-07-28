#include <SoftXMT.hpp>
#include "ForkJoin.hpp"
#include "GlobalAllocator.hpp"

#include <boost/test/unit_test.hpp>

DEFINE_bool( rotate, false, "Iterate through senders" );
DEFINE_bool( half, false, "Communicate bidirectionally, half cores sending, half receiving" );
DEFINE_bool( simul, false, "Communicate bidirectionally, sending and receiving simultaneously" );
DEFINE_bool( strong, true, "Strong scaling (iters per cores) or weak scaling (iters / core per core)?" );
DEFINE_bool( delegate, false, "Do delegate ops instead of calls" );
DEFINE_bool( delegate2, false, "Do delegate ops instead of calls" );
DEFINE_bool( gasnet, false, "Do gasnet ping test" );
DEFINE_int64( iterations, 1 << 30, "Iterations" );
DEFINE_int64( payload_size, 0, "Payload size" );
DEFINE_int64( sizeA, 1000, "Size of array that gups increments" );


double wall_clock_time() {
  const double nano = 1.0e-9;
  timespec t;
  clock_gettime( CLOCK_MONOTONIC, &t );
  return nano * t.tv_nsec + t.tv_sec;
}

BOOST_AUTO_TEST_SUITE( Gups_tests );

static int64_t value = 0;


void receive( char* arg, size_t size, void * payload, size_t payload_size ) {
  ++value;
}

int gasnet_ping_handle_ = -1;
void gasnet_ping_am( gasnet_token_t token, void * buf, size_t size ) {
  ++value;
}


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

char payload[4096];

LOOP_FUNCTOR( func_ping, index, ((int64_t, count)) ((int64_t, payload_size)) ) {
  if( SoftXMT_mynode() < SoftXMT_nodes() / 2 ) {
    // senders
    Node target = SoftXMT_mynode() + SoftXMT_nodes() / 2;
    LOG(INFO) << "Node " << SoftXMT_mynode() << " sending " << count << " messages to " << target;
    for( int i = 0; i < count; ++i ) {
      SoftXMT_call_on( target, &receive, payload, payload_size );
      if( FLAGS_rotate ) {
	++target;
	if( target == SoftXMT_nodes() ) target = SoftXMT_nodes() / 2;
      }
      //SoftXMT_yield();
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

LOOP_FUNCTOR( func_gasnet_ping, index, ((int64_t, count)) ((int64_t, payload_size)) ) {
  if( SoftXMT_mynode() < SoftXMT_nodes() / 2 ) {
    // senders
    Node target = SoftXMT_mynode() + SoftXMT_nodes() / 2;
    LOG(INFO) << "Node " << SoftXMT_mynode() << " sending " << count << " messages to " << target;
    for( int i = 0; i < count; ++i ) {
      GASNET_CHECK( gasnet_AMRequestMedium0( target, gasnet_ping_handle_, payload, payload_size ) );
      if( FLAGS_rotate ) {
	++target;
	if( target == SoftXMT_nodes() ) target = SoftXMT_nodes() / 2;
      }
      //SoftXMT_yield();
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

LOOP_FUNCTOR( func_delegate, index, ((int64_t, count)) ((int64_t, payload_size)) ) {
  if( SoftXMT_mynode() < SoftXMT_nodes() / 2 ) {
    // senders
    Node target = SoftXMT_mynode() + SoftXMT_nodes() / 2;
    LOG(INFO) << "Node " << SoftXMT_mynode() << " sending " << count << " messages to " << target;
    for( int i = 0; i < count; ++i ) {
      GlobalAddress< int64_t > global_value = make_global( &value, target );
      SoftXMT_delegate_fetch_and_add_word( global_value, 1 );
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

LOOP_FUNCTOR( func_delegate2, index, ((int64_t, payload_size)) ) {
  Node target = index % SoftXMT_nodes();
  GlobalAddress< int64_t > global_value = make_global( &value, target );
  SoftXMT_delegate_fetch_and_add_word( global_value, 1 );
}

LOOP_FUNCTOR( func_gups, index, ((GlobalAddress<int64_t>, Array)) ) {
  const uint64_t LARGE_PRIME = 18446744073709551557UL;
  uint64_t b = (index*LARGE_PRIME) % FLAGS_sizeA;
  SoftXMT_delegate_fetch_and_add_word( Array + b, 1 );
}

void user_main( int * args ) {
  value = 0;
  LOG(INFO) << "Value starts at " << value;

  func_start_profiling start_profiling;
  func_stop_profiling stop_profiling;

  GlobalAddress<int64_t> A = SoftXMT_typed_malloc<int64_t>(FLAGS_sizeA);

  func_gups gups=func_gups( A );

  fork_join_custom( &start_profiling );

  double start = wall_clock_time();
  fork_join( &gups, 1, FLAGS_iterations );
  double end = wall_clock_time();

  fork_join_custom( &stop_profiling );

  SoftXMT_dump_stats_all_nodes();

  double runtime = end - start;
  double throughput = FLAGS_iterations / runtime;

  LOG(INFO) << "GUPS: " << FLAGS_iterations << " updates at " << throughput << "updates/s.";
}


BOOST_AUTO_TEST_CASE( test1 ) {
    SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
		  &(boost::unit_test::framework::master_test_suite().argv) );
    SoftXMT_activate();

    SoftXMT_run_user_main( &user_main, (int*)NULL );

    SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
