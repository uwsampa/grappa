
#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"


DEFINE_int64( iterations, 10000000, "experiment loop iterations");


double compute_time( struct timespec starttime, struct timespec endtime ) {
  uint64_t start_ns = ((uint64_t) starttime.tv_sec * 1000000000 + starttime.tv_nsec);
  uint64_t end_ns = ((uint64_t) endtime.tv_sec * 1000000000 + endtime.tv_nsec);
  double runtime = ( (double) end_ns - (double) start_ns ) / 1000000000.0;
  return runtime;
}

BOOST_AUTO_TEST_SUITE( gasnet_tests );

bool done = false;
int64_t counter = 0;

int my_h = 0;
void my_am(gasnet_token_t token) {
  ++counter;
  if( counter == FLAGS_iterations ) {
    done = true;
  }
}

// int wait_for_h = 0;
// void wait_for_am(gasnet_token_t token) {
//   ++counter;
//   if( counter == FLAGS_iterations ) {
//     done = true;
//   }
// }

int set_done_h = 0;
void set_done_am(gasnet_token_t token) {
  done = true;
}

int my_remote_h = 0;
void my_remote_am(gasnet_token_t token) {
  ++counter;
  if( counter == FLAGS_iterations ) {
    gasnet_AMRequestShort0( 0, set_done_h );
  }
}

void user_main( thread * me, void * args ) 
{
  struct timespec starttime;
  struct timespec endtime;

  clock_gettime(CLOCK_MONOTONIC, &starttime);

  for( int i = 0; i < FLAGS_iterations; ++i ) {
    gasnet_AMRequestShort0( 0, my_h );
  }
  while( !done ) {
    global_communicator->poll();
  }

  clock_gettime(CLOCK_MONOTONIC, &endtime);

  LOG(INFO) << "Local time was " << compute_time( starttime, endtime ) << " s.";
  LOG(INFO) << "Local communication rate was " 
            << (double)FLAGS_iterations / compute_time( starttime, endtime ) << " M/s.";

  // clock_gettime(CLOCK_MONOTONIC, &starttime);

  // for( int i = 0; i < FLAGS_iterations; ++i ) {
  //   gasnet_AMRequestShort0( 1, my_remote_h );
  // }
  // while( !done ) {
  //   global_communicator->poll();
  // }

  // clock_gettime(CLOCK_MONOTONIC, &endtime);

  // LOG(INFO) << "Remote time was " << compute_time( starttime, endtime ) << " s.";
  // LOG(INFO) << "Remote communication runtime was " 
  //           << (double)FLAGS_iterations / compute_time( starttime, endtime ) << " M/s.";

  SoftXMT_signal_done();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  my_h = global_communicator->register_active_message_handler( &my_am );
  my_remote_h = global_communicator->register_active_message_handler( &my_remote_am );
  set_done_h = global_communicator->register_active_message_handler( &set_done_am );

  SoftXMT_activate();

  SoftXMT_run_user_main( &user_main, NULL );
  BOOST_CHECK( SoftXMT_done() == true );


  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

