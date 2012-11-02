
#include <signal.h>
#include <cstdlib>
#include <Grappa.hpp>
#include "ForkJoin.hpp"

#include <boost/test/unit_test.hpp>

DEFINE_bool( rotate, false, "Iterate through senders" );
DEFINE_bool( strong, false, "Strong scaling (iters per cores) or weak scaling (iters / core per core)?" );

DEFINE_bool( delegate, false, "Do delegate ops instead of calls" );
DEFINE_bool( iter, false, "Do delegate ops through iterations" );
DEFINE_bool( bidir, false, "Communicate bidirectionally, sending and receiving simultaneously" );

DEFINE_bool( gasnet, false, "Do gasnet ping test" );
DEFINE_bool( large, false, "Do gasnet large ping test" );

DEFINE_int64( iterations, 1 << 26, "Iterations" );
DEFINE_int64( payload_size, 0, "Payload size" );
DEFINE_int64( yield_mask, 0, "Yield mask" );

DEFINE_int64( trials, 1, "Number of trials to run" );

DECLARE_int32( stack_offset );

DECLARE_bool( flush_on_idle );

DEFINE_int32( debug_alarm, 0, "how many seconds before we drop into debugger?" );

// drop into debugger on alarm signal
void sigalrm_sighandler( int signum ) {
  raise( SIGUSR1 );
}


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

int gasnet_ping1_handle_ = -1;
void gasnet_ping1_am( gasnet_token_t token, void * buf, size_t size, gasnet_handlerarg_t arg ) {
  ++value;
}


LOOP_FUNCTION( func_start_profiling, index ) {
  Grappa_start_profiling();
}

LOOP_FUNCTION( func_stop_profiling, index ) {
  Grappa_stop_profiling();
}

LOOP_FUNCTION( func_print, index ) {
  LOG(INFO) << "value " << value << " iterations " << FLAGS_iterations;
  while( value < FLAGS_iterations / Grappa_nodes() ) Grappa_yield();
}

char payload[ 1 << 26 ];

LOOP_FUNCTOR( func_ping, index, ((int64_t, count)) ((int64_t, payload_size)) ) {
  if( Grappa_mynode() < Grappa_nodes() / 2 ) {
    // senders
    Node target = Grappa_mynode() + Grappa_nodes() / 2;
    LOG(INFO) << "Node " << Grappa_mynode() << " sending " << count << " messages to " << target;
    for( int i = 0; i < count; ++i ) {
      Grappa_call_on( target, &receive, payload, payload_size );
      if( FLAGS_rotate ) {
	++target;
	if( target == Grappa_nodes() ) target = Grappa_nodes() / 2;
      }
      if( (i & FLAGS_yield_mask) == 0 ) Grappa_yield();
    }
    // for( int i = Grappa_nodes() / 2; i < Grappa_nodes(); ++i ) {
    //   Grappa_flush( i );
    // }
    value = count;
    LOG(INFO) << "Node " << Grappa_mynode() << " sent " << count << " messages to " << target;
  } else {
    // receivers
    while( value != count ) Grappa_yield();
    LOG(INFO) << "Node " << Grappa_mynode() << " received " << value << " messages";
  }
  BOOST_CHECK( value == count );
}

LOOP_FUNCTOR( func_gasnet_ping, index, ((int64_t, count)) ((int64_t, payload_size)) ) {
  if( Grappa_mynode() < Grappa_nodes() / 2 ) {
    // senders
    Node target = Grappa_mynode() + Grappa_nodes() / 2;
    LOG(INFO) << "Node " << Grappa_mynode() << " sending " << count << " messages to " << target;
    for( int i = 0; i < count; ++i ) {
      GASNET_CHECK( gasnet_AMRequestMedium0( target, gasnet_ping_handle_, payload, payload_size ) );
      if( FLAGS_rotate ) {
	++target;
	if( target == Grappa_nodes() ) target = Grappa_nodes() / 2;
      }
      if( (i & FLAGS_yield_mask) == 0 ) Grappa_yield();
    }
    value = count;
    LOG(INFO) << "Node " << Grappa_mynode() << " sent " << count << " messages to " << target;
  } else {
    // receivers
    while( value != count ) Grappa_yield();
    LOG(INFO) << "Node " << Grappa_mynode() << " received " << value << " messages";
  }
  BOOST_CHECK( value == count );
}

