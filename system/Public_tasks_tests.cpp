
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.


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
      publicTask( [i] () {
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
