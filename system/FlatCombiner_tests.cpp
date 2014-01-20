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
