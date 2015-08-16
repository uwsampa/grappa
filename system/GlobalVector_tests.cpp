////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010-2015, University of Washington and Battelle
// Memorial Institute.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//     * Redistributions of source code must retain the above
//       copyright notice, this list of conditions and the following
//       disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials
//       provided with the distribution.
//     * Neither the name of the University of Washington, Battelle
//       Memorial Institute, or the names of their contributors may be
//       used to endorse or promote products derived from this
//       software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// UNIVERSITY OF WASHINGTON OR BATTELLE MEMORIAL INSTITUTE BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
// BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
// DAMAGE.
////////////////////////////////////////////////////////////////////////

#include <boost/test/unit_test.hpp>
#include "Grappa.hpp"
#include "ParallelLoop.hpp"
#include "GlobalAllocator.hpp"
#include "Delegate.hpp"
#include "GlobalVector.hpp"
#include "Metrics.hpp"

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

GRAPPA_DEFINE_METRIC(SummarizingMetric<double>, trial_time, 0);

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
  static std::default_random_engine e(12345L*mycore());
  static std::uniform_real_distribution<double> dist(0.0, 1.0);
  return dist(e) < probability;
}

enum class Exp { PUSH, POP, DEQUEUE, QUEUE, STACK };

template< Exp EXP >
double perf_test(GlobalAddress<GlobalVector<int64_t>> qa) {
  double t = Grappa::walltime();
  
  forall(0, FLAGS_nelems, [qa](int64_t i){
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
        // qa->pop();
        CHECK_GT(qa->pop(), -1);
      }
      
    } else if (EXP == Exp::PUSH) {
      qa->push(next_random<int64_t>());
    } else if (EXP == Exp::POP) {
      qa->pop();
    } else if (EXP == Exp::DEQUEUE) {
      qa->dequeue();
    }
  });
  
  t = Grappa::walltime() - t;
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
  
  // forall(qa->begin(), qa->size(), [](int64_t i, int64_t& e) { e = 9; });
  forall(qa, [](int64_t& e){ e = 9; });
    
  for (int i=0; i<N; i++) {
    BOOST_CHECK_EQUAL(delegate::read(qa->storage()+i), 9);
  }
  
  qa->destroy();
}

void test_dequeue() {
  auto qa = GlobalVector<long>::create(N);
  
  on_all_cores([qa]{
    auto r = blockDist(0, N, mycore(), cores());
    auto NC = r.end-r.start;
    for (int i=r.start; i<r.end; i++) {
      qa->enqueue(37);
    }
    auto size = qa->size();
    BOOST_CHECK(size >= NC);
    if (mycore() == 1) barrier();    
    
    forall_here(0, NC/2, [qa](int64_t i) {
      auto e = qa->dequeue();
      BOOST_CHECK_EQUAL(e, 37);
    });
    if (mycore() != 1) barrier();
    forall_here(0, NC-NC/2, [qa](int64_t i) {
      auto e = qa->dequeue();
      BOOST_CHECK_EQUAL(e, 37);
    });
  });
  BOOST_CHECK_EQUAL(qa->size(), 0);
  
  qa->destroy();
}

void test_stack() {
  LOG(INFO) << "testing stack...";
  auto sa = GlobalVector<long>::create(N);
  forall(sa->storage(), N, [](long& e){ e = -1; });
  
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

using StatTuple = std::tuple<long,long,long,long,long,long,long,long>;
StatTuple save_global_vector_stats() {
  return std::make_tuple(global_vector_push_msgs,
                    global_vector_push_ops,
                    global_vector_pop_msgs,
                    global_vector_pop_ops,
                    global_vector_deq_msgs,
                    global_vector_deq_ops,
                    global_vector_matched_pushes,
                    global_vector_matched_pops);
}
void restore_global_vector_stats(StatTuple t) {
  global_vector_push_msgs = std::get<0>(t);
  global_vector_push_ops  = std::get<1>(t);
  global_vector_pop_msgs  = std::get<2>(t);
  global_vector_pop_ops   = std::get<3>(t);
  global_vector_deq_msgs  = std::get<4>(t);
  global_vector_deq_ops   = std::get<5>(t);
  global_vector_matched_pushes = std::get<6>(t);
  global_vector_matched_pops   = std::get<7>(t);
}

BOOST_AUTO_TEST_CASE( test1 ) {
  Grappa::init( GRAPPA_TEST_ARGS );
  
  N = FLAGS_nelems;
  
  Grappa::run([]{
    if (FLAGS_queue_perf || FLAGS_stack_perf) {
      LOG(INFO) << "beginning performance test";
      auto qa = GlobalVector<int64_t>::create(FLAGS_vector_size);
    
      for (int i=0; i<FLAGS_ntrials; i++) {
        if (FLAGS_fraction_push < 1.0) { // fill halfway so we don't hit either rail
          auto saved = save_global_vector_stats();
          long diff = (FLAGS_vector_size/2) - qa->size();
          if (diff > 0) { // too small
            forall_here(0, diff, [qa](int64_t i){
              qa->push(next_random<int64_t>());
            });
          } else {  // too large
            forall_here(0, 0-diff, [qa](int64_t i){
              qa->pop();
            });
          }
          restore_global_vector_stats(saved);
        }
      
        if (FLAGS_queue_perf) {
          trial_time += perf_test<Exp::QUEUE>(qa);
        } else if (FLAGS_stack_perf) {
          trial_time += perf_test<Exp::STACK>(qa);
        }
        VLOG(0) << "global_vector_push_ops = " << global_vector_push_ops;
      
      }
    
      Metrics::merge_and_print();
      qa->destroy();
    
    } else {
      test_global_vector();
      test_dequeue();
      test_stack();
    }
  
  });
  Grappa::finalize();
}

BOOST_AUTO_TEST_SUITE_END();
