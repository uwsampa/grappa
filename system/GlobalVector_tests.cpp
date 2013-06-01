
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
#include <random>

using namespace Grappa;

BOOST_AUTO_TEST_SUITE( GlobalVector_tests );

static size_t N = (1L<<10) - 21;

DEFINE_int64(nelems, N, "number of elements in (large) test arrays");
DEFINE_int64(vector_size, N, "number of elements in (large) test arrays");
DEFINE_double(fraction_push, 0.5, "fraction of accesses that should be pushes");

// DEFINE_int64(buffer_size, 1<<10, "number of elements in buffer");
DEFINE_int64(ntrials, 1, "number of independent trials to average over");

DEFINE_bool(queue_perf, false, "do queue performance test");
DEFINE_bool(stack_perf, false, "do stack performance test");

GRAPPA_DEFINE_STAT(SummarizingStatistic<double>, trial_time, 0);

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

bool choose_random(double probability) {
  std::default_random_engine e(12345L);
  std::uniform_real_distribution<double> dist(0.0, 1.0);
  return dist(e) < probability;
}

enum class Exp { PUSH, POP, DEQUEUE, QUEUE, STACK };

template< Exp EXP >
double perf_test(GlobalAddress<GlobalVector<int64_t>> qa) {
  double t = Grappa_walltime();
  
  forall_global_private(0, FLAGS_nelems, [qa](int64_t i){
    if (EXP == Exp::QUEUE) {
      
      if (choose_random(FLAGS_fraction_push)) {
        qa->push(next_random<int64_t>());
      } else {
        qa->dequeue();
      }
      
    } else if (EXP == Exp::STACK) {
      
      if (choose_random(FLAGS_fraction_push)) {
        qa->push(next_random<int64_t>());
      } else {
        qa->pop();
      }
      
    } else if (EXP == Exp::PUSH) {
      qa->push(next_random<int64_t>());
    } else if (EXP == Exp::POP) {
      qa->pop();
    } else if (EXP == Exp::DEQUEUE) {
      qa->dequeue();
    }
  });
  
  t = Grappa_walltime() - t;
  return t;
}

void test_global_vector() {
  BOOST_MESSAGE("Testing GlobalVector"); VLOG(1) << "testing global queue";
  
  auto qa = GlobalVector<int64_t>::create(N);
  VLOG(1) << "queue addr: " << qa;
  
  BOOST_CHECK_EQUAL(qa->empty(), true);
  
  VLOG(0) << qa->storage();
  
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
  forall_localized(qa, [](int64_t& e){ e = 9; });
    
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
      qa->enqueue(37);
    }
    auto size = qa->size();
    BOOST_CHECK(size >= NC);
    if (mycore() == 1) barrier();    
    
    forall_here(0, NC/2, [qa](long s, long n) {
      for (int i=s; i<s+n; i++) {
        auto e = qa->dequeue();
        BOOST_CHECK_EQUAL(e, 37);
      }
    });
    if (mycore() != 1) barrier();
    forall_here(0, NC-NC/2, [qa](long s, long n) {
      for (int i=s; i<s+n; i++) {
        auto e = qa->dequeue();
        BOOST_CHECK_EQUAL(e, 37);
      }
    });
  });
  BOOST_CHECK_EQUAL(qa->size(), 0);
  
  qa->destroy();
}

void test_stack() {
  LOG(INFO) << "testing stack...";
  auto sa = GlobalVector<long>::create(N);
  forall_localized(sa->storage(), N, [](int64_t& e){ e = -1; });
  
  on_all_cores([sa]{
    forall_here(0, 100, [sa](int64_t i) {
      sa->push(17);
      BOOST_CHECK_EQUAL(sa->pop(), 17);
    });
  });
  LOG(INFO) << "second phase...";
  on_all_cores([sa]{
    for (int i=0; i<10; i++) sa->push(43);
    for (int i=0; i<5; i++) BOOST_CHECK_EQUAL(sa->pop(), 43);
  });
  BOOST_CHECK_EQUAL(sa->size(), cores()*5);
  
  while (!sa->empty()) BOOST_CHECK_EQUAL(sa->pop(), 43);
  BOOST_CHECK_EQUAL(sa->size(), 0);
  BOOST_CHECK(sa->empty());
  
  sa->destroy();
}

void user_main( void * ignore ) {
  if (FLAGS_queue_perf || FLAGS_stack_perf) {
    LOG(INFO) << "beginning performance test";
    auto qa = GlobalVector<int64_t>::create(FLAGS_vector_size);
    
    for (int i=0; i<FLAGS_ntrials; i++) {
      qa->clear();
      if (FLAGS_fraction_push < 1.0) { // fill halfway so we don't hit either rail
        forall_global_public(0, FLAGS_vector_size/2, [qa](int64_t i){
          qa->push(next_random<int64_t>());
        });
      }
      
      if (FLAGS_queue_perf) {
        trial_time += perf_test<Exp::QUEUE>(qa);
      } else if (FLAGS_stack_perf) {
        trial_time += perf_test<Exp::STACK>(qa);
      }
    }
    
    Statistics::merge_and_print();
    qa->destroy();
    
  } else {
    test_global_vector();
    test_dequeue();
    test_stack();
  }
  
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

