
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <SoftXMT.hpp>
#include "ForkJoin.hpp"

#include <boost/test/unit_test.hpp>

DEFINE_bool( rotate, false, "Iterate through senders" );
DEFINE_bool( strong, false, "Strong scaling (iters per cores) or weak scaling (iters / core per core)?" );

DEFINE_bool( delegate, false, "Do delegate ops instead of calls" );
DEFINE_bool( bidir, false, "Communicate bidirectionally, sending and receiving simultaneously" );

DEFINE_bool( gasnet, false, "Do gasnet ping test" );

DEFINE_int64( iterations, 1 << 26, "Iterations" );
DEFINE_int64( payload_size, 0, "Payload size" );
DEFINE_int64( yield_mask, 0, "Yield mask" );




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
      if( (i & FLAGS_yield_mask) == 0 ) SoftXMT_yield();
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
      if( (i & FLAGS_yield_mask) == 0 ) SoftXMT_yield();
    }
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
  Node target = 0;
  if( FLAGS_rotate )
    target = SoftXMT_mynode();
  else 
    target = (SoftXMT_nodes() - 1) - SoftXMT_mynode();
  
  //LOG(INFO) << "Node " << SoftXMT_mynode() << " sending " << count << " messages to " << target;

  for( int i = 0; i < count; ++i ) {
    SoftXMT_call_on( target, &receive, (char*)0, 0 );
    if( FLAGS_rotate ) {
      target = target + 1;
      if( target >= SoftXMT_nodes() ) target = 0;
    }
    if( (i & FLAGS_yield_mask) == 0 ) SoftXMT_yield();
  }
  //SoftXMT_flush( target );  
  LOG(INFO) << "Sent " << count << " messages";

  // receivers
  while( value != count ) SoftXMT_yield();

  LOG(INFO) << "Node " << SoftXMT_mynode() << " received " << value << " messages";

  BOOST_CHECK( value == count );
}

LOOP_FUNCTOR( func_delegate_half, index, ((int64_t, count)) ((int64_t, payload_size)) ) {
  if( SoftXMT_mynode() < SoftXMT_nodes() / 2 ) {
    // senders
    Node target = SoftXMT_mynode() + SoftXMT_nodes() / 2;
    LOG(INFO) << "Node " << SoftXMT_mynode() << " sending " << count << " messages to " << target;
    for( int i = 0; i < count; ++i ) {
      GlobalAddress< int64_t > global_value = make_global( &value, target );
      SoftXMT_delegate_fetch_and_add_word( global_value, 1 );
      if( FLAGS_rotate ) {
	++target;
	if( target == SoftXMT_nodes() ) target = SoftXMT_nodes() / 2;
      }
      if( (i & FLAGS_yield_mask) == 0 ) SoftXMT_yield();
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

LOOP_FUNCTOR( func_delegate, index, ((int64_t, payload_size)) ) {
  Node target = 0;
  if( FLAGS_rotate )
    target = index % SoftXMT_nodes();
  else 
    target = (SoftXMT_nodes() - 1) - SoftXMT_mynode();
  GlobalAddress< int64_t > global_value = make_global( &value, target );
  //SoftXMT_call_on( target, &receive, payload, payload_size );
  SoftXMT_delegate_fetch_and_add_word( global_value, 1 );
  if( (index & FLAGS_yield_mask) == 0 ) SoftXMT_yield();
}

void user_main( int * args ) {
  value = 0;
  LOG(INFO) << "Value starts at " << value;

  func_start_profiling start_profiling;
  func_stop_profiling stop_profiling;

  int nnode = atoi(getenv("SLURM_NNODES"));
  int cores_per_node = atoi(getenv("SLURM_NTASKS")) / nnode;
  int active_cores = SoftXMT_nodes();

  int64_t iterations = FLAGS_strong ? FLAGS_iterations : FLAGS_iterations / SoftXMT_nodes();

  // one-sided ops, single destination
  // one-sided ops, random destinations
  // delegate ops, single destination
  // delegate ops, random destinations


  fork_join_custom( &start_profiling );
  double start = wall_clock_time();
  if( FLAGS_gasnet ) {
    func_gasnet_ping gasnet_ping( iterations, FLAGS_payload_size );
    fork_join_custom( &gasnet_ping );
  } else {
    if( FLAGS_delegate ) {
      if( FLAGS_bidir ) {
	iterations = FLAGS_strong ? FLAGS_iterations : FLAGS_iterations / (SoftXMT_nodes() / 2);
	active_cores = SoftXMT_nodes() / 2;
	func_delegate_half delegate_half( iterations, FLAGS_payload_size );
	fork_join_custom( &delegate_half );
      } else {
	iterations = FLAGS_strong ? FLAGS_iterations * SoftXMT_nodes() : FLAGS_iterations;
	active_cores = SoftXMT_nodes();
	func_delegate delegate( FLAGS_payload_size );
	fork_join( &delegate, 0, iterations );
      }
    } else {
      if( FLAGS_bidir ) {
	iterations = FLAGS_strong ? FLAGS_iterations : FLAGS_iterations / SoftXMT_nodes();
	active_cores = SoftXMT_nodes();
	func_bidir_ping bidir_ping( iterations, FLAGS_payload_size );
	fork_join_custom( &bidir_ping );
      } else {
	iterations = FLAGS_strong ? FLAGS_iterations : FLAGS_iterations / (SoftXMT_nodes() / 2);
	active_cores = SoftXMT_nodes() / 2;
	func_ping ping( iterations, FLAGS_payload_size );
	fork_join_custom( &ping );
      }
    }
  }
  double end = wall_clock_time();
  fork_join_custom( &stop_profiling );

  SoftXMT_merge_and_dump_stats();
  //SoftXMT_dump_stats_all_nodes();

  double runtime = end - start;
  double throughput = FLAGS_strong ? SoftXMT_nodes() * FLAGS_iterations / runtime : FLAGS_iterations / runtime;
  double throughput_per_node = throughput / (active_cores / cores_per_node);
  double bandwidth = FLAGS_payload_size * throughput;
  double bandwidth_per_node = bandwidth / (active_cores / cores_per_node);

  std::cout << "Flags { "
	    << " FLAGS_rotate: " << FLAGS_rotate << ","
	    << " FLAGS_strong: " << FLAGS_strong << ","
	    << " FLAGS_delegate: " << FLAGS_delegate << ","
	    << " FLAGS_bidir: " << FLAGS_bidir << ","
	    << " FLAGS_gasnet: " << FLAGS_gasnet << ","
	    << " FLAGS_iterations: " << FLAGS_iterations << ","
	    << " FLAGS_payload_size: " << FLAGS_payload_size << ","
	    << " FLAGS_yield_mask: " << FLAGS_yield_mask << ","
	    << " }"
	    << std::endl;
  
  std::cout << "Result { "
	    << " value: " << value << ","
	    << " runtime: " << runtime  << ","
	    << " iterations_per_s: " << throughput << ","
	    << " iterations_per_s_per_node: " << throughput_per_node << ","
	    << " MB_per_s: " << bandwidth/(1<<20) << ","
	    << " MB_per_s_per_node: " << bandwidth_per_node/(1<<20) << ","
	    << " }"
	    << std::endl;
}


BOOST_AUTO_TEST_CASE( test1 ) {
    SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
		  &(boost::unit_test::framework::master_test_suite().argv),
		  (1 << 22) );
    gasnet_ping_handle_ = global_communicator.register_active_message_handler( &gasnet_ping_am );
    SoftXMT_activate();

    SoftXMT_run_user_main( &user_main, (int*)NULL );

    SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
