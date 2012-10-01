// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Delegate.hpp"
#include "ForkJoin.hpp"

BOOST_AUTO_TEST_SUITE( Tasking_tests );

//
// Basic test of Grappa running on two Nodes: run user main, and spawning local tasks, local task joiner
//
// This test spawns a few private tasks that do delegate operations to Node 1

int num_tasks = 8;
int64_t num_finished=0;
GlobalAddress<int64_t> nf_addr;

struct task1_arg {
    int num;
    Thread * parent;
};

void task1_f( task1_arg * arg ) {
    int mynum = arg->num;
    Thread * parent = arg->parent;

    BOOST_MESSAGE( CURRENT_THREAD << " with task " << mynum << " about to yield 1" );
    Grappa_yield( );
    BOOST_MESSAGE( CURRENT_THREAD << " with task " << mynum << " about to yield 2" );
    Grappa_yield( );
    BOOST_MESSAGE( CURRENT_THREAD << " with task " << mynum << " is done" );

    // int fetch add to address on Node1
    int64_t result = Grappa_delegate_fetch_and_add_word( nf_addr, 1 );
    BOOST_MESSAGE( CURRENT_THREAD << " with task " << mynum << " result=" << result );
    if ( result == num_tasks-1 ) {
        Grappa_wake( parent );
    }
}

struct task2_shared {
  GlobalAddress<int64_t> nf;
  int64_t * array;
  Thread * parent;
  LocalTaskJoiner * joiner;
};

void task2_f(int64_t index, task2_shared * sa) {
  
  BOOST_MESSAGE( CURRENT_THREAD << " with task " << index << ", sa = " << sa);

  int64_t result = Grappa_delegate_fetch_and_add_word( sa->nf, 1 );
  sa->array[index] = result;

  BOOST_MESSAGE( CURRENT_THREAD << " with task " << index << " about to yield" );
  Grappa_yield( );
  BOOST_MESSAGE( CURRENT_THREAD << " with task " << index << " is done" );

  BOOST_MESSAGE( CURRENT_THREAD << " with task " << index << " result=" << result );
  
  sa->joiner->signal();
}

void user_main( void* args ) 
{
  nf_addr = make_global( &num_finished );

  task1_arg argss[num_tasks];
  for (int ta = 0; ta<num_tasks; ta++) {
    argss[ta].num = ta; argss[ta].parent = CURRENT_THREAD;
    Grappa_privateTask( &task1_f, &argss[ta] );
  }

  Grappa_suspend(); // no wakeup race because tasks wont run until this yield occurs
                     // normally a higher level robust synchronization object should be used

  BOOST_MESSAGE( "testing shared args" );
  int64_t array[num_tasks];
  int64_t nf = 0;
  LocalTaskJoiner joiner;
  
  task2_shared sa;
  sa.nf = make_global(&nf);
  sa.array = array;
  sa.parent = CURRENT_THREAD;
  sa.joiner = &joiner;

  for (int64_t i=0; i<num_tasks; i++) {
    joiner.registerTask();
    Grappa_privateTask(&task2_f, i, &sa);
  }
  joiner.wait();

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

