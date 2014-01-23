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
#include "CompletionEvent.hpp"

#include <string>

DECLARE_uint64( num_starting_workers );
DEFINE_uint64( lines, 512, "Cachelines to touch before context switch" );

using namespace Grappa;

CompletionEvent * final;
CompletionEvent * task_barrier;
CompletionEvent * task_signal;
        
Grappa::Timestamp start_ts, end_ts;

struct Cacheline {
  uint64_t val;
  char padding[56];
};

const size_t MAX_ARRAY_SIZE = 5*1024*1024;
Cacheline myarray[MAX_ARRAY_SIZE];

BOOST_AUTO_TEST_SUITE( ContextSwitchLatency_tests );

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    srand((unsigned int)Grappa::walltime());
  
    final = new CompletionEvent(2);
    task_barrier = new CompletionEvent(2);
    task_signal = new CompletionEvent(1);

    // BOOST_CHECK( Grappa::cores() == 1 );
    BOOST_CHECK( FLAGS_lines <= MAX_ARRAY_SIZE );
      spawn( [] {

        // wait for all to start (to hack scheduler yield)
        task_barrier->complete();
        task_barrier->wait();

        // blow out cache
        for ( uint64_t i=0; i<FLAGS_lines; i++ ) {
          myarray[i].val += 1;
        }

          Grappa::tick();
          start_ts = Grappa::timestamp();
        
          task_signal->complete();
          final->complete();
          });

      spawn( [] {
        // wait for all to start (to hack scheduler yield)
        task_barrier->complete();
        task_barrier->wait();
          task_signal->wait();

          Grappa::tick();
          end_ts = Grappa::timestamp();

          final->complete();
          });

      final->wait();

        BOOST_MESSAGE( "ticks = " << end_ts << " - " << start_ts << " = " << end_ts-start_ts );
        BOOST_MESSAGE( "time = " << ((double)(end_ts-start_ts)) / Grappa::tick_rate );


    BOOST_MESSAGE( "user main is exiting" );
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
