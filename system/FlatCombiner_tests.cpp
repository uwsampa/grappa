
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

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( FlatCombiner_tests );

const Core MASTER = 0;

class GlobalCounter {
public:
  
  struct Master {
    long count;
  } master;
  
  GlobalAddress<GlobalCounter> self;
  
  struct Proxy {
    GlobalCounter* outer;
    long delta;
    
    Proxy(GlobalCounter* outer): outer(outer), delta(0) {}
    Proxy* clone_fresh() { return locale_new<Proxy>(outer); }
    
    void sync() {
      auto s = outer->self;
      auto d = delta;
      delegate::call(MASTER,[s,d]{ s->master.count += d; });
    }
    
    bool is_full() { return false; }
  };
  FlatCombiner<Proxy> comb;
  
  char pad[block_size - sizeof(comb)-sizeof(self)-sizeof(master)];
  
public:
  GlobalCounter(long initial_count = 0): comb(locale_new<Proxy>(this)) {
    master.count = initial_count;
  }
  
  void incr(long d = 1) {
    comb.combine([d](Proxy& p){
      p.delta += d;
    });
  }
  
  long count() {
    auto s = self;
    return delegate::call(MASTER, [s]{ return s->master.count; });
  }
  
  static GlobalAddress<GlobalCounter> create(long initial_count = 0) {
    auto a = mirrored_global_alloc<GlobalCounter>();
    call_on_all_cores([a]{ new (a.localize()) GlobalCounter(0); });
    return a;
  }
  
  void destroy() {
    auto a = self;
    call_on_all_cores([a]{ a->~GlobalCounter(); });
  }
};

void user_main( void * ignore ) {
  
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

