////////////////////////////////////////////////////////////////////////
// This file is part of Grappa, a system for scaling irregular
// applications on commodity clusters. 

// Copyright (C) 2010-2014 University of Washington and Battelle
// Memorial Institute. University of Washington authorizes use of this
// Grappa software.

// Grappa is free software: you can redistribute it and/or modify it
// under the terms of the Affero General Public License as published
// by Affero, Inc., either version 1 of the License, or (at your
// option) any later version.

// Grappa is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Affero General Public License for more details.

// You should have received a copy of the Affero General Public
// License along with this program. If not, you may obtain one from
// http://www.affero.org/oagpl.html.
////////////////////////////////////////////////////////////////////////

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
      auto parent = Grappa::current_worker();
    
      spawn([ta,parent,nf_addr]{
        int mynum = ta;

        BOOST_MESSAGE( Grappa::current_worker() << " with task " << mynum << " about to yield 1" );
        Grappa::yield( );
        BOOST_MESSAGE( Grappa::current_worker() << " with task " << mynum << " about to yield 2" );
        Grappa::yield( );
        BOOST_MESSAGE( Grappa::current_worker() << " with task " << mynum << " is done" );

        // int fetch add to address on Core1
        int64_t result = delegate::fetch_and_add( nf_addr, 1 );
        BOOST_MESSAGE( Grappa::current_worker() << " with task " << mynum << " result=" << result );
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
      spawn([i,a,&joiner,g_nf]{
        BOOST_MESSAGE( Grappa::current_worker() << " with task " << i );

        int64_t result = delegate::fetch_and_add(g_nf, 1);
        a[i] = result;

        BOOST_MESSAGE( Grappa::current_worker() << " with task " << i << " about to yield" );
        Grappa::yield();
        BOOST_MESSAGE( Grappa::current_worker() << " with task " << i << " is done" );

        BOOST_MESSAGE( Grappa::current_worker() << " with task " << i << " result=" << result );
  
        joiner.complete();
      });
    }
  
    joiner.wait();
  
    BOOST_CHECK_EQUAL(nf, num_tasks);
  
    for (int i=0; i<num_tasks; i++) {
      BOOST_CHECK( array[i] >= 0 );
    }
  
    Metrics::merge_and_print();
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
