
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "CompletionEvent.hpp"

#include <string>

DECLARE_uint64( num_starting_workers );
DEFINE_uint64( lines, 512, "Cachelines to touch before context switch" );

using namespace Grappa;

CompletionEvent * final;
CompletionEvent * task_barrier;
CompletionEvent * task_signal;
        
Grappa_Timestamp start_ts, end_ts;

struct Cacheline {
  uint64_t val;
  char padding[56];
};

const size_t MAX_ARRAY_SIZE = 5*1024*1024;
Cacheline myarray[MAX_ARRAY_SIZE];

BOOST_AUTO_TEST_SUITE( ContextSwitchLatency_tests );

void user_main( void * args ) {
  srand((unsigned int)Grappa_walltime());
  
  final = new CompletionEvent(2);
  task_barrier = new CompletionEvent(2);
  task_signal = new CompletionEvent(1);

  BOOST_CHECK( Grappa_nodes() == 1 );
  BOOST_CHECK( FLAGS_lines <= MAX_ARRAY_SIZE );
    privateTask( [] {

      // wait for all to start (to hack scheduler yield)
      task_barrier->complete();
      task_barrier->wait();

      // blow out cache
      for ( uint64_t i=0; i<FLAGS_lines; i++ ) {
        myarray[i].val += 1;
      }

        Grappa_tick();
        start_ts = Grappa_get_timestamp();
        
        task_signal->complete();
        final->complete();
        });

    privateTask( [] {
      // wait for all to start (to hack scheduler yield)
      task_barrier->complete();
      task_barrier->wait();
        task_signal->wait();

        Grappa_tick();
        end_ts = Grappa_get_timestamp();

        final->complete();
        });

    final->wait();

      BOOST_MESSAGE( "ticks = " << end_ts << " - " << start_ts << " = " << end_ts-start_ts );
      BOOST_MESSAGE( "time = " << ((double)(end_ts-start_ts)) / Grappa::tick_rate );


  BOOST_MESSAGE( "user main is exiting" );
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

