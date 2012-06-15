

#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "ForkJoin.hpp"

//#define BLOG(msg) BOOST_MESSAGE(msg)
#define BLOG(msg) VLOG(1) << msg

BOOST_AUTO_TEST_SUITE( ForkJoinStress_tests );

int64_t ind_local_count = 0;

LOOP_FUNCTION(task1_func,nid) {
    for (int i=0; i<100000; i++ ) {
        SoftXMT_yield(); // do "work"
    }
}

struct func_yield : public ForkJoinIteration {
    int64_t * ignore;
    void operator()(int64_t i) const {
        ind_local_count+=i;
        SoftXMT_yield();
    }
};

struct user_main_args {
};

void user_main( user_main_args * args ) 
{

    BOOST_MESSAGE( "Running fork join custom" );
    { 
        // do several rounds of fork join
        for (int i=0; i<12; i++) {
            task1_func f;
            fork_join_custom(&f);
            BLOG( "fj round " << i );
        }
    }
    
    BOOST_MESSAGE( "Running fork join -- yield iter" );
    {
        // a[i] = i
        func_yield fy;
        int64_t ii = 0;
        fy.ignore = &ii;
        fork_join(&fy, 0, 4000000);
    }

    BLOG( "user main is exiting" );
    SoftXMT_end_tasks();
}

BOOST_AUTO_TEST_CASE( test1 ) {

    SoftXMT_init( &(boost::unit_test::framework::master_test_suite().argc),
            &(boost::unit_test::framework::master_test_suite().argv) );

    SoftXMT_activate();

    // require lots of procs
    BOOST_CHECK( SoftXMT_nodes() >= 96 );

    user_main_args uargs;

    DVLOG(1) << "Spawning user main Thread....";
    SoftXMT_run_user_main( &user_main, &uargs );
    VLOG(5) << "run_user_main returned";
    CHECK( SoftXMT_done() );

    SoftXMT_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

