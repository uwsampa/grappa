// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


#include <boost/test/unit_test.hpp>

#include "Grappa.hpp"
#include "Delegate.hpp"
#include "CompletionEvent.hpp"

BOOST_AUTO_TEST_SUITE( Tasking_tests );

using namespace Grappa;

//
// Basic test of Grappa running on two Cores: run user main, and spawning local tasks, local task joiner
//
// This test spawns a few private tasks that do delegate operations to Core 1

int num_tasks = 8;
int64_t num_finished=0;

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{


    auto nf_addr = make_global( &num_finished );

    for (int ta = 0; ta<num_tasks; ta++) {
      auto parent = CURRENT_THREAD;
    
      privateTask([ta,parent,nf_addr]{
        int mynum = ta;

        BOOST_MESSAGE( CURRENT_THREAD << " with task " << mynum << " about to yield 1" );
        Grappa::yield( );
        BOOST_MESSAGE( CURRENT_THREAD << " with task " << mynum << " about to yield 2" );
        Grappa::yield( );
        BOOST_MESSAGE( CURRENT_THREAD << " with task " << mynum << " is done" );

        // int fetch add to address on Core1
        int64_t result = delegate::fetch_and_add( nf_addr, 1 );
        BOOST_MESSAGE( CURRENT_THREAD << " with task " << mynum << " result=" << result );
        if ( result == num_tasks-1 ) {
            Grappa::wake( parent );
        }
      });
    }

    Grappa::suspend(); // no wakeup race because tasks wont run until this yield occurs
                       // normally a higher level robust synchronization object should be used

    BOOST_MESSAGE( "testing shared args" );
    int64_t array[num_tasks]; ::memset(array, 0, sizeof(array));
    int64_t nf = 0;
    CompletionEvent joiner;
    
    int64_t * a = array;
    
    auto g_nf = make_global(&nf);

    for (int64_t i=0; i<num_tasks; i++) {
      joiner.enroll();
      privateTask([i,a,&joiner,g_nf]{
        BOOST_MESSAGE( CURRENT_THREAD << " with task " << i );

        int64_t result = delegate::fetch_and_add(g_nf, 1);
        a[i] = result;

        BOOST_MESSAGE( CURRENT_THREAD << " with task " << i << " about to yield" );
        Grappa::yield();
        BOOST_MESSAGE( CURRENT_THREAD << " with task " << i << " is done" );

        BOOST_MESSAGE( CURRENT_THREAD << " with task " << i << " result=" << result );
  
        joiner.complete();
      });
    }
  
    joiner.wait();
  
    BOOST_CHECK_EQUAL(nf, num_tasks);
  
    for (int i=0; i<num_tasks; i++) {
      BOOST_CHECK( array[i] >= 0 );
    }
  
    Statistics::merge_and_print();
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