LOOP_FUNCTOR( func_gasnet_large_ping, index, ((int64_t, count)) ((int64_t, payload_size)) ) {
  if( Grappa_mynode() < Grappa_nodes() / 2 ) {
    // senders
    Node target = Grappa_mynode() + Grappa_nodes() / 2;
    LOG(INFO) << "Node " << Grappa_mynode() << " sending " << count << " messages to " << target;
    for( int i = 0; i < count; ++i ) {
      GASNET_CHECK( gasnet_AMRequestLong0( target, gasnet_ping_handle_, payload, payload_size, payload ) );
      //GASNET_CHECK( gasnet_AMRequestLong0( target, gasnet_ping_handle_, payload, payload_size, payload ) );
      if( FLAGS_rotate ) {
	++target;
	if( target == Grappa_nodes() ) target = Grappa_nodes() / 2;
      }
      if( (i & FLAGS_yield_mask) == 0 ) Grappa_yield();
    }
    value = count;
    LOG(INFO) << "Node " << Grappa_mynode() << " sent " << count << " messages to " << target;
  } else {
    // receivers
    while( value != count ) Grappa_yield();
    LOG(INFO) << "Node " << Grappa_mynode() << " received " << value << " messages";
  }
  BOOST_CHECK( value == count );
}

LOOP_FUNCTOR( func_bidir_ping, index, ((int64_t, count)) ((int64_t, payload_size)) ) {
  // senders
  Node target = 0;
  if( FLAGS_rotate )
    target = Grappa_mynode();
  else 
    target = (Grappa_nodes() - 1) - Grappa_mynode();
  
  //LOG(INFO) << "Node " << Grappa_mynode() << " sending " << count << " messages to " << target;

  for( int i = 0; i < count; ++i ) {
    Grappa_call_on( target, &receive, (char*)0, 0 );
    if( FLAGS_rotate ) {
      target = target + 1;
      if( target >= Grappa_nodes() ) target = 0;
    }
    if( (i & FLAGS_yield_mask) == 0 ) Grappa_yield();
  }
  //Grappa_flush( target );  
  LOG(INFO) << "Sent " << count << " messages";

  // receivers
  while( value != count ) Grappa_yield();

  LOG(INFO) << "Node " << Grappa_mynode() << " received " << value << " messages";

  BOOST_CHECK( value == count );
}

LOOP_FUNCTOR( func_delegate_half, index, ((int64_t, count)) ((int64_t, payload_size)) ) {
  if( Grappa_mynode() < Grappa_nodes() / 2 ) {
    // senders
    Node target = Grappa_mynode() + Grappa_nodes() / 2;
    LOG(INFO) << "Node " << Grappa_mynode() << " sending " << count << " messages to " << target;
    for( int i = 0; i < count; ++i ) {
      GlobalAddress< int64_t > global_value = make_global( &value, target );
      Grappa_delegate_fetch_and_add_word( global_value, 1 );
      if( FLAGS_rotate ) {
	++target;
	if( target == Grappa_nodes() ) target = Grappa_nodes() / 2;
      }
      if( (i & FLAGS_yield_mask) == 0 ) Grappa_yield();
    }
    // for( int i = Grappa_nodes() / 2; i < Grappa_nodes(); ++i ) {
    //   Grappa_flush( i );
    // }
    value = count;
    LOG(INFO) << "Node " << Grappa_mynode() << " sent " << count << " messages to " << target;
  } else {
    // receivers
    while( value != count ) Grappa_yield();
    LOG(INFO) << "Node " << Grappa_mynode() << " received " << value << " messages";
  }
  BOOST_CHECK( value == count );
}

LOOP_FUNCTOR( func_delegate, index, ((int64_t, payload_size)) ) {
  Node target = 0;
  if( FLAGS_rotate )
    target = index % Grappa_nodes();
  else 
    target = (Grappa_nodes() - 1) - Grappa_mynode();
  GlobalAddress< int64_t > global_value = make_global( &value, target );
  //Grappa_call_on( target, &receive, payload, payload_size );
  Grappa_delegate_fetch_and_add_word( global_value, 1 );
  if( (index & FLAGS_yield_mask) == 0 ) Grappa_yield();
}

LOOP_FUNCTOR( func_iter_ping, index, ((int64_t, payload_size)) ) {
  if( index == 0 ) LOG(INFO) << "HELLO";
  Node target = 0;
  if( FLAGS_rotate )
    target = index % Grappa_nodes();
  else 
    target = (Grappa_nodes() - 1) - Grappa_mynode();
  //GlobalAddress< int64_t > global_value = make_global( &value, target );
  Grappa_call_on( target, &receive, payload, payload_size );
  //  Grappa_delegate_fetch_and_add_word( global_value, 1 );
  //if( (index & FLAGS_yield_mask) == 0 ) Grappa_yield();
  Grappa_yield();
}


