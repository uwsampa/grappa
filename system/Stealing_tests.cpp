
#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Delegate.hpp"

BOOST_AUTO_TEST_SUITE( Stealing_tests );

int num_tasks = 10000;
int64_t num_finished=0;
GlobalAddress<int64_t> nf_addr;

struct task1_arg {
    int num;
    thread * parent;
};

void wake_f( task1_arg * args, size_t size, void * payload, size_t payload_size ) {
    SoftXMT_wake( args->parent );
}

void task1_f( void * arg ) {
    task1_arg* targ = (task1_arg*) arg;
    int mynum = targ->num;
    thread * parent = targ->parent;

    if (mynum % 500 == 0) {
        BOOST_MESSAGE( CURRENT_THREAD << " with task " << mynum << " about to yield" );
    }
    SoftXMT_yield( );
    
    int64_t result = SoftXMT_delegate_fetch_and_add_word( nf_addr, 1 );
    if ( result == num_tasks-1 ) {
        BOOST_MESSAGE( CURRENT_THREAD << " with task " << mynum << " result=" << result );
        if ( SoftXMT_mynode() == 0 ) {
            SoftXMT_wake( parent );
        } else {
            SoftXMT_call_on( 0, wake_f, targ );
        }
    }
}

void user_main( thread * me, void * args ) 
{
  nf_addr = GlobalAddress<int64_t>::TwoDimensional( &num_finished );

  task1_arg argss[num_tasks];
  for (int ta = 0; ta<num_tasks; ta++) {
    argss[ta] = { ta, me };
    SoftXMT_publicTask( &task1_f, &argss[ta] );
  }

  SoftXMT_suspend( );  // currently here just not to starve workers

  BOOST_MESSAGE( "user main is exiting" );
  SoftXMT_signal_done();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();


  DVLOG(1) << "Spawning user main thread....";
  SoftXMT_run_user_main( &user_main, NULL );
  BOOST_CHECK( SoftXMT_done() == true );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

