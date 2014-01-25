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
#include "Message.hpp"
#include "MessagePool.hpp"
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
