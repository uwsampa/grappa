

#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Delegate.hpp"
#include "Tasking.hpp"

BOOST_AUTO_TEST_SUITE( Tasking_tests );

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
    SoftXMT_yield( );
    BOOST_MESSAGE( CURRENT_THREAD << " with task " << mynum << " about to yield 2" );
    SoftXMT_yield( );
    BOOST_MESSAGE( CURRENT_THREAD << " with task " << mynum << " is done" );

    int64_t result = SoftXMT_delegate_fetch_and_add_word( nf_addr, 1 );
    BOOST_MESSAGE( CURRENT_THREAD << " with task " << mynum << " result=" << result );
    if ( result == num_tasks-1 ) {
        SoftXMT_wake( parent );
    }
}

struct user_main_args {
};

void user_main( user_main_args * args ) 
{
  nf_addr = GlobalAddress<int64_t>::TwoDimensional( &num_finished );

  task1_arg argss[num_tasks];
  for (int ta = 0; ta<num_tasks; ta++) {
    argss[ta] = { ta, CURRENT_THREAD };
    SoftXMT_privateTask( &task1_f, &argss[ta] );
  }

  SoftXMT_suspend(); // no wakeup race because tasks wont run until this yield occurs

  BOOST_MESSAGE( "user main is exiting" );
}

BOOST_AUTO_TEST_CASE( test1 ) {

  SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  SoftXMT_activate();

  user_main_args uargs;

  DVLOG(1) << "Spawning user main Thread....";
  SoftXMT_run_user_main( &user_main, &uargs );
  VLOG(5) << "run_user_main returned";
  CHECK( SoftXMT_done() );

  SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

