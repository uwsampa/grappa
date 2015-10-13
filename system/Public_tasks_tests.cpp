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
#include "Message.hpp"
#include "CompletionEvent.hpp"
#include "ConditionVariable.hpp"
#include "Delegate.hpp"
#include "Tasking.hpp"
#include "GlobalAllocator.hpp"
#include "ParallelLoop.hpp"
#include "Array.hpp"

BOOST_AUTO_TEST_SUITE( Public_tasks_tests );

using namespace Grappa;

DEFINE_uint64( N, 1<<10, "iters");

uint64_t * finished;
CompletionEvent * ce;
uint64_t finished_local = 0;

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    ce = new CompletionEvent(FLAGS_N);

    on_all_cores([] {
      finished = new uint64_t[FLAGS_N];
      for (int i=0; i<FLAGS_N; i++) {
        finished[i] = 0;
       }
    });

    for (int i=0; i<FLAGS_N; i++) {
  //    BOOST_MESSAGE("Spawn " << i);
      spawn<unbound>( [i] () {
  //      BOOST_MESSAGE("Run " << i << " on " << Grappa::mycore());
        finished_local++;
        finished[i] = 1;

        delegate::call( 0, [] {
          ce->complete();
          }); 
      });
    }
    ce->wait();

    uint64_t total = 0;
    for (uint64_t i=0; i<Grappa::cores(); i++) {
      total += delegate::read(make_global(&finished_local,i)); 
    }
  
    BOOST_MESSAGE( "total=" << total );
    BOOST_CHECK( total == FLAGS_N );

  //  for ( uint64_t i = 0; i<N; i++ ) {
  //    uint64_t other = delegate::read( make_global(&finished[i],1));
  //    BOOST_MESSAGE( "i " << i << " fi "<<finished[i] <<" o " << other );
  //    BOOST_CHECK( (finished[i] ^ other) == 1 );
  //  }

  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
