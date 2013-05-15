
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
#include "GlobalVector.hpp"
#include "Statistics.hpp"
#include "FlatCombiner.hpp"

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( FlatCombiner_tests );

const Core MASTER = 0;

class Counter {
protected:
  
  struct Master {
    long count;
  } master;
  
  GlobalAddress<Counter> self;
  
  struct Proxy {
    Counter* outer;
    long delta;
    
    Proxy(Counter* outer): outer(outer), delta(0) {}
    Proxy* clone_fresh() { return new Proxy(outer); }
    
    void sync() {
      auto s = outer->self;
      auto d = delta;
      delegate::call(MASTER,[s,d]{ s->master.count += d; });
    }
    
    bool is_full() { return false; }
  };
  FlatCombiner<Proxy> comb;
  
  // char pad[block_size - sizeof(comb)-sizeof(self)-sizeof(master)];
  
public:
  Counter(long initial_count = 0): comb(new Proxy(this)) {
    master.count = initial_count;
  }
  
  void incr() {
    comb.combine([](Proxy& p){
      p.delta++;
    });
  }
  
  long count() {
    auto s = self;
    return delegate::call(MASTER, [s]{ return s->master.count; });
  }
  
  // static GlobalAddress<Counter> create(long initial_count = 0) {
  //   auto a = mirrored_global_alloc<Counter>();
  //   call_on_all_cores([a]{ new (a.localize()) Counter(0); });
  //   return a;
  // }
  // 
  // void destroy() {
  //   auto a = self;
  //   call_on_all_cores([a]{ a->~Counter(); });
  // }
};

void user_main( void * ignore ) {
  
  auto c = MirroredGlobal<Counter>::create([](Counter* c){ new (c) Counter(0); });
  
  on_all_cores([c]{
    for (int i=0; i<10; i++) {
      c->incr();
    }
  });
  
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

