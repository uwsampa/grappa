
#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Delegate.hpp"
#include "Tasking.hpp"

/// 8 tasks are spawned; first 4 meant to be stolen, then 4 meant to stay put
/// chunksize is 4 to ensure half the 8 will be released and stolen
/// these 4 stolen tasks are meant to wake the unstolen tasks
BOOST_AUTO_TEST_SUITE( Stealing_tests );


int num_tasks = 8;
int64_t num_finished=0;
int64_t vals[4] = { 0 };
Thread * threads[4];

struct task1_arg {
    int num;
    Thread * parent;
};

struct wake_arg {
    Thread * wakee;
};

void wake_f( wake_arg * args, size_t size, void * payload, size_t payload_size ) {
    SoftXMT_wake( args->wakee );
}

struct wakeindex_args {
    int index;
};

void wakeindex_f( wakeindex_args * args, size_t size, void * payload, size_t payload_size ) {
    SoftXMT_wake( threads[args->index] );
}

void pairsBarrier( int index ) {
    // increment the val on Node 0
    GlobalAddress<int64_t> vals_addr = GlobalAddress<int64_t>::TwoDimensional( &vals[index], 0 );
    int64_t result = SoftXMT_delegate_fetch_and_add_word( vals_addr, 1 );
    if ( 0 == result ) {
        // I am first so suspend
        SoftXMT_suspend( );
    } else if ( 1 == result ) {
        // I am second so wake other
        wakeindex_args warg = { index };
        SoftXMT_call_on( 1-SoftXMT_mynode(), &wakeindex_f, &warg );
    } else {
        BOOST_CHECK( false );
    }
}

void task_local( task1_arg * arg ) {
    int mynum = arg->num;
    Thread * parent = arg->parent;
   
    // this task should not have been stolen and running on 0 
    BOOST_CHECK_EQUAL( 0, SoftXMT_mynode() );

    BOOST_MESSAGE( CURRENT_THREAD << " with task(local) " << mynum << " about to enter pair barrier" );
    threads[mynum] = CURRENT_THREAD; // store my Thread ptr in local global array
    pairsBarrier( mynum );
    
    // increment the global counter
    GlobalAddress<int64_t> nf_addr = GlobalAddress<int64_t>::TwoDimensional( &num_finished, 0 );
    int64_t result = SoftXMT_delegate_fetch_and_add_word( nf_addr, 1 );
    if ( result == num_tasks-1 ) {
        BOOST_MESSAGE( CURRENT_THREAD << " with task(local) " << mynum << " result=" << result );
        SoftXMT_wake( parent );
    }
}

void task_stolen( task1_arg * arg ) {
    int mynum = arg->num;
    Thread * parent = arg->parent;

    // this task should have been stolen and running on 1
    BOOST_CHECK_EQUAL( 1, SoftXMT_mynode() );

    // wake the corresponding task on Node 0
    BOOST_MESSAGE( CURRENT_THREAD << " with task(stolen) " << mynum << " about to enter pair barrier" );
    threads[mynum] = CURRENT_THREAD; // store my Thread ptr in local global array
    pairsBarrier( mynum );
    
    // increment the global counter
    GlobalAddress<int64_t> nf_addr = GlobalAddress<int64_t>::TwoDimensional( &num_finished, 0 );
    int64_t result = SoftXMT_delegate_fetch_and_add_word( nf_addr, 1 );
    if ( result == num_tasks-1 ) {
        BOOST_MESSAGE( CURRENT_THREAD << " with task(stolen) " << mynum << " result=" << result );
        wake_arg wwarg = { parent };
        SoftXMT_call_on( 0, &wake_f, &wwarg );
    }
     
}

void user_main( Thread * me, void * args ) 
{
  task1_arg argss[num_tasks];
  for (int ta = 0; ta<(num_tasks/2); ta++) {
    argss[ta] = { ta, me };
    SoftXMT_publicTask( &task_stolen, &argss[ta] );
  }
  for (int ta = 0; ta<(num_tasks/2); ta++) {
    argss[ta] = { ta, me };
    SoftXMT_publicTask( &task_local, &argss[ta] );
  }

  SoftXMT_suspend( );  // currently here just not to starve workers

  BOOST_MESSAGE( "user main is exiting" );
  SoftXMT_signal_done();
}


BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();

  // force certain run parameters
  //BOOST_CHECK_EQUAL( FLAGS_chunk_size, 4 );
  //BOOST_CHECK_EQUAL( FLAGS_num_starting_workers, 4 );

  DVLOG(1) << "Spawning user main Thread....";
  SoftXMT_run_user_main( &user_main, NULL );
  BOOST_CHECK( SoftXMT_done() == true );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

