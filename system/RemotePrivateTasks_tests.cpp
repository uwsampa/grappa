

#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "ForkJoin.hpp"
#include "Delegate.hpp"

//#define BLOG(msg) BOOST_MESSAGE(msg)
#define BLOG(msg) VLOG(1) << msg

BOOST_AUTO_TEST_SUITE( RemotePrivateTasks_tests );

struct task1_arg {
    GlobalAddress<Semaphore> sem;
    int i;
};

void task1_f( task1_arg * args ) {
    
    BLOG( CURRENT_THREAD << " task runs i=" << args->i );
    SoftXMT_yield();
    SoftXMT_yield();

    Semaphore::release( &args->sem, 1 ); 
}

DECLARE_CACHE_WRAPPED(task1_f_CA, &task1_f, task1_arg )

struct user_main_args {
};

void user_main( user_main_args * args ) 
{

    // do several rounds of private spawns
    for (int i=0; i<500; i++) {
        Semaphore sem(1, 0);
        task1_arg t1_arg = { make_global( &sem ), i };
        BLOG( "remote-spawn " << i );
        SoftXMT_remote_privateTask( &task1_f_CA, make_global( &t1_arg ), 1 );
        sem.acquire_all( CURRENT_THREAD );
        BLOG( "phase " << i << " done" );
    }

    BLOG( "user main is exiting" );
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

