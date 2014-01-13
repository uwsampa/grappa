// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


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

