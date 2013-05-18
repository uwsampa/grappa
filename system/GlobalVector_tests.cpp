
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

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/variate_generator.hpp>

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( GlobalVector_tests );

static size_t N = (1L<<10) - 21;

DEFINE_int64(nelems, N, "number of elements in (large) test arrays");
// DEFINE_int64(buffer_size, 1<<10, "number of elements in buffer");
DEFINE_int64(ntrials, 1, "number of independent trials to average over");

DEFINE_bool(perf, false, "do performance test");

GRAPPA_DEFINE_STAT(SummarizingStatistic<double>, push_time, 0);
GRAPPA_DEFINE_STAT(SummarizingStatistic<double>, push_const_time, 0);

template< typename T >
inline T next_random() {
  using engine_t = boost::mt19937_64;
  using dist_t = boost::uniform_int<T>;
  using gen_t = boost::variate_generator<engine_t&,dist_t>;
  
  // continue using same generator with multiple calls (to not repeat numbers)
  // but start at different seed on each node so we don't get overlap
  static engine_t engine(12345L*mycore());
  static gen_t gen(engine, dist_t(0, std::numeric_limits<T>::max()));
  return gen();
}

template< bool             RANDOM = true,
          CompletionEvent* CE     = &impl::local_ce,
          int64_t          TH     = impl::USE_LOOP_THRESHOLD_FLAG >
double push_perf_test(GlobalAddress<GlobalVector<int64_t>> qa) {
  qa->clear();
  double t = Grappa_walltime();
  
  forall_global_private<CE,TH>(0, N, [qa](int64_t s, int64_t n){
    for (int64_t i=s; i<s+n; i++) {
      if (RANDOM) {
        qa->push(next_random<int64_t>());
      } else {
        qa->push(42);
      }
    }
  });
  
  t = Grappa_walltime() - t;
  
  BOOST_CHECK_EQUAL(qa->size(), N);
  return t;
}

void test_global_vector() {
  BOOST_MESSAGE("Testing GlobalVector"); VLOG(1) << "testing global queue";
  
  auto qa = GlobalVector<int64_t>::create(N);
  VLOG(1) << "queue addr: " << qa;
  
  BOOST_CHECK_EQUAL(qa->empty(), true);
  
  on_all_cores([qa] {
    auto f = [qa](int64_t s, int64_t n) {
      for (int64_t i=s; i<s+n; i++) {
        qa->push(7);
      }
      BOOST_CHECK_EQUAL(qa->empty(), false);
    };
    switch (mycore()) {
    case 0:
      forall_here(0, N/2, f);
      break;
    case 1:
      forall_here(N/2, N-N/2, f);
      break;
    }
  });
  
  for (int i=0; i<N; i++) {
    BOOST_CHECK_EQUAL(delegate::read(qa->storage()+i), 7);
  }
  
  // forall_localized(qa->begin(), qa->size(), [](int64_t i, int64_t& e) { e = 9; });
  forall_localized(qa, [](int64_t i, int64_t& e){ e = 9; });
  
  for (int i=0; i<N; i++) {
    BOOST_CHECK_EQUAL(delegate::read(qa->storage()+i), 9);
  }
  
  qa->destroy();
}

void test_dequeue() {
  auto qa = GlobalVector<long>::create(N);
  
  on_all_cores([qa]{
    auto NC = N/cores();
    for (int i=0; i<NC; i++) {
      qa->enqueue(42);
    }
    auto size = qa->size();
    VLOG(0) << "size = " << size;
    BOOST_CHECK(size >= NC);
    if (mycore() == 1) barrier();    
    
    forall_here(0, NC/2, [qa](long s, long n) {
      for (int i=s; i<s+n; i++) {
        auto e = qa->dequeue();
        BOOST_CHECK(e == 42);
      }
    });
    if (mycore() != 1) barrier();
    forall_here(0, NC-NC/2, [qa](long s, long n) {
      for (int i=s; i<s+n; i++) {
        auto e = qa->dequeue();
        BOOST_CHECK(e == 42);
      }
    });
  });
  BOOST_CHECK_EQUAL(qa->size(), 0);
}

void user_main( void * ignore ) {
  if (FLAGS_perf) {
    double t;
    auto qa = GlobalVector<int64_t>::create(N);
    
    for (int i=0; i<FLAGS_ntrials; i++) {
      t = push_perf_test<true>(qa);
      LOG_FIRST_N(INFO,1) << "push_time: " << t;
      push_time += t;
      
      t = push_perf_test<false>(qa);
      LOG_FIRST_N(INFO,1) << "push_const_time: " << t;
      push_const_time += t;
    }
    
    qa->destroy();
    
  } else {
    test_global_vector();
    test_dequeue();
  }
  
  Statistics::merge_and_print();
}

BOOST_AUTO_TEST_CASE( test1 ) {

  Grappa_init( &(boost::unit_test::framework::master_test_suite().argc),
                &(boost::unit_test::framework::master_test_suite().argv) );

  Grappa_activate();
  N = FLAGS_nelems;

  Grappa_run_user_main( &user_main, (void*)NULL );

  Grappa_finish( 0 );
}

BOOST_AUTO_TEST_SUITE_END();

