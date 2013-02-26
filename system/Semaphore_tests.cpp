
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Semaphore.hpp"
#include "Statistics.hpp"
#include "TaskingScheduler.hpp"

BOOST_AUTO_TEST_SUITE( Semaphore_tests );

void yield() {
  Worker * thr = global_scheduler.get_current_thread();
  Grappa::privateTask( [thr] {
      global_scheduler.thread_wake( thr );
    });
  global_scheduler.thread_suspend();
}


void user_main( void * args ) 
{
  Grappa::CountingSemaphore s(1);

  // bit vector
  int data = 1;
  int count = 0;

  Grappa::privateTask( [&s,&data,&count] { 
      for( int i = 0; i < 3; ++i ) {
        Grappa::decrement( &s );
        data <<= 1;
        data |= 1;
        count++;
        Grappa::increment( &s );
        yield();
      }
    } );

  Grappa::privateTask( [&s,&data,&count] { 
      for( int i = 0; i < 3; ++i ) {
        Grappa::decrement( &s );
        data <<= 1;
        count++;
        Grappa::increment( &s );
        yield();
      }
    } );

  // other thread runs and blocks
  while( count < 6 ) {
    DVLOG(5) << global_scheduler.get_current_thread() << ": Checking count=" << count;
    yield();
  }

  BOOST_CHECK_EQUAL( count, 6 );

  Grappa::Statistics::merge_and_print();
  //Grappa::Statistics::print();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
	       &(boost::unit_test::framework::master_test_suite().argv),
	       (1L << 20) );

  Grappa_activate();

  Grappa_run_user_main( &user_main, (void*)NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

