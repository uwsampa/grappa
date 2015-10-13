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
