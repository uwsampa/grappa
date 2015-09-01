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
#include "DictOut.hpp"

//
// Benchmarking scheduler performance.
// Currently calculates average context switch time when there are no other user threads.
//

#ifndef BILLION
#define BILLION 1000000000
#endif

BOOST_AUTO_TEST_SUITE( Scheduler_benchmarking_tests );

int64_t warmup_iters = 1<<18;
int64_t iters = 1<<24;

#include <sys/time.h>
double wctime() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec + 1E-6 * tv.tv_usec);
}

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{

    double start, end;

    // warmup context switches  
    for (int64_t i=0; i<warmup_iters; i++) {
      Grappa::yield();
    }

    // time many context switches
    start = wctime();
    for (int64_t i=0; i<iters; i++) {
      Grappa::yield();
    }
    end = wctime();

    double runtime = end - start;

    DictOut d;
    d.add( "iterations", iters );
    d.add( "runtime", runtime ); 
    BOOST_MESSAGE( d.toString() );

    // average time per context switch
    BOOST_MESSAGE( (runtime / iters) * BILLION << " ns / switch" );

    BOOST_MESSAGE( "user main is exiting" );
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();

