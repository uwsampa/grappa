// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.



#include <boost/test/unit_test.hpp>

#include "SoftXMT.hpp"
#include "Cache.hpp"
#include "ForkJoin.hpp"

/// This test suite tests the task cache wrappers defined in Cache.hpp.
/// These functions wrap a task function pointer to take care of caching
/// arguments from the original Node.
/// There are two variations, declare the wrapped function or pass it in inline.


//RESUME!
//all variations and declare and wrap

BOOST_AUTO_TEST_SUITE( CacheWrapped_tests );


struct task_arg {
    int num;
    GlobalAddress<Semaphore> sem;
};

// test when arg is pointer
void pointer_task_f( task_arg * arg ) {
    CHECK( arg->num == 1 ) << "num=" << arg->num;
    arg->num++;
    Semaphore::release( &(arg->sem), 1 );
}
DECLARE_CACHE_WRAPPED(pointer_task_f_CW, &pointer_task_f, task_arg)

// test when arg is reference
void ref_task_f( task_arg& arg ) {
    CHECK( arg.num == 1 ) << "num=" << arg.num;
    arg.num++;
    Semaphore::release( &(arg.sem), 1);
}
DECLARE_CACHE_WRAPPED(ref_task_f_CW, &ref_task_f, task_arg)

// test when arg is pointer to const
void c_pointer_task_f( const task_arg * arg ) {
    CHECK( arg->num == 1 ) << "num=" << arg->num ;
    GlobalAddress<Semaphore> * sa = (GlobalAddress<Semaphore>*) &arg->sem;
    Semaphore::release( sa, 1 );  // don't care about const of address
}
DECLARE_CACHE_WRAPPED(c_pointer_task_f_CW, &c_pointer_task_f, task_arg)

// test when arg is reference of const
void c_ref_task_f( const task_arg& arg ) {
    CHECK( arg.num == 1 ) << "num=" << arg.num ;
    GlobalAddress<Semaphore> * sa = (GlobalAddress<Semaphore>*) &arg.sem;
    Semaphore::release( sa, 1 ); // don't care about const of address
}
DECLARE_CACHE_WRAPPED(c_ref_task_f_CW, &c_ref_task_f, task_arg)


struct user_main_args {
};

void user_main( user_main_args * args ) 
{
    BOOST_MESSAGE( "pointer RW, declared" );
    {
        for ( Node i=0; i<2; i++ ) {
            Semaphore sem( 1, 0 );
            task_arg args = { 1, make_global( &sem ) };
            SoftXMT_remote_privateTask( &pointer_task_f_CW, make_global( &args ), i );
            sem.acquire_all( CURRENT_THREAD );
            // User sync will not guarentee this
            //CHECK( args.num == 2 ) << "num=" << args.num << " i=" << i; 
        }
    }
    
//    BOOST_MESSAGE( "pointer RW, callsite" );
//    {
//        for ( Node i=0; i<2; i++ ) {
//            phore sem( 1, 0 );
//            task_arg args = { 1, make_global( &sem ) };
//            SoftXMT_remote_privateTask( CACHE_WRAP( &pointer_task_f, &args ), i );
//            sem.acquire_all( CURRENT_THREAD );
//            BOOST_CHECK( args.num == 2 );
//        }
//    }
    
    BOOST_MESSAGE( "ref RW, declared" );
    {
        for ( Node i=0; i<2; i++ ) {
            Semaphore sem( 1, 0 );
            task_arg args = { 1, make_global( &sem ) };
            SoftXMT_remote_privateTask( &ref_task_f_CW, make_global( &args ), i );
            sem.acquire_all( CURRENT_THREAD );
            // User sync will not guarentee this
            //CHECK( args.num == 2 ) << "num=" << args.num << " i=" << i; 
        }
    }
    
//    BOOST_MESSAGE( "ref RW, callsite" );
//    {
//        for ( Node i=0; i<2; i++ ) {
//            Semaphore sem( 1, 0 );
//            task_arg args = { 1, make_global( &sem ) };
//            SoftXMT_remote_privateTask( CACHE_WRAP( &ref_task_f, &args ), i );
//            sem.acquire_all( CURRENT_THREAD );
//            BOOST_CHECK( args.num == 1 );
//        }
//    }
    
    BOOST_MESSAGE( "pointer RO, declared" );
    {
        for ( Node i=0; i<2; i++ ) {
            Semaphore sem( 1, 0 );
            task_arg args = { 1, make_global( &sem ) };
            SoftXMT_remote_privateTask( &c_pointer_task_f_CW, make_global( &args ), i );
            sem.acquire_all( CURRENT_THREAD );
            CHECK( args.num == 1 ) << "num=" << args.num << " i=" << i; 
        }
    }
    
    BOOST_MESSAGE( "ref RO, declared" );
    {
        for ( Node i=0; i<2; i++ ) {
            Semaphore sem( 1, 0 );
            task_arg args = { 1, make_global( &sem ) };
            SoftXMT_remote_privateTask( &c_ref_task_f_CW, make_global( &args ), i );
            sem.acquire_all( CURRENT_THREAD );
            CHECK( args.num == 1 ) << "num=" << args.num << " i=" << i; 
        }
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

