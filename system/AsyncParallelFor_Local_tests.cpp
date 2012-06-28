#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "ForkJoin.hpp"
#include "AsyncParallelFor.hpp"

BOOST_AUTO_TEST_SUITE( AsyncParallelFor_Local_tests );

#define size 12
bool done[size] = {false};
Semaphore * sem;
int count = 0;


void loop_body(int64_t start, int64_t num ) {
  BOOST_CHECK( num <= FLAGS_async_par_for_threshold );

  BOOST_MESSAGE( "execute [" << start << ","
            << start+num << ") with thread " << CURRENT_THREAD->id );
  for (int i=start; i<start+num; i++) {
    BOOST_CHECK( !done[i] );
    done[i] = true;
    SoftXMT_delegate_fetch_and_add_word( make_global( &count, 1 ), 1 );  // force a suspend 
    sem->release(1);
  }
}

void spawn_private_task(int64_t a, int64_t b) {
  SoftXMT_privateTask( &async_parallel_for<&loop_body, &spawn_private_task>, a, b );
}

void user_main( void * args ) {
  sem = new Semaphore(size, 0);

  async_parallel_for<&loop_body, &spawn_private_task >( 0, size );

  sem->acquire_all( CURRENT_THREAD );

  for (int i=0; i<size; i++) {
    BOOST_CHECK( done[i] );
  }
  BOOST_CHECK( size == SoftXMT_delegate_read_word( make_global( &count, 1 ) ));

  BOOST_MESSAGE( "user main is exiting" );
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
