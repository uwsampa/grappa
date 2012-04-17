
#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Delegate.hpp"
#include "Tasking.hpp"
#include "Future.hpp"

BOOST_AUTO_TEST_SUITE( Future_tests );

int64_t dummy_int;

struct task1_args {
    int len;
    GlobalAddress<int64_t> array;
};

void task1_f( task1_args * args ) {
    BOOST_MESSAGE( "task1 (thread " << CURRENT_THREAD->id << ")"
                   << " started" );
    
    // do some work (in-place linear-time prefix sum)
    int64_t sum = 0;
    for (int i=0; i<args->len; i++) {
        int64_t value = SoftXMT_delegate_fetch_and_add_word(args->array + i, sum);
        sum+=value;
    }

    BOOST_MESSAGE( "task1 (thread " << CURRENT_THREAD->id << ")"
                   << " exiting" );
}

struct user_main_args {
};

void user_main( user_main_args * args ) 
{
    ///
    /// Test1
    ///
    BOOST_MESSAGE( "test: likely touch go" );
    int length1 = 8;
    int64_t array1[8] = { 0,1,2,3,4,5,6,7 };
    task1_args t1_args = { length1, make_global(&array1) };
    Future< task1_args > d1( &task1_f, &t1_args );
    d1.addAsPublicTask( );
    BOOST_MESSAGE( "user main (thread " << CURRENT_THREAD->id << ")"
                   << " spawned future 1" );
    
    // likely go
    d1.touch( );

    int64_t sum_ck1=0;
    for (int i=0; i<length1; i++) {
        BOOST_MESSAGE( "array[" << i << "]" << "=" << array1[i] );
        sum_ck1+=i;
        BOOST_CHECK_EQUAL( sum_ck1, array1[i] ); 
    }

    ///
    /// Test2
    ///
    BOOST_MESSAGE( "test: likely touch wait" );
    int length2 = 5;
    int64_t array2[5] = { 2,4,6,8,10 };
    int64_t out2[5]   = { 2,6,12,20,30 };
    task1_args t2_args = { length2, make_global(&array2) };
    Future< task1_args > d2( &task1_f, &t2_args );
    d2.addAsPublicTask( );
    BOOST_MESSAGE( "user main (thread " << CURRENT_THREAD->id << ")"
                   << " spawned future 2" );
   
    // do a operation that will yield to make it very likely another
    // thread will execute d2
    SoftXMT_delegate_write_word( make_global(&dummy_int, 1), 0xBEEF );

    // likely wait
    d2.touch( );

    for (int i=0; i<length2; i++) {
        BOOST_MESSAGE( "array[" << i << "]" << "=" << array2[i] );
        BOOST_CHECK_EQUAL( out2[i], array2[i] ); 
    }


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

