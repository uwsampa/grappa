
// Copyright 2010-2012 University of Washington. All Rights Reserved.
// LICENSE_PLACEHOLDER
// This software was created with Government support under DE
// AC05-76RL01830 awarded by the United States Department of
// Energy. The Government has certain rights in the software.

#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "ParallelLoop.hpp"
#include "GlobalAllocator.hpp"
#include "Delegate.hpp"
// #include "GlobalVector.hpp"
#include "Metrics.hpp"
#include "FlatCombiner.hpp"
#include "GlobalCounter.hpp"
using namespace Grappa;

BOOST_AUTO_TEST_SUITE( FlatCombiner_tests );

struct Foo {
  long x, y;
  double z;
} GRAPPA_BLOCK_ALIGNED;

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  Grappa::run([]{
    CHECK_EQ(sizeof(Foo), block_size);
    auto f = symmetric_global_alloc<Foo>();
    on_all_cores([f]{
      f->x = 1;
      f->y = 2;
      f->z = 3;
    });
  
    auto c = GlobalCounter::create();
  
    on_all_cores([c]{
      for (int i=0; i<10; i++) {
        c->incr();
      }
    });
  
    // auto count = c->([](GlobalCounter * self){ return self->master.count; });
    // BOOST_CHECK_EQUAL(count, c->count());
    // c->on_master([](GlobalCounter * self){ VLOG(0) << self; });
  
    BOOST_CHECK_EQUAL(c->count(), 10*cores());
    LOG(INFO) << "count = " << c->count();
  
    Metrics::merge_and_print();
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