// const int max_random_nodes = 1 << 16;
// Node random_nodes[ max_random_nodes ] = { 0 };
// LOOP_FUNCTOR( func_variable_gasnet_ping, index, ((int64_t, count)) ((int64_t, payload_size)) ((int, cores_per_node)) ((int, nnode)) ) {
//   // senders
//   //const Node initial_target = (Grappa_nodes() - 1) - Grappa_mynode();
//   const Node initial_target = (Grappa_mynode() + Grappa_nodes() / 2) % Grappa_nodes();
//   Node target = initial_target;
//   const Node max_target = (initial_target + FLAGS_receivers) % Grappa_nodes();

//   if( FLAGS_random_dest && !FLAGS_paired_dest ) {
//     // fill
//     boost::mt19937 gen;
//     boost::uniform_int<> range(0, Grappa_nodes() - 1);
//     boost::variate_generator<boost::mt19937&, boost::uniform_int<> > next(gen, range);
//     gen.seed( time(NULL) * Grappa_mynode() );
//     for( int i = 0; i < max_random_nodes; ++i ) {
//       random_nodes[ i ] = next();
//     }
//   } 

//   if( FLAGS_random_dest && FLAGS_paired_dest ) {
//     for( int i = 0; i < max_random_nodes; ++i ) {





//       int cores_per_node = atoi( 
//       random_nodes[ i ] = next();
//     }
//   }

//   //boost::scoped_ptr< int32_t > expected_values( new int32_t[ Grappa_nodes() ] );
//   int32_t expected_values[ Grappa_nodes() ];
//   for( int i = 0; i < Grappa_nodes(); ++i ) {
//     expected_values[i] = 0;
//   }

//   //VLOG(1) << "Node " << Grappa_mynode() << " sending " << count << " messages to " << target;

//   for( int i = 0; i < count; ++i ) {
//     //if( FLAGS_rotate ) {
//     //    target = nodemod( i * Grappa_mynode() * LARGE_PRIME );
//     //    target = i * Grappa_mynode() * LARGE_PRIME % Grappa_nodes();
//     //       target = target + 1;
//     //       if( target == Grappa_nodes() ) target = 0;
//     //       if( target == max_target ) target = initial_target;
//     //}
    
//     if( FLAGS_rotate && (FLAGS_random_dest || FLAGS_paired_dest ) {
//       target = random_nodes[ i % FLAGS_receivers ];
//     }
    
//     GASNET_CHECK( gasnet_AMRequestMedium0( target, gasnet_ping_handle_, payload, payload_size ) );
//     expected_values[ target ]++;

//     if( FLAGS_rotate && !FLAGS_random_dest ) {
//       target = target + 1;
//       if( target == Grappa_nodes() ) target = 0;
//       if( target == max_target ) target = initial_target;
//     }

//     if( (i & FLAGS_yield_mask) == 0 ) Grappa_yield();
//   }
//   //Grappa_flush( target );
//   VLOG(1) << "Node " << Grappa_mynode() << " sent " << count << " messages";

//   for( int i = 0; i < Grappa_nodes(); ++i ) {
//     GASNET_CHECK( gasnet_AMRequestMedium0( i, gasnet_finish_handle_, &expected_values[i], sizeof( expected_values[i] ) ) );
//   }

//   // receivers
//   //while( value < count ) Grappa_yield();
//   while( expected_replies != Grappa_nodes() && value != expected_value ) Grappa_yield();

//   VLOG(1) << "Node " << Grappa_mynode() << " received " << value << " messages";

//   BOOST_CHECK( value == expected_value );
// }


