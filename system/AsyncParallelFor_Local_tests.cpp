
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

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
  SoftXMT_privateTask( &async_parallel_for<&loop_body, &spawn_private_task, ASYNC_PAR_FOR_DEFAULT>, a, b );
}

void user_main( void * args ) {
  // create a semaphore to synchronize tasks on a single node (could also use LocalTaskJoiner)
  // able to use semaphore to tell it how many to wait on
  sem = new Semaphore(size, 0);

  // recursive decomposition that spawns private tasks (not stealable)
  async_parallel_for<&loop_body, &spawn_private_task, ASYNC_PAR_FOR_DEFAULT >( 0, size );

  // wait on all tasks finishing
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
