////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
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
