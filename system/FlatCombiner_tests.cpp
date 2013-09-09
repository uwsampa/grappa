
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
#include "Statistics.hpp"
#include "FlatCombiner.hpp"
#include "GlobalCounter.hpp"
using namespace Grappa;

BOOST_AUTO_TEST_SUITE( FlatCombiner_tests );

struct Foo {
  long x, y;
  double z;
} __attribute__((aligned(BLOCK_SIZE)));

void user_main( void * ignore ) {
  CHECK_EQ(sizeof(Foo), block_size);
  auto f = mirrored_global_alloc<Foo>();
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
  
  Statistics::merge_and_print();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  Grappa_activate();

  Grappa_run_user_main( &user_main, (void*)NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