void user_main( int * args ) {
  value = 0;
  LOG(INFO) << "Value starts at " << value;

  func_start_profiling start_profiling;
  func_stop_profiling stop_profiling;

  int64_t nnode = atoi(getenv("SLURM_NNODES"));
  int64_t cores_per_node = atoi(getenv("SLURM_NTASKS_PER_NODE"));
  int64_t active_cores = Grappa_nodes();

  int64_t iterations = FLAGS_strong ? FLAGS_iterations : FLAGS_iterations / Grappa_nodes();

  int64_t i;

  double runtime = 0.0;
  double throughput = 0.0;
  double throughput_per_node = 0.0;
  double bandwidth = 0.0;
  double bandwidth_per_node = 0.0;

  Grappa_add_profiling_integer( &i, "run_trial", "it", false, 0 );
  Grappa_add_profiling_integer( &value, "Value", "", false, 0 );
  Grappa_add_profiling_integer( &active_cores, "active_cores", "cores", false, 0 );
  Grappa_add_profiling_integer( &cores_per_node, "cores_per_node", "cores", false, 0 );

  Grappa_add_profiling_value( &runtime, "runtime", "s", false, 0.0 );
  Grappa_add_profiling_value( &throughput, "throughput", "it/s", false, 0.0 );
  Grappa_add_profiling_value( &throughput_per_node, "throughput_per_node", "it/s", false, 0.0 );
  Grappa_add_profiling_value( &bandwidth, "bandwidth", "B/s", false, 0.0 );
  Grappa_add_profiling_value( &bandwidth_per_node, "bandwidth_per_node", "B/s", false, 0.0 );


  // one-sided ops, single destination
  // one-sided ops, random destinations
  // delegate ops, single destination
  // delegate ops, random destinations

  for( i = 0; i < FLAGS_trials; ++i ) {
    value = 0;
    alarm( FLAGS_debug_alarm );

  fork_join_custom( &start_profiling );
  double start = wall_clock_time();
  if( FLAGS_gasnet ) {
    if( FLAGS_large ) {
      iterations = FLAGS_strong ? FLAGS_iterations : FLAGS_iterations / (Grappa_nodes() / 2);
      active_cores = Grappa_nodes() / 2;
      func_gasnet_large_ping gasnet_ping( iterations, FLAGS_payload_size );
      fork_join_custom( &gasnet_ping );
    } else {
      iterations = FLAGS_strong ? FLAGS_iterations : FLAGS_iterations / (Grappa_nodes() / 2);
      active_cores = Grappa_nodes() / 2;
      func_gasnet_ping gasnet_ping( iterations, FLAGS_payload_size );
      fork_join_custom( &gasnet_ping );
    }
  } else {
    if( FLAGS_delegate ) {
      if( FLAGS_bidir ) {
	iterations = FLAGS_strong ? FLAGS_iterations * Grappa_nodes() : FLAGS_iterations;
	active_cores = Grappa_nodes();
	func_delegate delegate( FLAGS_payload_size );
	fork_join( &delegate, 0, iterations );
	// when all delegate ops have a reply, we're done.
      } else {
	iterations = FLAGS_strong ? FLAGS_iterations : FLAGS_iterations / (Grappa_nodes() / 2);
	active_cores = Grappa_nodes() / 2;
	func_delegate_half delegate_half( iterations, FLAGS_payload_size );
	fork_join_custom( &delegate_half );
      }
    } else {
      if( FLAGS_bidir ) {
	if( FLAGS_iter ) {
	  iterations = FLAGS_strong ? FLAGS_iterations * Grappa_nodes() : FLAGS_iterations;
	  active_cores = Grappa_nodes();
	  func_iter_ping iter_ping( FLAGS_payload_size );
	  fork_join( &iter_ping, 0, iterations );
	  // have each node spin until it's received all its messages
	  func_print print;
	  fork_join_custom( &print );
	} else {
	  iterations = FLAGS_strong ? FLAGS_iterations : FLAGS_iterations / Grappa_nodes();
	  active_cores = Grappa_nodes();
	  func_bidir_ping bidir_ping( iterations, FLAGS_payload_size );
	  fork_join_custom( &bidir_ping );
	}
      } else {
	iterations = FLAGS_strong ? FLAGS_iterations : FLAGS_iterations / (Grappa_nodes() / 2);
	active_cores = Grappa_nodes() / 2;
	func_ping ping( iterations, FLAGS_payload_size );
	fork_join_custom( &ping );
      }
    }
  }
  double end = wall_clock_time();
  fork_join_custom( &stop_profiling );

  runtime = end - start;
  throughput = FLAGS_strong ? Grappa_nodes() * FLAGS_iterations / runtime : FLAGS_iterations / runtime;
  throughput_per_node = throughput / (active_cores / cores_per_node);
  bandwidth = FLAGS_payload_size * throughput;
  bandwidth_per_node = bandwidth / (active_cores / cores_per_node);

  Grappa_merge_and_dump_stats( LOG(INFO) );
  //Grappa_dump_stats_all_nodes();

  Grappa_reset_stats_all_nodes();

  alarm( 0 );

  sleep(1);

  }
}


BOOST_AUTO_TEST_CASE( test1 ) {

  struct sigaction sigalrm_sa;
  sigemptyset( &sigalrm_sa.sa_mask );
  sigalrm_sa.sa_flags = 0;
  sigalrm_sa.sa_handler = &sigalrm_sighandler;
  CHECK_EQ( 0, sigaction( SIGALRM, &sigalrm_sa, 0 ) ) << "SIGALRM signal handler installation failed.";

    Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
		  &(boost::unit_test::framework::master_test_suite().argv),
		  (1 << 22) );
    gasnet_ping_handle_ = global_communicator.register_active_message_handler( &gasnet_ping_am );
    gasnet_ping1_handle_ = global_communicator.register_active_message_handler( &gasnet_ping1_am );
    Grappa_activate();

    Grappa_run_user_main( &user_main, (int*)NULL );

    Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();
