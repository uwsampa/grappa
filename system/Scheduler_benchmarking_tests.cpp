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
#include "DictOut.hpp"

//
// Benchmarking scheduler performance.
// Currently calculates average context switch time when there are no other user threads.
//

#define BILLION 1000000000

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

